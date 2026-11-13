# Basic_DB — root Makefile (Phase 0 scaffold)
# Everything builds with plain clang++; there are no external dependencies.

CXX       ?= clang++
CXXFLAGS  ?= -std=c++20 -Wall -Wextra -Werror -Wpedantic -g -O0
LDFLAGS   ?=
SANITIZE  ?= 1

ifeq ($(SANITIZE),1)
CXXFLAGS  += -fsanitize=address,undefined
LDFLAGS   += -fsanitize=address,undefined
endif

INCLUDES  := -Isrc -Itests
BUILD     := build

APP_SRCS  := $(shell find src -name '*.cpp')
TEST_SRCS := $(filter-out src/main.cpp,$(APP_SRCS)) $(wildcard tests/*.cpp)
HEADERS   := $(shell find src tests -name '*.h')

APP_BIN   := $(BUILD)/basicdb
TEST_BIN  := $(BUILD)/basicdb_tests

.PHONY: all test run clean

all: $(APP_BIN)

$(APP_BIN): $(APP_SRCS) $(HEADERS)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(APP_SRCS) -o $@ $(LDFLAGS)

$(TEST_BIN): $(TEST_SRCS) $(HEADERS)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(TEST_SRCS) -o $@ $(LDFLAGS)

test: $(TEST_BIN)
	./$(TEST_BIN)

run: $(APP_BIN)
	./$(APP_BIN)

clean:
	rm -rf $(BUILD)
