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
#include "fl/ecs/components/equipment.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/loot/equipment_builder.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::ecs::systems {
namespace {

using fl::ecs::components::Closet;
using fl::ecs::components::Equipment;
using fl::loot::ArmorKind;
using fl::loot::EquipmentSlot;
using fl::loot::Tier;

constexpr std::array<EquipmentSlot, 8> kArmorSlots{
    EquipmentSlot::chest,  EquipmentSlot::helm,    EquipmentSlot::pants,
    EquipmentSlot::belt,   EquipmentSlot::boots,   EquipmentSlot::gloves,
    EquipmentSlot::sleeves, EquipmentSlot::cape,
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
    EquipmentSlot::chest,    EquipmentSlot::helm,     EquipmentSlot::pants,
    EquipmentSlot::belt,     EquipmentSlot::boots,    EquipmentSlot::gloves,
    EquipmentSlot::sleeves,  EquipmentSlot::cape,     EquipmentSlot::mainhand,
    EquipmentSlot::offhand,  EquipmentSlot::knife,    EquipmentSlot::necklace,
    EquipmentSlot::ring_1,   EquipmentSlot::ring_2,
};

constexpr std::array<ArmorKind, 4> kUpgradableKinds{
    ArmorKind::none,
    ArmorKind::cloth,
    ArmorKind::leather,
    ArmorKind::plate,
};

entt::entity &slot_ref(Closet &closet, EquipmentSlot slot) {
  switch (slot) {
  case EquipmentSlot::chest:
    return closet.chest;
  case EquipmentSlot::helm:
    return closet.helm;
  case EquipmentSlot::pants:
    return closet.pants;
  case EquipmentSlot::belt:
    return closet.belt;
  case EquipmentSlot::boots:
    return closet.boots;
  case EquipmentSlot::gloves:
    return closet.gloves;
  case EquipmentSlot::sleeves:
    return closet.sleeves;
  case EquipmentSlot::cape:
    return closet.cape;
  case EquipmentSlot::necklace:
    return closet.necklace;
  case EquipmentSlot::ring_1:
    return closet.ring_1;
  case EquipmentSlot::ring_2:
    return closet.ring_2;
  case EquipmentSlot::mainhand:
    return closet.mainhand;
  case EquipmentSlot::offhand:
    return closet.offhand;
  case EquipmentSlot::knife:
    return closet.knife;
  }

  return closet.chest;
}

entt::entity slot_value(const Closet &closet, EquipmentSlot slot) {
  switch (slot) {
  case EquipmentSlot::chest:
    return closet.chest;
  case EquipmentSlot::helm:
    return closet.helm;
  case EquipmentSlot::pants:
    return closet.pants;
  case EquipmentSlot::belt:
    return closet.belt;
  case EquipmentSlot::boots:
    return closet.boots;
  case EquipmentSlot::gloves:
    return closet.gloves;
  case EquipmentSlot::sleeves:
    return closet.sleeves;
  case EquipmentSlot::cape:
    return closet.cape;
  case EquipmentSlot::necklace:
    return closet.necklace;
  case EquipmentSlot::ring_1:
    return closet.ring_1;
  case EquipmentSlot::ring_2:
    return closet.ring_2;
  case EquipmentSlot::mainhand:
    return closet.mainhand;
  case EquipmentSlot::offhand:
    return closet.offhand;
  case EquipmentSlot::knife:
    return closet.knife;
  }

  return entt::null;
}

int tier_rank(Tier tier) {
  switch (tier) {
  case Tier::worn:
    return 0;
  case Tier::plain:
    return 1;
  case Tier::sturdy:
    return 2;
  case Tier::fine:
    return 3;
  case Tier::excellent:
    return 4;
  case Tier::masterwork:
    return 5;
  case Tier::mythic:
    return 6;
  }

  return 0;
}

std::optional<Tier> next_tier(Tier tier) {
  switch (tier) {
  case Tier::worn:
    return Tier::plain;
  case Tier::plain:
    return Tier::sturdy;
  case Tier::sturdy:
    return Tier::fine;
  case Tier::fine:
    return Tier::excellent;
  case Tier::excellent:
    return Tier::masterwork;
  case Tier::masterwork:
    return Tier::mythic;
  case Tier::mythic:
    return std::nullopt;
  }

  return std::nullopt;
}

std::string_view armor_kind_name(ArmorKind kind) {
  switch (kind) {
  case ArmorKind::cloth:
    return "Cloth";
  case ArmorKind::leather:
    return "Leather";
  case ArmorKind::plate:
    return "Plate";
  case ArmorKind::none:
    return "";
  }

  return "";
}

std::string_view tier_name(Tier tier) {
  switch (tier) {
  case Tier::worn:
    return "Worn";
  case Tier::plain:
    return "Plain";
  case Tier::sturdy:
    return "Sturdy";
  case Tier::fine:
    return "Fine";
  case Tier::excellent:
    return "Excellent";
  case Tier::masterwork:
    return "Masterwork";
  case Tier::mythic:
    return "Mythic";
  }

  return "Odd";
}

std::string_view slot_name(EquipmentSlot slot) {
  switch (slot) {
  case EquipmentSlot::chest:
    return "Chestpiece";
  case EquipmentSlot::helm:
    return "Helm";
  case EquipmentSlot::pants:
    return "Pants";
  case EquipmentSlot::belt:
    return "Belt";
  case EquipmentSlot::boots:
    return "Boots";
  case EquipmentSlot::gloves:
    return "Gloves";
  case EquipmentSlot::sleeves:
    return "Sleeves";
  case EquipmentSlot::cape:
    return "Cape";
  case EquipmentSlot::necklace:
    return "Necklace";
  case EquipmentSlot::ring_1:
  case EquipmentSlot::ring_2:
    return "Ring";
  case EquipmentSlot::mainhand:
    return "Mainhand";
  case EquipmentSlot::offhand:
    return "Offhand";
  case EquipmentSlot::knife:
    return "Knife";
  }

  return "Equipment";
}

std::string generated_name(EquipmentSlot slot, ArmorKind kind, Tier tier) {
  return fmt::format("{} {} {}", tier_name(tier), armor_kind_name(kind),
                     slot_name(slot));
}

bool is_special(const Equipment &equipment) {
  const auto name = equipment.name();
  return name == "Boots of Damp Authority" || name == "Mouse-Nibbled Cape";
}

bool is_better(const Equipment *candidate, const Equipment *current) {
  if (candidate == nullptr) {
    return false;
  }
  if (current == nullptr) {
    return true;
  }
  return tier_rank(candidate->tier()) > tier_rank(current->tier());
}

std::optional<ArmorKind>
existing_armor_kind(entt::registry &reg, const Closet &closet) {
  for (auto slot : kArmorSlots) {
    const auto worn = slot_value(closet, slot);
    if (worn == entt::null || !reg.valid(worn)) {
      continue;
    }

    const auto *equipment = reg.try_get<Equipment>(worn);
    if (equipment != nullptr && equipment->is_armor()) {
      return equipment->armor_kind();
    }
  }

  return std::nullopt;
}

std::optional<ArmorKind>
best_available_armor_kind(entt::registry &reg,
                          const std::vector<entt::entity> &inventory) {
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
    const auto *equipment = reg.try_get<Equipment>(item);
    if (equipment == nullptr || !equipment->is_armor()) {
      continue;
    }

    auto iter = std::find_if(scores.begin(), scores.end(), [&](Score score) {
      return score.kind == equipment->armor_kind();
    });
    if (iter != scores.end()) {
      ++iter->pieces;
      iter->tiers += tier_rank(equipment->tier());
    }
  }

  const auto best = std::max_element(scores.begin(), scores.end(),
                                     [](Score lhs, Score rhs) {
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

std::optional<std::size_t>
find_best_inventory_item(entt::registry &reg,
                         const std::vector<entt::entity> &inventory,
                         EquipmentSlot slot, ArmorKind kind,
                         const Equipment *current) {
  std::optional<std::size_t> best_index;
  const Equipment *best_equipment = nullptr;

  for (std::size_t i = 0; i < inventory.size(); ++i) {
    const auto *equipment = reg.try_get<Equipment>(inventory[i]);
    if (equipment == nullptr || equipment->slot() != slot ||
        equipment->armor_kind() != kind) {
      continue;
    }

    if (!is_better(equipment, current)) {
      continue;
    }

    if (best_equipment == nullptr ||
        tier_rank(equipment->tier()) > tier_rank(best_equipment->tier())) {
      best_index = i;
      best_equipment = equipment;
    }
  }

  return best_index;
}

void equip_party(fl::context::PartyCtx &ctx,
                 std::vector<entt::entity> &inventory) {
  auto &reg = ctx.reg();

  for (const auto &member : ctx.party_data().members()) {
    auto *party_member =
        reg.try_get<fl::ecs::components::PartyMember>(member.member_id());
    if (party_member == nullptr) {
      continue;
    }

    auto &closet = party_member->closet();
    auto preferred_kind = existing_armor_kind(reg, closet);
    if (!preferred_kind) {
      preferred_kind = best_available_armor_kind(reg, inventory);
    }

    if (preferred_kind) {
      for (auto slot : kArmorSlots) {
        auto &worn = slot_ref(closet, slot);
        const auto *current =
            worn == entt::null ? nullptr : reg.try_get<Equipment>(worn);
        auto best_index =
            find_best_inventory_item(reg, inventory, slot, *preferred_kind,
                                     current);
        if (!best_index) {
          continue;
        }

        const auto next_item = inventory[*best_index];
        const auto &next_equipment = reg.get<Equipment>(next_item);
        inventory.erase(inventory.begin() +
                        static_cast<std::ptrdiff_t>(*best_index));
        if (worn != entt::null) {
          inventory.push_back(worn);
        }
        worn = next_item;

        ctx.log().append_markup(fmt::format(
            "[player_name]({}) equipped [item]({}) in {}.", member.name(),
            next_equipment.name(), fl::ecs::components::to_string(slot)));
      }

      for (auto slot : kWeaponSlots) {
        auto &worn = slot_ref(closet, slot);
        const auto *current =
            worn == entt::null ? nullptr : reg.try_get<Equipment>(worn);
        auto best_index =
            find_best_inventory_item(reg, inventory, slot, *preferred_kind,
                                     current);
        if (!best_index) {
          continue;
        }

        const auto next_item = inventory[*best_index];
        const auto &next_equipment = reg.get<Equipment>(next_item);
        inventory.erase(inventory.begin() +
                        static_cast<std::ptrdiff_t>(*best_index));
        if (worn != entt::null) {
          inventory.push_back(worn);
        }
        worn = next_item;

        ctx.log().append_markup(fmt::format(
            "[player_name]({}) equipped [item]({}) in {}.", member.name(),
            next_equipment.name(), fl::ecs::components::to_string(slot)));
      }
    }

    for (auto slot : kJewelrySlots) {
      auto &worn = slot_ref(closet, slot);
      const auto *current =
          worn == entt::null ? nullptr : reg.try_get<Equipment>(worn);
      auto best_index = find_best_inventory_item(
          reg, inventory, slot, ArmorKind::none, current);
      if (!best_index) {
        continue;
      }

      const auto next_item = inventory[*best_index];
      const auto &next_equipment = reg.get<Equipment>(next_item);
      inventory.erase(inventory.begin() +
                      static_cast<std::ptrdiff_t>(*best_index));
      if (worn != entt::null) {
        inventory.push_back(worn);
      }
      worn = next_item;

      ctx.log().append_markup(fmt::format(
          "[player_name]({}) equipped [item]({}) in {}.", member.name(),
          next_equipment.name(), fl::ecs::components::to_string(slot)));
    }
  }
}

void upgrade_inventory(fl::context::PartyCtx &ctx,
                       std::vector<entt::entity> &inventory) {
  auto &reg = ctx.reg();
  bool upgraded = true;

  while (upgraded) {
    upgraded = false;

    for (auto slot : kUpgradableSlots) {
      for (auto kind : kUpgradableKinds) {
        for (auto tier : {Tier::worn, Tier::plain, Tier::sturdy, Tier::fine,
                          Tier::excellent, Tier::masterwork}) {
          auto next = next_tier(tier);
          if (!next) {
            continue;
          }

          std::vector<std::size_t> matches;
          for (std::size_t i = 0; i < inventory.size(); ++i) {
            const auto *equipment = reg.try_get<Equipment>(inventory[i]);
            if (equipment != nullptr && !is_special(*equipment) &&
                equipment->slot() == slot && equipment->armor_kind() == kind &&
                equipment->tier() == tier) {
              matches.push_back(i);
            }
          }

          while (matches.size() >= 3) {
            std::array<std::size_t, 3> consumed{matches.back(), 0, 0};
            matches.pop_back();
            consumed[1] = matches.back();
            matches.pop_back();
            consumed[2] = matches.back();
            matches.pop_back();

            std::sort(consumed.begin(), consumed.end(), std::greater<>{});
            for (auto index : consumed) {
              reg.destroy(inventory[index]);
              inventory.erase(inventory.begin() +
                              static_cast<std::ptrdiff_t>(index));
            }

            auto upgraded_item =
                fl::loot::EquipmentBuilder{
                    .slot = slot,
                    .armor_kind = kind,
                    .tier = *next,
                    .name = generated_name(slot, kind, *next),
                }
                    .create(reg);
            inventory.push_back(upgraded_item);
            upgraded = true;

            const auto &equipment = reg.get<Equipment>(upgraded_item);
            ctx.log().append_markup(fmt::format(
                "Upgraded 3 loose {} {} {} into [item]({}).", tier_name(tier),
                armor_kind_name(kind), slot_name(slot), equipment.name()));
          }
        }
      }
    }
  }
}

} // namespace

void PartyGearing::commit(fl::context::PartyCtx &ctx) {
  auto inventory = std::vector<entt::entity>{ctx.party_data().items().begin(),
                                             ctx.party_data().items().end()};

  equip_party(ctx, inventory);
  upgrade_inventory(ctx, inventory);

  ctx.party_data().replace_items(std::move(inventory));
}

} // namespace fl::ecs::systems
