#pragma once

#include <cassert>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "fl/context.hpp"
#include "fl/loot/equipment_builder.hpp"
#include "fl/primitives/random_hub.hpp"
#include "fl/loot/weight.hpp"

namespace fl::loot {


using EquipmentFactory = std::function<EquipmentBuilder()>;

struct LootEntry {
  Weight weight;
  EquipmentFactory make;
};

class LootTable {
public:
  LootTable() = default;

  explicit LootTable(std::vector<LootEntry> entries)
      : entries_(std::move(entries)) {
    assert(total_weight() <= 100);
  }

  [[nodiscard]] int total_weight() const noexcept {
    int total = 0;
    for (const auto &entry : entries_) {
      total += entry.weight.value;
    }
    return total;
  }

  [[nodiscard]] bool empty() const noexcept { return entries_.empty(); }

  template <fl::context::WorldCoreCtx Ctx>
  [[nodiscard]] std::optional<EquipmentBuilder>
  roll(Ctx &ctx, std::string_view stream_name) const {
    assert(total_weight() <= 100);

    auto rng = ctx.rng().stream(stream_name);
    const int rolled = rng.template uniform_int<int>(1, 100);

    int cursor = 0;
    for (const auto &entry : entries_) {
      cursor += entry.weight.value;
      if (rolled <= cursor) {
        return entry.make();
      }
    }

    return std::nullopt;
  }

  [[nodiscard]] LootTable augmented_with(LootTable other) const {
    auto combined = entries_;
    combined.insert(combined.end(),
                    std::make_move_iterator(other.entries_.begin()),
                    std::make_move_iterator(other.entries_.end()));
    return LootTable{std::move(combined)};
  }

private:
  std::vector<LootEntry> entries_;
};

} // namespace fl::loot
