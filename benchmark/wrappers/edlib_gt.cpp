#include <iostream>
#include <string>

#include "edlib.h"

int main() {
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);

  std::string pair_id;
  std::string read;
  std::string reference;
  while (std::cin >> pair_id >> read >> reference) {
    EdlibAlignConfig config =
        edlibNewAlignConfig(-1, EDLIB_MODE_NW, EDLIB_TASK_DISTANCE, nullptr, 0);
    EdlibAlignResult result =
        edlibAlign(read.c_str(), static_cast<int>(read.size()), reference.c_str(),
                   static_cast<int>(reference.size()), config);
    if (result.status != EDLIB_STATUS_OK) {
      std::cerr << "edlib failed for " << pair_id << '\n';
      edlibFreeAlignResult(result);
      return 1;
    }
    std::cout << pair_id << '\t' << result.editDistance << '\n';
    edlibFreeAlignResult(result);
  }
  return 0;
}
