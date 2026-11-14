# Basic_DB

![CI](https://github.com/vipulktiwari/Basic_DB/actions/workflows/ci.yml/badge.svg)

A basic but fully functional relational database built from scratch in C++20.
See [DESIGN.md](DESIGN.md) for the architecture, component breakdown, and phased roadmap.

## Prerequisites

| Tool | Version | Check |
|---|---|---|
| C++20 compiler | Apple clang 15+ / LLVM clang 15+ / GCC 12+ | `clang++ --version` |
| make | GNU or BSD | `make --version` |

**No external libraries are required** — the project builds with the compiler alone.
On macOS the toolchain ships with the Xcode Command Line Tools (`xcode-select --install`).

## Quick start

```sh
git clone git@github.com:vipulktiwari/Basic_DB.git && cd Basic_DB   # skip if already here
make test                                                           # build everything & run the unit tests
```

Expected output (colorized in a real terminal):

```
[PASS] types.page_size_is_4_kib
[PASS] types.reserved_page_ids
[PASS] types.identifier_widths_are_pinned
[PASS] smoke.framework_itself

4/4 tests passed
```

## Build targets

| Command | What it does | Output |
|---|---|---|
| `make` / `make all` | Builds the main binary | `build/basicdb` |
| `make test` | Builds **and runs** the whole unit-test suite | `build/basicdb_tests` |
| `make run` | Builds and runs the main binary | — |
| `make clean` | Deletes the build directory | — |
| `make build/basicdb_tests` | Builds the test binary without running it | `build/basicdb_tests` |

## Running tests

```sh
make test                              # build + run everything
./build/basicdb_tests                  # re-run without rebuilding
./build/basicdb_tests types            # only tests whose suite.name contains "types"
./build/basicdb_tests types.reserved   # a single test
```

The exit code is `0` when all selected tests pass, `1` otherwise (CI-friendly).

## Build options

All knobs are Make variables — pass them on the command line:

```sh
make test SANITIZE=0                   # disable ASan/UBSan (enabled by default)
make CXX=g++ test                      # use a different compiler
make CXXFLAGS='-std=c++20 -O2' test    # fully custom flags
```

AddressSanitizer + UndefinedBehaviorSanitizer are **on by default**; the suite
should always pass clean under them. Note: Make rebuilds on *file* changes, not
flag changes — after switching `SANITIZE`/`CXXFLAGS`, run `make clean` first.

## Writing tests

Add a file `tests/test_<name>.cpp` — the Makefile picks up `tests/*.cpp`
automatically, no registration step needed:

```cpp
#include "test_framework.h"

TEST(my_suite, my_test) {
  ASSERT_TRUE(2 + 2 == 4);
  ASSERT_FALSE(1 > 2);
  ASSERT_EQ(actual, expected);   // prints both values on failure
  ASSERT_NE(a, b);
}
```

Assertions abort only the current test; a failure never stops the rest of the suite.

## Project layout

```
src/common/    foundational types & constants (page_id_t, lsn_t, PAGE_SIZE, …)
src/main.cpp   temporary shell entry point
tests/         zero-dependency test framework + one test file per component
DESIGN.md      architecture, component breakdown, phased roadmap (§6)
Makefile       all build/test entry points
```

## Troubleshooting

- **`clang++: command not found` (macOS)** → `xcode-select --install`
- **Sanitizer runtime fails to start on your platform** → `make clean && make test SANITIZE=0`
- **Weird stale binary after changing flags** → `make clean && make test`

## Status

Phase 0 scaffold: project structure, foundational types, zero-dependency test harness.
Roadmap and milestones: see §6 of [DESIGN.md](DESIGN.md).
