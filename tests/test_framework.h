// Minimal zero-dependency unit-test framework for Basic_DB.
//
// Usage:
//   #include "test_framework.h"
//   TEST(suite_name, test_name) {
//     ASSERT_TRUE(1 + 1 == 2);
//     ASSERT_EQ(actual, expected);
//   }
//
// Tests self-register via static initialization. Link tests/test_main.cpp to get
// a main() that runs everything; the test binary accepts an optional substring
// argument to run only matching tests (e.g. `./basicdb_tests types`).
#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace testfw {

struct TestCase {
  std::string suite;
  std::string name;
  std::function<void()> body;
};

// One shared registry across all translation units.
inline std::vector<TestCase> &registry() {
  static std::vector<TestCase> instance;
  return instance;
}

struct Registrar {
  Registrar(std::string suite, std::string name, std::function<void()> body) {
    registry().push_back({std::move(suite), std::move(name), std::move(body)});
  }
};

// Thrown by a failed assertion to abort the current test (caught by run_all).
class TestFailure : public std::exception {
 public:
  TestFailure(const char *file, int line, std::string message) {
    std::ostringstream oss;
    oss << file << ':' << line << ": " << message;
    what_ = oss.str();
  }
  const char *what() const noexcept override { return what_.c_str(); }

 private:
  std::string what_;
};

[[noreturn]] inline void fail(const char *file, int line, const std::string &message) {
  throw TestFailure(file, line, message);
}

template <typename A, typename B>
void assert_eq(const A &actual, const B &expected, const char *a_expr, const char *b_expr,
               const char *file, int line) {
  if (!(actual == expected)) {
    std::ostringstream oss;
    oss << "ASSERT_EQ failed: " << a_expr << " == " << b_expr << "\n    actual:   " << actual
        << "\n    expected: " << expected;
    fail(file, line, oss.str());
  }
}

template <typename A, typename B>
void assert_ne(const A &actual, const B &expected, const char *a_expr, const char *b_expr,
               const char *file, int line) {
  if (!(actual != expected)) {
    std::ostringstream oss;
    oss << "ASSERT_NE failed: " << a_expr << " != " << b_expr << "\n    both were: " << actual;
    fail(file, line, oss.str());
  }
}

constexpr const char *kGreen = "\033[32m";
constexpr const char *kRed   = "\033[31m";
constexpr const char *kBold  = "\033[1m";
constexpr const char *kReset = "\033[0m";

// Runs every registered test whose "suite.name" contains the optional CLI
// filter (empty filter → run everything). Returns 0 when all selected pass.
inline int run_all(int argc, char **argv) {
  const std::string filter = (argc > 1) ? argv[1] : "";
  int passed = 0;
  int failed = 0;
  int skipped = 0;
  for (const auto &tc : registry()) {
    const std::string full_name = tc.suite + '.' + tc.name;
    if (!filter.empty() && full_name.find(filter) == std::string::npos) {
      ++skipped;
      continue;
    }
    try {
      tc.body();
      std::cout << kGreen << "[PASS] " << kReset << full_name << '\n';
      ++passed;
    } catch (const std::exception &e) {
      std::cout << kRed << "[FAIL] " << kReset << full_name << "\n       " << e.what() << '\n';
      ++failed;
    } catch (...) {
      std::cout << kRed << "[FAIL] " << kReset << full_name << " (unknown exception)\n";
      ++failed;
    }
  }
  const int total = passed + failed;
  std::cout << kBold << (failed == 0 ? kGreen : kRed) << '\n'
            << passed << "/" << total << " tests passed" << kReset;
  if (skipped > 0) {
    std::cout << "  (" << skipped << " skipped by filter \"" << filter << "\")";
  }
  std::cout << '\n';
  return failed == 0 ? 0 : 1;
}

}  // namespace testfw

#define TEST(suite_name, test_name)                                                            \
  static void suite_name##_##test_name##_body();                                               \
  static ::testfw::Registrar suite_name##_##test_name##_registrar(                             \
      #suite_name, #test_name, suite_name##_##test_name##_body);                               \
  static void suite_name##_##test_name##_body()

#define ASSERT_TRUE(cond)                                                                      \
  do {                                                                                         \
    if (!(cond)) {                                                                             \
      ::testfw::fail(__FILE__, __LINE__, std::string("ASSERT_TRUE failed: ") + #cond);         \
    }                                                                                          \
  } while (0)

#define ASSERT_FALSE(cond)                                                                     \
  do {                                                                                         \
    if ((cond)) {                                                                              \
      ::testfw::fail(__FILE__, __LINE__, std::string("ASSERT_FALSE failed: ") + #cond);        \
    }                                                                                          \
  } while (0)

#define ASSERT_EQ(actual, expected)                                                            \
  ::testfw::assert_eq((actual), (expected), #actual, #expected, __FILE__, __LINE__)

#define ASSERT_NE(actual, expected)                                                            \
  ::testfw::assert_ne((actual), (expected), #actual, #expected, __FILE__, __LINE__)
