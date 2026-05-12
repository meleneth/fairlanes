#pragma once

#include <cassert>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "fl/context.hpp"
#include "fl/loot/equipment_builder.hpp"
#include "fl/primitives/random_hub.hpp"
#include "item_kind.hpp"
#include "special_item.hpp"
#include "upgrade_table.hpp"
#include "tier.hpp"
#include "weight.hpp"
#include "weighted_choice.hpp"
#include "weighted_table.hpp"

namespace fl::loot {

using EquipmentFactory = std::function<EquipmentBuilder()>;

struct LootEntry {
  Weight weight;
  EquipmentFactory make;
};

class LootTable {
public:
  LootTable(Weight drop_chance, WeightedTable<ItemKind> kinds,
            WeightedTable<fl::loot::EquipmentSlot> armor_slots,
            WeightedTable<fl::loot::ArmorKind> armor_kinds,
            WeightedTable<fl::loot::EquipmentSlot> weapon_slots,
            WeightedTable<fl::loot::EquipmentSlot> jewelry_slots,
            UpgradeTable<Tier> tiers, std::vector<SpecialItem> specials = {})
      : drop_chance_(drop_chance), kinds_(std::move(kinds)),
        armor_slots_(std::move(armor_slots)),
        armor_kinds_(std::move(armor_kinds)),
        weapon_slots_(std::move(weapon_slots)),
        jewelry_slots_(std::move(jewelry_slots)), tiers_(std::move(tiers)),
        specials_(std::move(specials)) {}

  template <fl::context::WorldCoreCtx Ctx>
  [[nodiscard]] std::optional<EquipmentBuilder>
  roll(Ctx &ctx, std::string_view stream_name) const {
    auto drop = WeightedTable<bool>{{
        WeightedChoice<bool>{drop_chance_, true},
    }};

    if (!drop.roll(ctx, stream_name).value_or(false)) {
      return std::nullopt;
    }

    auto kind = kinds_.roll(ctx, std::string{stream_name} + ".kind");
    if (!kind) {
      return std::nullopt;
    }

    if (*kind == ItemKind::special) {
      return roll_special(ctx, std::string{stream_name} + ".special");
    }

    auto tier = tiers_.roll(ctx, std::string{stream_name} + ".tier", Tier::worn);

    if (*kind == ItemKind::armor) {
      auto slot = armor_slots_.roll(ctx, std::string{stream_name} + ".slot");
      auto armor_kind =
          armor_kinds_.roll(ctx, std::string{stream_name} + ".armor_kind");

      if (!slot || !armor_kind) {
        return std::nullopt;
      }

      return EquipmentBuilder{
          .slot = *slot,
          .armor_kind = *armor_kind,
          .tier = tier,
          .name = generated_name(*slot, *armor_kind, tier),
      };
    }

    if (*kind == ItemKind::weapon) {
      auto slot = weapon_slots_.roll(ctx, std::string{stream_name} + ".slot");
      auto armor_kind =
          armor_kinds_.roll(ctx, std::string{stream_name} + ".style");

      if (!slot || !armor_kind) {
        return std::nullopt;
      }

      return EquipmentBuilder{
          .slot = *slot,
          .armor_kind = *armor_kind,
          .tier = tier,
          .name = generated_name(*slot, *armor_kind, tier),
      };
    }

    if (*kind == ItemKind::jewelry) {
      auto slot = jewelry_slots_.roll(ctx, std::string{stream_name} + ".slot");
      if (!slot) {
        return std::nullopt;
      }

      return EquipmentBuilder{
          .slot = *slot,
          .armor_kind = fl::loot::ArmorKind::none,
          .tier = tier,
          .name = generated_name(*slot, fl::loot::ArmorKind::none, tier),
      };
    }

    return std::nullopt;
  }

private:
  template <fl::context::WorldCoreCtx Ctx>
  [[nodiscard]] std::optional<EquipmentBuilder>
  roll_special(Ctx &ctx, std::string_view stream_name) const {
    std::vector<WeightedChoice<EquipmentBuilder>> choices;
    choices.reserve(specials_.size());

    for (const auto &special : specials_) {
      choices.push_back({special.weight, special.item});
    }

    return WeightedTable<EquipmentBuilder>{std::move(choices)}.roll(
        ctx, stream_name);
  }

  [[nodiscard]] static std::string generated_name(fl::loot::EquipmentSlot slot,
                                                  fl::loot::ArmorKind kind,
                                                  Tier tier);

  Weight drop_chance_;
  WeightedTable<ItemKind> kinds_;
  WeightedTable<fl::loot::EquipmentSlot> armor_slots_;
  WeightedTable<fl::loot::ArmorKind> armor_kinds_;
  WeightedTable<fl::loot::EquipmentSlot> weapon_slots_;
  WeightedTable<fl::loot::EquipmentSlot> jewelry_slots_;
  UpgradeTable<Tier> tiers_;
  std::vector<SpecialItem> specials_;
};

} // namespace fl::loot
