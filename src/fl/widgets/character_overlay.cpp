#include "fl/widgets/character_overlay.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include <ftxui/dom/elements.hpp>

#include "fl/ecs/components/combat_affinity.hpp"
#include "fl/ecs/components/equipment.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/ecs/systems/change_combat_affinity.hpp"
#include "fl/ecs/systems/effective_combat_stats.hpp"
#include "fl/loot/equipment_slot.hpp"
#include "fl/loot/equipment_stat_policy.hpp"
#include "fl/lospec500.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/skills/skill_definition.hpp"
#include "fl/skills/skill_scaling.hpp"
#include "fl/widgets/equipment_label.hpp"

namespace fl::widgets {

CharacterOverlay::CharacterOverlay(fl::context::AccountCtx ctx, std::size_t party_index)
    : ctx_(std::move(ctx)), party_index_(party_index) {}

bool CharacterOverlay::OnEvent(ftxui::Event event) {
  auto& parties = ctx_.account_data().parties();
  if (party_index_ >= parties.size() || parties[party_index_].members().empty()) return true;
  auto& party = parties[party_index_];
  if (event == ftxui::Event::ArrowLeft) {
    member_index_ = (member_index_ + party.members().size() - 1) % party.members().size();
    return true;
  }
  if (event == ftxui::Event::ArrowRight) {
    member_index_ = (member_index_ + 1) % party.members().size();
    return true;
  }
  fl::primitives::CombatAffinity affinity;
  if (event == ftxui::Event::Character("1"))
    affinity = fl::primitives::CombatAffinity::Cloth;
  else if (event == ftxui::Event::Character("2"))
    affinity = fl::primitives::CombatAffinity::Leather;
  else if (event == ftxui::Event::Character("3"))
    affinity = fl::primitives::CombatAffinity::Plate;
  else
    return true;
  const auto member = party.members()[member_index_].member_id();
  const auto result =
      fl::ecs::systems::ChangeCombatAffinity::commit(party.party_ctx(), member, affinity);
  message_ = result == fl::ecs::systems::AffinityChangeResult::Changed
                 ? "Affinity changed; compatible inventory was reevaluated."
             : result == fl::ecs::systems::AffinityChangeResult::NotInTown
                 ? "Town only: affinity cannot change during an active encounter or raid."
             : result == fl::ecs::systems::AffinityChangeResult::Unchanged
                 ? "That affinity is already active."
                 : "Affinity change is unavailable for this character.";
  return true;
}

ftxui::Element CharacterOverlay::Render() {
  using namespace ftxui;
  auto& parties = ctx_.account_data().parties();
  if (party_index_ >= parties.size() || parties[party_index_].members().empty())
    return window(text("Character management"), text("No selected party members.")) | center;
  auto& party = parties[party_index_];
  member_index_ = std::min(member_index_, party.members().size() - 1);
  const auto& data = party.members()[member_index_];
  const auto member = data.member_id();
  auto& reg = ctx_.reg();
  const auto& assignment = reg.get<fl::ecs::components::PartyMember>(member);
  const auto& xp = reg.get<fl::ecs::components::TrackXP>(member);
  const auto active = reg.get<fl::ecs::components::ActiveCombatAffinity>(member).value;
  const auto stats = fl::ecs::systems::effective_combat_stats(reg, member);

  Elements skills;
  for (std::size_t index = 0; index < assignment.closet().skill_slots.size(); ++index) {
    const auto& slot = assignment.closet().skill_slots[index];
    if (!slot) {
      skills.push_back(text(index == 0 ? "Observe: --" : "Skill: --") | dim);
      continue;
    }
    const auto& def = fl::skills::definition(*slot);
    const auto current =
        fl::skills::evaluate_scaling(reg, member, def.scaling, def.scaling.base_effect);
    skills.push_back(paragraph((index == 0 ? "Observe: " : "Skill: ") +
                               fl::skills::display_name(*slot) + " | " +
                               std::string(fl::skills::to_string(def.scaling.dimension)) + " | " +
                               fl::skills::describe_scaling(def.scaling) + " | current numeric " +
                               std::to_string(current.total)));
  }
  Elements learned_skills;
  for (const auto skill : data.grimoire().known_skills()) {
    const auto& def = fl::skills::definition(skill);
    const auto current =
        fl::skills::evaluate_scaling(reg, member, def.scaling, def.scaling.base_effect);
    learned_skills.push_back(paragraph("Available: " + fl::skills::display_name(skill) + " | " +
                                       std::string(fl::skills::to_string(def.scaling.dimension)) +
                                       " | " + fl::skills::describe_scaling(def.scaling) +
                                       " | current numeric " + std::to_string(current.total)));
  }

  Elements gear;
  for (const auto slot : fl::loot::kEquipmentSlots) {
    const auto item = assignment.closet().equipped_item(slot);
    std::string line = std::string(fl::ecs::components::to_string(slot)) + ": ";
    if (!reg.valid(item)) {
      gear.push_back(text(line + "--") | dim);
      continue;
    }
    const auto& equipment = reg.get<fl::ecs::components::Equipment>(item);
    const bool compatible = !equipment.is_armor() ||
                            (active == fl::primitives::CombatAffinity::Cloth &&
                             equipment.armor_kind() == fl::loot::ArmorKind::cloth) ||
                            (active == fl::primitives::CombatAffinity::Leather &&
                             equipment.armor_kind() == fl::loot::ArmorKind::leather) ||
                            (active == fl::primitives::CombatAffinity::Plate &&
                             equipment.armor_kind() == fl::loot::ArmorKind::plate);
    line += std::string(equipment.name()) + " " + equipment_modifier_text(equipment) +
            " | tier level " + std::to_string(fl::loot::equipment_minimum_level(equipment.tier())) +
            (compatible ? " | compatible" : " | transition gear");
    gear.push_back(paragraph(line));
  }

  auto body = vbox(
      {text("←/→ character   1 Cloth (control)   2 Leather (ranged)   3 Plate (brawling)   Esc "
            "close") |
           bold,
       separator(),
       text(std::string(party.name()) + " — " + data.name() + " — Level " +
            std::to_string(xp.level_) + " — " + std::string(fl::primitives::to_string(active))) |
           bold,
       text("Effective: ATK " + std::to_string(stats.attack) + "  ARM " +
            std::to_string(stats.armor) + "  HP " + std::to_string(stats.max_hp) +
            "  Resist P/M/F/I/L " + std::to_string(stats.resistances.physical) + "/" +
            std::to_string(stats.resistances.magical) + "/" +
            std::to_string(stats.resistances.fire) + "/" + std::to_string(stats.resistances.ice) +
            "/" + std::to_string(stats.resistances.lightning)),
       separator(), vbox(std::move(skills)), vbox(std::move(learned_skills)), separator(),
       vbox(std::move(gear)), separator(),
       paragraph(message_.empty() ? "Affinity changes are immediate only at a town encounter "
                                    "boundary; combat remains live behind this overlay."
                                  : message_)});
  return window(text("Character skills & equipment"), std::move(body)) |
         size(WIDTH, LESS_THAN, 110) | size(HEIGHT, LESS_THAN, 40) | center |
         bgcolor(fl::lospec500::color_at(0)) | clear_under;
}

} // namespace fl::widgets
