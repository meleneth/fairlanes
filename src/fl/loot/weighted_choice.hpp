#pragma once

#include <vector>

namespace fl::loot {

template <typename T>
struct WeightedChoice {
  Weight weight;
  T value;
};

};
