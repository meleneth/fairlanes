#pragma once

#include <cassert>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "fl/context.hpp"
#include "fl/generated/gear_content.hpp"
#include "fl/loot/equipment_builder.hpp"
#include "fl/loot/equipment_stat_policy.hpp"
#include "fl/primitives/random_hub.hpp"
#include "item_kind.hpp"
#include "special_item.hpp"
#include "tier.hpp"
#include "upgrade_table.hpp"
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
            WeightedTable<fl::loot::EquipmentSlot> jewelry_slots, UpgradeTable<Tier> tiers,
            std::vector<SpecialItem> specials = {})
      : drop_chance_(drop_chance), kinds_(std::move(kinds)), armor_slots_(std::move(armor_slots)),
        armor_kinds_(std::move(armor_kinds)), weapon_slots_(std::move(weapon_slots)),
        jewelry_slots_(std::move(jewelry_slots)), tiers_(std::move(tiers)),
        specials_(std::move(specials)) {}

  [[nodiscard]] std::span<const SpecialItem> special_items() const noexcept {
    return specials_;
  }

  template <fl::context::WorldCoreCtx Ctx>
  [[nodiscard]] std::optional<EquipmentBuilder> roll(Ctx& ctx, std::string_view stream_name,
                                                     std::string_view biome_id = {}) const {
    auto drop = WeightedTable<bool>{{
        WeightedChoice<bool>{drop_chance_, true},
    }};

    if (!drop.roll(ctx, stream_name).value_or(false)) {
      return std::nullopt;
    }

    const auto* profile = generated_content::equipment_profile(biome_id);
    std::optional<ItemKind> kind;
    if (profile != nullptr) {
      kind = WeightedTable<ItemKind>{
          {
              {Weight{profile->kinds.armor}, ItemKind::armor},
              {Weight{profile->kinds.weapon}, ItemKind::weapon},
              {Weight{profile->kinds.jewelry}, ItemKind::jewelry},
              {Weight{5}, ItemKind::special},
          }}.roll(ctx, std::string{stream_name} + ".kind");
    } else {
      kind = kinds_.roll(ctx, std::string{stream_name} + ".kind");
    }
    if (!kind) {
      return std::nullopt;
    }

    if (*kind == ItemKind::special) {
      return roll_special(ctx, std::string{stream_name} + ".special");
    }

    auto tier = tiers_.roll(ctx, std::string{stream_name} + ".tier", Tier::worn);

    if (*kind == ItemKind::armor) {
      auto slot = armor_slots_.roll(ctx, std::string{stream_name} + ".slot");
      std::optional<fl::loot::ArmorKind> armor_kind;
      if (profile != nullptr) {
        armor_kind = WeightedTable<fl::loot::ArmorKind>{
            {
                {Weight{profile->armor_kinds.cloth}, fl::loot::ArmorKind::cloth},
                {Weight{profile->armor_kinds.leather}, fl::loot::ArmorKind::leather},
                {Weight{profile->armor_kinds.plate}, fl::loot::ArmorKind::plate},
            }}.roll(ctx, std::string{stream_name} + ".armor_kind");
      } else {
        armor_kind = armor_kinds_.roll(ctx, std::string{stream_name} + ".armor_kind");
      }

      if (!slot || !armor_kind) {
        return std::nullopt;
      }

      EquipmentBuilder result{
          .slot = *slot,
          .armor_kind = *armor_kind,
          .tier = tier,
          .name = generated_name(*slot, *armor_kind, tier),
          .unique_id = {},
      };
      apply_ordinary_equipment_stats(result, ctx.rng(), biome_id,
                                     std::string{stream_name} + ".stats");
      return result;
    }

    if (*kind == ItemKind::weapon) {
      auto slot = weapon_slots_.roll(ctx, std::string{stream_name} + ".slot");
      auto armor_kind = armor_kinds_.roll(ctx, std::string{stream_name} + ".style");

      if (!slot || !armor_kind) {
        return std::nullopt;
      }

      EquipmentBuilder result{
          .slot = *slot,
          .armor_kind = *armor_kind,
          .tier = tier,
          .name = generated_name(*slot, *armor_kind, tier),
          .unique_id = {},
      };
      apply_ordinary_equipment_stats(result, ctx.rng(), biome_id,
                                     std::string{stream_name} + ".stats");
      return result;
    }

    if (*kind == ItemKind::jewelry) {
      auto slot = jewelry_slots_.roll(ctx, std::string{stream_name} + ".slot");
      if (!slot) {
        return std::nullopt;
      }

      EquipmentBuilder result{
          .slot = *slot,
          .armor_kind = fl::loot::ArmorKind::none,
          .tier = tier,
          .name = generated_name(*slot, fl::loot::ArmorKind::none, tier),
          .unique_id = {},
      };
      apply_ordinary_equipment_stats(result, ctx.rng(), biome_id,
                                     std::string{stream_name} + ".stats");
      return result;
    }

    return std::nullopt;
  }

private:
  template <fl::context::WorldCoreCtx Ctx>
  [[nodiscard]] std::optional<EquipmentBuilder> roll_special(Ctx& ctx,
                                                             std::string_view stream_name) const {
    std::vector<WeightedChoice<EquipmentBuilder>> choices;
    choices.reserve(specials_.size());

    for (const auto& special : specials_) {
      choices.push_back({special.weight, special.item});
    }

    return WeightedTable<EquipmentBuilder>{std::move(choices)}.roll(ctx, stream_name);
  }

  [[nodiscard]] static std::string generated_name(fl::loot::EquipmentSlot slot,
                                                  fl::loot::ArmorKind kind, Tier tier);

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
