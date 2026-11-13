# Basic_DB — Design Document

A basic but **fully functional** relational database built from scratch: SQL in, durable rows out, with indexes, transactions, and crash recovery.

## 1. Goals & Non-Goals

**Goals (v1)**
- Relational model: tables, typed columns, rows
- SQL: `CREATE TABLE`, `CREATE INDEX`, `INSERT`, `SELECT`, `UPDATE`, `DELETE`, plus `WHERE`, `ORDER BY`, `LIMIT`, `GROUP BY`, basic joins
- Durable disk storage with write-ahead logging (WAL) and crash recovery
- Fast access paths: B+ tree indexes + sequential scans
- ACID transactions with strict 2PL concurrency control
- Client/server over TCP + interactive CLI

**Non-Goals (v1)**
- Distributed operation / replication
- Full SQL surface (no stored procedures, triggers, views, foreign keys — stretch goals)
- Cost-based optimizer (rule-based first)
- Columnar / OLAP storage

## 2. High-Level Architecture

```
                ┌──────────────────────────┐
                │   CLI client / drivers   │
                └────────────┬─────────────┘
                             │ TCP · text wire protocol
┌────────────────────────────▼─────────────────────────────┐
│ 9. SERVER          sessions, connections, simple auth    │
├──────────────────────────────────────────────────────────┤
│ 8. QUERY PROCESSOR                                       │
│    SQL → Lexer/Parser → AST → Binder → Planner →         │
│    Executor (volcano: scan/filter/project/join/agg)      │
├──────────────────────────────────────────────────────────┤
│ 7. TRANSACTION MGR   BEGIN/COMMIT/ABORT, txn ids         │
│ 6. CONCURRENCY       lock manager, strict 2PL,           │
│                      deadlock detection                  │
├──────────────────────────────────────────────────────────┤
│ 5. ACCESS METHODS    heap scan · B+ tree index           │
│ 4. RECORD MANAGER    slotted pages, tuple ser/de, RID    │
├──────────────────────────────────────────────────────────┤
│ 3. BUFFER POOL       page cache, eviction, pin/dirty     │
├──────────────────────────────────────────────────────────┤
│ 2. DISK MANAGER      page allocation, file read/write    │
│ 1. WAL / RECOVERY    append-only log, checkpoints,       │
│                      ARIES-style recovery                │
└────────────────────────────┬─────────────────────────────┘
                       ┌─────▼──────┐
                       │  OS / disk │
                       └────────────┘
```

Cross-cutting: **Catalog** (system tables for schemas), **type system & serialization**, **test harness**.

## 3. Core Design Decisions

| Decision | Choice | Rationale |
|---|---|---|
| Data model | Relational, row-oriented (NSM) | Simplest correct model for OLTP-style access |
| Page size | 4 KiB fixed | Matches OS pages; simple buffer pool math |
| Record ID | RID = (page_id, slot_id) | Stable across slotted-page compaction |
| Index | B+ tree | Balanced; point + range queries; the industry default |
| Execution | Volcano iterators | Composable; add operators one at a time |
| Buffer policy | Steal / no-force | Fast commits; recovery handled by WAL |
| Concurrency | Strict 2PL + deadlock detection | Correct & explainable; MVCC is a later upgrade |
| Language | TBD | Pick before Phase 1 |

## 4. Component Breakdown

### 4.1 Disk Manager
Owns the database file; everything on disk is a fixed-size page.
- `allocate_page() → page_id` · `free_page(id)` · `read_page(id)` · `write_page(id, bytes)`
- Page 0 = file header: magic, version, page size, page count, free-list head
- Freed pages recycled via an on-disk free list

### 4.2 Buffer Pool Manager
The *only* path through which upper layers touch pages.
- Fixed frame array (e.g. 128 frames); page table `page_id → frame`
- Per-frame: `pin_count`, `dirty` flag, page LSN
- `fetch_page` · `new_page` · `unpin_page(id, dirty)` · `flush_page` · `flush_all`
- Eviction: LRU or Clock; pinned pages are never evicted
- **WAL rule**: refuse to flush a dirty page whose pageLSN > flushed-log LSN

### 4.3 Record Manager (heap file + slotted pages)
Unordered record storage per table.
- Slotted page: header (LSN, slot count, free ptr) → slot directory (offset,len) grows down, records grow up
- Deletes tombstone the slot; page compaction reuses space without changing RIDs
- Tuple layout: null bitmap + fixed fields inline + var-length fields with offsets
- `insert(tuple) → RID` · `get(RID)` · `update(RID, tuple)` · `delete(RID)` · `scan()`

### 4.4 Catalog
Self-describing metadata stored as ordinary tables.
- `sys_tables(table_id, name, first_page)` · `sys_columns(table_id, name, type, ordinal, nullable)` · `sys_indexes(index_id, table_id, name, keys, root_page)`
- Bootstrapped with hardcoded schemas, then read/written via the normal record layer
- Binder/planner resolve all names through the catalog

### 4.5 B+ Tree Index
- Internal nodes `(key, child_page)+`; leaves `(key, RID)+` with sibling links for range scans
- `search(key)` · `insert(key, RID)` (splits propagate up) · `delete(key)` (merge/redistribute) · `range(lo, hi)` iterator
- Duplicate keys handled by sorting on composite `(key, RID)`
- Root page id lives in the catalog

### 4.6 Query Processor
- **Lexer/Parser** — hand-written recursive descent, no dependencies; SQL text → AST
- **Binder** — resolve names via catalog, type-check, fold constants
- **Planner (v1, rule-based)** — predicate pushdown; pick index scan when a predicate matches an index prefix; greedy join order
- **Executor (volcano)** — every operator: `open() / next() / close()`
  - v1: `SeqScan`, `IndexScan`, `Filter`, `Project`, `Insert`, `Update`, `Delete`
  - v2: `NestedLoopJoin` → `HashJoin`, `Aggregate`, `Sort`, `Limit`

### 4.7 Transactions & Concurrency
- `BEGIN / COMMIT / ROLLBACK`; monotonic txn ids
- Lock manager: S/X locks at table + RID granularity, upgrades, strict 2PL (hold to commit) → serializable isolation by construction
- Deadlock detection: periodic waits-for graph cycle check; youngest txn aborts

### 4.8 WAL & Recovery
- Append-only log; record = `[LSN, txn, prev_LSN, type, payload]`
- Types: `BEGIN, COMMIT, ABORT, UPDATE (old+new), CLR, CHECKPOINT`
- Commit = flush log up to txn's last LSN, then ack (data pages flush later, lazily)
- Recovery (simplified ARIES): analysis → redo everything → undo losers with CLRs
- Fuzzy checkpoints bound recovery time

## 5. Key Data Flows

**INSERT**
1. Server → parser → binder → `Insert` plan
2. Tuple serialized → heap `insert()` through buffer pool (page pinned)
3. WAL `UPDATE` appended *before* page change; page LSN stamped
4. B+ tree leaf updated the same way; pages unpinned dirty
5. `COMMIT` → log flushed → client acked (data pages flush later, lazily)

**SELECT (indexed)**
1. Parse → bind → planner chooses `IndexScan`
2. B+ tree lookup → RIDs → heap fetch via buffer pool
3. `Filter`/`Project` shape rows → wire protocol encodes the result set

## 6. Build Phases — every phase ends green & testable

| # | Deliverable | Milestone |
|---|---|---|
| 0 | Scaffold: build, test harness, CI | Unit tests run |
| 1 | Disk manager, slotted pages, heap file, tuple ser/de | Insert/scan records in tests |
| 2 | Buffer pool (eviction, flushing, WAL rule) | Eviction-under-pressure tests pass |
| 3 | Catalog + mini-REPL (commands, pre-SQL) | Create/insert/scan a table; **restart → data persists** |
| 4 | B+ tree index | Point/range lookups; randomized split/merge fuzzing passes |
| 5 | SQL parser + binder + volcano executor | `CREATE TABLE / INSERT / SELECT … WHERE` end-to-end |
| 6 | UPDATE/DELETE, joins, ORDER BY, LIMIT, aggregates | SQL logic test suite passes |
| 7 | WAL + checkpoints + recovery | `kill -9` mid-write: committed survives, uncommitted rolls back |
| 8 | Txns + lock manager + deadlock detection | Concurrent clients stay serializable |
| 9 | TCP server + CLI + `EXPLAIN`/metrics | psql-like experience over the wire |

## 7. Testing Strategy
- **Unit**: page layout round-trips, eviction policy, B+ tree invariants after random ops
- **Property/fuzz**: B+ tree vs. sorted-map model under random op sequences; parser fuzzing
- **SQL logic tests**: files of `statement → expected output` (sqllogictest-style)
- **Crash**: fault injection around flush/commit; verify recovery invariants
- **Concurrency**: deterministic schedules + randomized stress with serializability checks

## 8. Stretch Goals (post-v1)
- MVCC / snapshot isolation for lock-free reads
- Cost-based optimizer with statistics
- Hash join everywhere, external sort, parallel scans
- Page compression
- Basic replication / read replicas


