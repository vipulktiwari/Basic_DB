// Entry point for the unit-test runner. All TEST()s self-register; run them all.
// Optional first CLI argument filters which tests run (substring of suite.name).
#include "test_framework.h"

int main(int argc, char **argv) { return ::testfw::run_all(argc, argv); }
