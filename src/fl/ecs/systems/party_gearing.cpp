#include "party_gearing.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include "fl/ecs/components/closet.hpp"
#include "fl/ecs/components/combat_affinity.hpp"
#include "fl/ecs/components/equipment.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/ecs/systems/effective_combat_stats.hpp"
#include "fl/loot/equipment_builder.hpp"
#include "fl/loot/equipment_stat_policy.hpp"
#include "fl/lospec500.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/widgets/equipment_label.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::ecs::systems {
namespace {

using fl::ecs::components::Closet;
using fl::ecs::components::Equipment;
using fl::loot::ArmorKind;
using fl::loot::EquipmentSlot;
using fl::loot::Tier;

constexpr std::array<EquipmentSlot, 8> kArmorSlots{
    EquipmentSlot::chest, EquipmentSlot::helm,   EquipmentSlot::pants,   EquipmentSlot::belt,
    EquipmentSlot::boots, EquipmentSlot::gloves, EquipmentSlot::sleeves, EquipmentSlot::cape,
};

constexpr std::array<EquipmentSlot, 3> kWeaponSlots{
    EquipmentSlot::mainhand,
    EquipmentSlot::offhand,
    EquipmentSlot::knife,
};

constexpr std::array<EquipmentSlot, 3> kJewelrySlots{
    EquipmentSlot::necklace,
    EquipmentSlot::ring_1,
    EquipmentSlot::ring_2,
};

constexpr std::array<EquipmentSlot, 14> kUpgradableSlots{
    EquipmentSlot::chest,   EquipmentSlot::helm,   EquipmentSlot::pants,
    EquipmentSlot::belt,    EquipmentSlot::boots,  EquipmentSlot::gloves,
    EquipmentSlot::sleeves, EquipmentSlot::cape,   EquipmentSlot::mainhand,
    EquipmentSlot::offhand, EquipmentSlot::knife,  EquipmentSlot::necklace,
    EquipmentSlot::ring_1,  EquipmentSlot::ring_2,
};

constexpr std::array<ArmorKind, 4> kUpgradableKinds{
    ArmorKind::none,
    ArmorKind::cloth,
    ArmorKind::leather,
    ArmorKind::plate,
};

entt::entity& slot_ref(Closet& closet, EquipmentSlot slot) {
  return closet.equipped_item(slot);
}

entt::entity slot_value(const Closet& closet, EquipmentSlot slot) {
  return closet.equipped_item(slot);
}

int tier_rank(Tier tier) {
  switch (tier) {
  case Tier::worn: return 0;
  case Tier::plain: return 1;
  case Tier::sturdy: return 2;
  case Tier::fine: return 3;
  case Tier::excellent: return 4;
  case Tier::masterwork: return 5;
  case Tier::mythic: return 6;
  }

  return 0;
}

std::optional<Tier> next_tier(Tier tier) {
  switch (tier) {
  case Tier::worn: return Tier::plain;
  case Tier::plain: return Tier::sturdy;
  case Tier::sturdy: return Tier::fine;
  case Tier::fine: return Tier::excellent;
  case Tier::excellent: return Tier::masterwork;
  case Tier::masterwork: return Tier::mythic;
  case Tier::mythic: return std::nullopt;
  }

  return std::nullopt;
}

std::string_view armor_kind_name(ArmorKind kind) {
  switch (kind) {
  case ArmorKind::cloth: return "Cloth";
  case ArmorKind::leather: return "Leather";
  case ArmorKind::plate: return "Plate";
  case ArmorKind::none: return "";
  }

  return "";
}

std::string_view tier_name(Tier tier) {
  switch (tier) {
  case Tier::worn: return "Worn";
  case Tier::plain: return "Plain";
  case Tier::sturdy: return "Sturdy";
  case Tier::fine: return "Fine";
  case Tier::excellent: return "Excellent";
  case Tier::masterwork: return "Masterwork";
  case Tier::mythic: return "Mythic";
  }

  return "Odd";
}

std::string_view slot_name(EquipmentSlot slot) {
  switch (slot) {
  case EquipmentSlot::chest: return "Chestpiece";
  case EquipmentSlot::helm: return "Helm";
  case EquipmentSlot::pants: return "Pants";
  case EquipmentSlot::belt: return "Belt";
  case EquipmentSlot::boots: return "Boots";
  case EquipmentSlot::gloves: return "Gloves";
  case EquipmentSlot::sleeves: return "Sleeves";
  case EquipmentSlot::cape: return "Cape";
  case EquipmentSlot::necklace: return "Necklace";
  case EquipmentSlot::ring_1:
  case EquipmentSlot::ring_2: return "Ring";
  case EquipmentSlot::mainhand: return "Mainhand";
  case EquipmentSlot::offhand: return "Offhand";
  case EquipmentSlot::knife: return "Knife";
  }

  return "Equipment";
}

std::string generated_name(EquipmentSlot slot, ArmorKind kind, Tier tier) {
  if (kind == ArmorKind::none) {
    return fmt::format("{} {}", tier_name(tier), slot_name(slot));
  }

  return fmt::format("{} {} {}", tier_name(tier), armor_kind_name(kind), slot_name(slot));
}

bool is_special(const Equipment& equipment) {
  return equipment.is_unique();
}

bool is_better(const Equipment* candidate, const Equipment* current) {
  if (candidate == nullptr) {
    return false;
  }
  if (current == nullptr) {
    return true;
  }
  const int candidate_value = fl::loot::equipment_combat_value(*candidate);
  const int current_value = fl::loot::equipment_combat_value(*current);
  if (candidate_value < current_value) return false;
  if (candidate_value > current_value) {
    if (current_value == 0) return true;
    const auto& candidate_resistance = candidate->resistances();
    const auto& current_resistance = current->resistances();
    return candidate->attack() >= current->attack() && candidate->armor() >= current->armor() &&
           candidate->max_hp() >= current->max_hp() &&
           candidate_resistance.physical >= current_resistance.physical &&
           candidate_resistance.magical >= current_resistance.magical &&
           candidate_resistance.fire >= current_resistance.fire &&
           candidate_resistance.ice >= current_resistance.ice &&
           candidate_resistance.lightning >= current_resistance.lightning;
  }
  const auto& candidate_resistance = candidate->resistances();
  const auto& current_resistance = current->resistances();
  const bool same_modifiers = candidate->attack() == current->attack() &&
                              candidate->armor() == current->armor() &&
                              candidate->max_hp() == current->max_hp() &&
                              candidate_resistance.physical == current_resistance.physical &&
                              candidate_resistance.magical == current_resistance.magical &&
                              candidate_resistance.fire == current_resistance.fire &&
                              candidate_resistance.ice == current_resistance.ice &&
                              candidate_resistance.lightning == current_resistance.lightning;
  return same_modifiers && tier_rank(candidate->tier()) > tier_rank(current->tier());
}

void append_equipped_log(fl::context::PartyCtx& ctx, std::string_view member,
                         const Equipment& equipment, EquipmentSlot slot) {
  ctx.log().append(ftxui::hbox({
      ftxui::text(std::string{member}) | fl::lospec500::at(22),
      ftxui::text(" equipped ") | fl::lospec500::at(19),
      fl::widgets::equipment_name_label(equipment),
      ftxui::text(" in ") | fl::lospec500::at(19),
      ftxui::text(std::string{fl::ecs::components::to_string(slot)}) | ftxui::dim,
      ftxui::text(".") | fl::lospec500::at(19),
  }));
}

void append_upgraded_log(fl::context::PartyCtx& ctx, Tier tier, ArmorKind kind, EquipmentSlot slot,
                         const Equipment& equipment) {
  auto source_name = generated_name(slot, kind, tier);
  ctx.log().append(ftxui::hbox({
      ftxui::text("Upgraded 3 loose ") | fl::lospec500::at(19),
      ftxui::text(source_name) | ftxui::dim,
      ftxui::text(" into ") | fl::lospec500::at(19),
      fl::widgets::equipment_name_label(equipment),
      ftxui::text(".") | fl::lospec500::at(19),
  }));
}

std::optional<ArmorKind> existing_armor_kind(entt::registry& reg, const Closet& closet) {
  for (auto slot : kArmorSlots) {
    const auto worn = slot_value(closet, slot);
    if (worn == entt::null || !reg.valid(worn)) {
      continue;
    }

    const auto* equipment = reg.try_get<Equipment>(worn);
    if (equipment != nullptr && equipment->is_armor()) {
      return equipment->armor_kind();
    }
  }

  return std::nullopt;
}

ArmorKind armor_kind_for(fl::primitives::CombatAffinity affinity) {
  switch (affinity) {
  case fl::primitives::CombatAffinity::Cloth: return ArmorKind::cloth;
  case fl::primitives::CombatAffinity::Leather: return ArmorKind::leather;
  case fl::primitives::CombatAffinity::Plate: return ArmorKind::plate;
  case fl::primitives::CombatAffinity::Neutral: return ArmorKind::none;
  }
  return ArmorKind::none;
}

std::optional<ArmorKind> best_available_armor_kind(entt::registry& reg,
                                                   const std::vector<entt::entity>& inventory,
                                                   int character_level) {
  struct Score {
    ArmorKind kind;
    int pieces{0};
    int tiers{0};
  };

  std::array<Score, 3> scores{{
      {ArmorKind::cloth, 0, 0},
      {ArmorKind::leather, 0, 0},
      {ArmorKind::plate, 0, 0},
  }};

  for (auto item : inventory) {
    const auto* equipment = reg.try_get<Equipment>(item);
    if (equipment == nullptr || !equipment->is_armor() ||
        !fl::loot::can_equip(*equipment, character_level)) {
      continue;
    }

    auto iter = std::find_if(scores.begin(), scores.end(),
                             [&](Score score) { return score.kind == equipment->armor_kind(); });
    if (iter != scores.end()) {
      ++iter->pieces;
      iter->tiers += tier_rank(equipment->tier());
    }
  }

  const auto best = std::max_element(scores.begin(), scores.end(), [](Score lhs, Score rhs) {
    if (lhs.pieces != rhs.pieces) {
      return lhs.pieces < rhs.pieces;
    }
    return lhs.tiers < rhs.tiers;
  });

  if (best == scores.end() || best->pieces == 0) {
    return std::nullopt;
  }

  return best->kind;
}

std::optional<std::size_t> find_best_inventory_item(entt::registry& reg,
                                                    const std::vector<entt::entity>& inventory,
                                                    EquipmentSlot slot, ArmorKind kind,
                                                    const Equipment* current, int character_level,
                                                    bool match_kind = true) {
  std::optional<std::size_t> best_index;
  const Equipment* best_equipment = nullptr;

  for (std::size_t i = 0; i < inventory.size(); ++i) {
    const auto* equipment = reg.try_get<Equipment>(inventory[i]);
    if (equipment == nullptr || equipment->slot() != slot ||
        (match_kind && equipment->armor_kind() != kind)) {
      continue;
    }
    if (!fl::loot::can_equip(*equipment, character_level)) continue;

    if (!is_better(equipment, current)) {
      continue;
    }

    if (best_equipment == nullptr || is_better(equipment, best_equipment)) {
      best_index = i;
      best_equipment = equipment;
    }
  }

  return best_index;
}

void equip_party(fl::context::PartyCtx& ctx, std::vector<entt::entity>& inventory) {
  auto& reg = ctx.reg();

  for (const auto& member : ctx.party_data().members()) {
    auto* party_member = reg.try_get<fl::ecs::components::PartyMember>(member.member_id());
    if (party_member == nullptr) {
      continue;
    }

    auto& closet = party_member->closet();
    const auto* xp = reg.try_get<fl::ecs::components::TrackXP>(member.member_id());
    const int character_level = xp == nullptr ? 1 : xp->level_;
    auto preferred_kind = std::optional<ArmorKind>{};
    if (const auto* affinity =
            reg.try_get<fl::ecs::components::ActiveCombatAffinity>(member.member_id()))
      preferred_kind = armor_kind_for(affinity->value);
    if (!preferred_kind) preferred_kind = existing_armor_kind(reg, closet);
    if (!preferred_kind) {
      preferred_kind = best_available_armor_kind(reg, inventory, character_level);
    }

    if (preferred_kind) {
      for (auto slot : kArmorSlots) {
        auto& worn = slot_ref(closet, slot);
        const auto* current = worn == entt::null ? nullptr : reg.try_get<Equipment>(worn);
        const auto* comparable =
            current != nullptr && current->armor_kind() == *preferred_kind ? current : nullptr;
        auto best_index = find_best_inventory_item(reg, inventory, slot, *preferred_kind,
                                                   comparable, character_level);
        if (!best_index) {
          continue;
        }

        const auto next_item = inventory[*best_index];
        const auto& next_equipment = reg.get<Equipment>(next_item);
        inventory.erase(inventory.begin() + static_cast<std::ptrdiff_t>(*best_index));
        if (worn != entt::null) {
          inventory.push_back(worn);
        }
        worn = next_item;

        append_equipped_log(ctx, member.name(), next_equipment, slot);
      }
    }

    for (auto slot : kWeaponSlots) {
      auto& worn = slot_ref(closet, slot);
      const auto* current = worn == entt::null ? nullptr : reg.try_get<Equipment>(worn);
      auto best_index = find_best_inventory_item(reg, inventory, slot, ArmorKind::none, current,
                                                 character_level, false);
      if (!best_index) continue;

      const auto next_item = inventory[*best_index];
      const auto& next_equipment = reg.get<Equipment>(next_item);
      inventory.erase(inventory.begin() + static_cast<std::ptrdiff_t>(*best_index));
      if (worn != entt::null) inventory.push_back(worn);
      worn = next_item;
      append_equipped_log(ctx, member.name(), next_equipment, slot);
    }

    for (auto slot : kJewelrySlots) {
      auto& worn = slot_ref(closet, slot);
      const auto* current = worn == entt::null ? nullptr : reg.try_get<Equipment>(worn);
      auto best_index =
          find_best_inventory_item(reg, inventory, slot, ArmorKind::none, current, character_level);
      if (!best_index) {
        continue;
      }

      const auto next_item = inventory[*best_index];
      const auto& next_equipment = reg.get<Equipment>(next_item);
      inventory.erase(inventory.begin() + static_cast<std::ptrdiff_t>(*best_index));
      if (worn != entt::null) {
        inventory.push_back(worn);
      }
      worn = next_item;

      append_equipped_log(ctx, member.name(), next_equipment, slot);
    }
    auto& stats = reg.get<fl::ecs::components::Stats>(member.member_id());
    stats.hp_ = std::min(stats.hp_,
                         fl::ecs::systems::effective_combat_stats(reg, member.member_id()).max_hp);
  }
}

void remove_equipped_items_from_inventory(fl::context::PartyCtx& ctx,
                                          std::vector<entt::entity>& inventory) {
  auto& reg = ctx.reg();
  std::vector<entt::entity> equipped;

  for (const auto& member : ctx.party_data().members()) {
    auto* party_member = reg.try_get<fl::ecs::components::PartyMember>(member.member_id());
    if (party_member == nullptr) {
      continue;
    }

    const auto& closet = party_member->closet();
    for (auto slot : kUpgradableSlots) {
      const auto item = slot_value(closet, slot);
      if (item != entt::null) {
        equipped.push_back(item);
      }
    }
  }

  inventory.erase(std::remove_if(inventory.begin(), inventory.end(),
                                 [&](entt::entity item) {
                                   return std::find(equipped.begin(), equipped.end(), item) !=
                                          equipped.end();
                                 }),
                  inventory.end());
}

void upgrade_inventory(fl::context::PartyCtx& ctx, std::vector<entt::entity>& inventory) {
  auto& reg = ctx.reg();
  bool upgraded = true;

  while (upgraded) {
    upgraded = false;

    for (auto slot : kUpgradableSlots) {
      for (auto kind : kUpgradableKinds) {
        for (auto tier : {Tier::worn, Tier::plain, Tier::sturdy, Tier::fine, Tier::excellent,
                          Tier::masterwork}) {
          auto next = next_tier(tier);
          if (!next) {
            continue;
          }

          std::optional<std::array<std::size_t, 3>> match;
          for (std::size_t i = 0; i < inventory.size(); ++i) {
            const auto* equipment = reg.try_get<Equipment>(inventory[i]);
            if (equipment == nullptr || is_special(*equipment) || equipment->slot() != slot ||
                equipment->armor_kind() != kind || equipment->tier() != tier)
              continue;
            std::array<std::size_t, 3> candidates{i, 0, 0};
            std::size_t count = 1;
            for (std::size_t j = i + 1; j < inventory.size() && count < 3; ++j) {
              const auto* other = reg.try_get<Equipment>(inventory[j]);
              if (other != nullptr && !is_special(*other) && other->slot() == slot &&
                  other->armor_kind() == kind && other->tier() == tier &&
                  other->biome_id() == equipment->biome_id())
                candidates[count++] = j;
            }
            if (count == 3) {
              match = candidates;
              break;
            }
          }

          if (match) {
            auto consumed = *match;

            const auto first = reg.get<Equipment>(inventory[consumed[0]]);
            const auto second = reg.get<Equipment>(inventory[consumed[1]]);
            const auto third = reg.get<Equipment>(inventory[consumed[2]]);
            auto upgraded_builder = fl::loot::upgraded_equipment_builder(
                first, second, third, *next, ctx.rng(),
                fmt::format("equipment.upgrade.{}.{}.{}", static_cast<int>(slot),
                            static_cast<int>(tier), first.biome_id()));
            upgraded_builder.name = generated_name(slot, kind, *next);

            std::sort(consumed.begin(), consumed.end(), std::greater<>{});
            for (auto index : consumed) {
              reg.destroy(inventory[index]);
              inventory.erase(inventory.begin() + static_cast<std::ptrdiff_t>(index));
            }

            auto upgraded_item = upgraded_builder.create(reg);
            inventory.push_back(upgraded_item);
            upgraded = true;

            const auto& equipment = reg.get<Equipment>(upgraded_item);
            append_upgraded_log(ctx, tier, kind, slot, equipment);
          }
        }
      }
    }
  }
}

} // namespace

void PartyGearing::commit(fl::context::PartyCtx& ctx) {
  auto inventory =
      std::vector<entt::entity>{ctx.party_data().items().begin(), ctx.party_data().items().end()};

  equip_party(ctx, inventory);
  remove_equipped_items_from_inventory(ctx, inventory);
  upgrade_inventory(ctx, inventory);

  ctx.party_data().replace_items(std::move(inventory));
}

} // namespace fl::ecs::systems
