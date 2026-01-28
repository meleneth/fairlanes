#include <iostream>

#include <ftxui/component/screen_interactive.hpp>

#include "fl/grand_central.hpp"

int main() {
  using fl::GrandCentral;

  GrandCentral gc{8, 5, 5};
  // GrandCentral gc{1, 1, 1};

  std::cout << "You are a young hero, and you live in a village";

  gc.main_loop();

  return 0;
}
