#include <iostream>

#include "common/types.h"

int main() {
  std::cout << "Basic_DB v0.1.0 (phase 0 scaffold)\n"
            << "  page size: " << basicdb::PAGE_SIZE << " bytes\n"
            << "  see DESIGN.md for the roadmap\n";
  return 0;
}
