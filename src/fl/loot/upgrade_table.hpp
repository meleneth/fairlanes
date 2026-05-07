#pragma once

#include <cassert>
#include <optional>
#include <string_view>
#include <vector>

#include "fl/context.hpp"
#include "fl/loot/weighted_choice.hpp"

namespace fl::loot {

template <typename T> class UpgradeTable {
public:
  explicit UpgradeTable(std::vector<WeightedChoice<T>> choices)
      : choices_(std::move(choices)) {
  
  }

  template <fl::context::WorldCoreCtx Ctx>
  [[nodiscard]] T roll(Ctx &ctx, std::string_view stream_name,
                       T initial) const {
    auto rng = ctx.rng().stream(stream_name);

    T current = initial;

    for (const auto &choice : choices_) {
      const int rolled = rng.template uniform_int<int>(1, 100);

      if (rolled > choice.weight.value) {
        break;
      }

      current = choice.value;
    }

    return current;
  }

private:
  std::vector<WeightedChoice<T>> choices_;
};

} // namespace fl::loot
