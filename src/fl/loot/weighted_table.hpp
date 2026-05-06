#pragma once

#include <vector>

namespace fl::loot {

template <typename T>
class WeightedTable {
public:
  explicit WeightedTable(std::vector<WeightedChoice<T>> choices)
      : choices_(std::move(choices)) {
    assert(total_weight() <= 100);
  }

  [[nodiscard]] int total_weight() const noexcept {
    int total = 0;
    for (const auto &choice : choices_) {
      total += choice.weight.value;
    }
    return total;
  }

  template <fl::context::WorldCoreCtx Ctx>
  [[nodiscard]] std::optional<T> roll(Ctx &ctx,
                                      std::string_view stream_name) const {
    assert(total_weight() <= 100);

    auto rng = ctx.rng().stream(stream_name);
    const int rolled = rng.template uniform_int<int>(1, 100);

    int cursor = 0;
    for (const auto &choice : choices_) {
      cursor += choice.weight.value;
      if (rolled <= cursor) {
        return choice.value;
      }
    }

    return std::nullopt;
  }

private:
  std::vector<WeightedChoice<T>> choices_;
};

};
