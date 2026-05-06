#pragma once

#include <vector>

namespace fl::loot {

struct Weight {
  int value;
};

template <typename T>
struct WeightedChoice {
  Weight weight;
  T value;
};

template <typename T>
class WeightedTable {
public:
  explicit WeightedTable(std::vector<WeightedChoice<T>> choices)
      : choices_(std::move(choices)) {
    int total = 0;
    for (const auto &choice : choices_) {
      total += choice.weight.value;
    }
    assert(total <= 100);
  }

private:
  std::vector<WeightedChoice<T>> choices_;
};

};
