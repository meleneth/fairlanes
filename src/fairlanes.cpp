#include <iostream>

#include <ftxui/component/screen_interactive.hpp>

#include "fl/grand_central.hpp"

int main() {
  using fl::GrandCentral;

  GrandCentral gc{1, 1};
  gc.bootstrap_logs();

  std::cout << "You are a young hero, and you live in a village";

  return 0;
}
