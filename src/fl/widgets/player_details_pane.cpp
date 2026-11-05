#include "player_details_pane.hpp"

#include <algorithm>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "fl/ecs/components/casting_skill.hpp"
#include "fl/ecs/components/closet.hpp"
#include "fl/ecs/components/equipment.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/effective_combat_stats.hpp"
#include "fl/lospec500.hpp"
#include "fl/skills/skill.hpp"
#include "fl/widgets/equipment_label.hpp"

namespace fl::widgets {

namespace {

std::string entity_label(entt::entity entity) {
  return "#" + std::to_string(static_cast<std::underlying_type_t<entt::entity>>(entity));
}

ftxui::Element equipment_label(entt::registry& reg, entt::entity equipment_id) {
  if (equipment_id == entt::null || !reg.valid(equipment_id)) {
    return ftxui::text("-");
  }

  if (auto* equipment = reg.try_get<fl::ecs::components::Equipment>(equipment_id)) {
    return equipment_name_label(*equipment);
  }

  return ftxui::text(entity_label(equipment_id));
}

} // namespace

PlayerDetailsPane::PlayerDetailsPane(entt::registry& reg, fl::primitives::PartyData& party,
                                     InventorySelected on_inventory_selected,
                                     SkillSelected on_skill_selected)
    : reg_(reg), party_(party), on_inventory_selected_(std::move(on_inventory_selected)),
      on_skill_selected_(std::move(on_skill_selected)) {}

bool PlayerDetailsPane::OnEvent(ftxui::Event event) {
  const int member_count = static_cast<int>(party_.members().size());
  if (member_count <= 0) {
    cursor_ = 0;
    return false;
  }

  cursor_ = std::clamp(cursor_, 0, member_count - 1);

  if (event == ftxui::Event::ArrowRight || event == ftxui::Event::Character("]")) {
    cursor_ = (cursor_ + 1) % member_count;
    selection_cursor_ = 0;
    scroll_cursor_ = 0;
    notify_skill_selection();
    return true;
  }

  if (event == ftxui::Event::ArrowLeft || event == ftxui::Event::Character("[")) {
    cursor_ = (cursor_ + member_count - 1) % member_count;
    selection_cursor_ = 0;
    scroll_cursor_ = 0;
    notify_skill_selection();
    return true;
  }

  if (event == ftxui::Event::ArrowDown || event == ftxui::Event::Character("j")) {
    constexpr int selection_count = fl::ecs::components::Closet::kSkillSlotCount + 1;
    selection_cursor_ = (selection_cursor_ + 1) % selection_count;
    scroll_cursor_ = selection_cursor_ < fl::ecs::components::Closet::kSkillSlotCount
                         ? 7 + selection_cursor_
                         : std::numeric_limits<int>::max();
    notify_skill_selection();
    return true;
  }

  if (event == ftxui::Event::ArrowUp || event == ftxui::Event::Character("k")) {
    constexpr int selection_count = fl::ecs::components::Closet::kSkillSlotCount + 1;
    selection_cursor_ = (selection_cursor_ + selection_count - 1) % selection_count;
    scroll_cursor_ = selection_cursor_ < fl::ecs::components::Closet::kSkillSlotCount
                         ? 7 + selection_cursor_
                         : std::numeric_limits<int>::max();
    notify_skill_selection();
    return true;
  }

  if (event == ftxui::Event::Return &&
      selection_cursor_ == fl::ecs::components::Closet::kSkillSlotCount && on_inventory_selected_) {
    on_inventory_selected_(party_.members()[static_cast<std::size_t>(cursor_)].member_id());
    return true;
  }

  if (event == ftxui::Event::Delete &&
      selection_cursor_ < fl::ecs::components::Closet::kSkillSlotCount) {
    const auto member = party_.members()[static_cast<std::size_t>(cursor_)].member_id();
    const bool dropped = party_.drop_skill(member, static_cast<std::size_t>(selection_cursor_));
    if (dropped) {
      notify_skill_selection();
    }
    return dropped;
  }

  if (event == ftxui::Event::PageDown) {
    scroll_cursor_ += 8;
    return true;
  }

  if (event == ftxui::Event::PageUp) {
    scroll_cursor_ = std::max(scroll_cursor_ - 8, 0);
    return true;
  }

  return false;
}

ftxui::Element PlayerDetailsPane::Render() {
  using namespace ftxui;

  const auto& members = party_.members();
  const int member_count = static_cast<int>(members.size());
  auto title = focused_ ? text("Player") | inverted : text("Player");

  if (member_count <= 0) {
    cursor_ = 0;
    return window(title, text("No members."));
  }

  cursor_ = std::clamp(cursor_, 0, member_count - 1);

  const auto& member = members[static_cast<std::size_t>(cursor_)];
  const auto member_id = member.member_id();
  const auto* stats = reg_.try_get<fl::ecs::components::Stats>(member_id);

  const std::string name = stats ? stats->name_ : member.name();
  Elements lines;
  lines.push_back(text(name) | bold);
  lines.push_back(
      text("Member " + std::to_string(cursor_ + 1) + "/" + std::to_string(member_count)));
  lines.push_back(separator());

  if (stats) {
    const auto effective = fl::ecs::systems::effective_combat_stats(reg_, member_id);
    lines.push_back(
        text("HP " + std::to_string(stats->hp_) + "/" + std::to_string(effective.max_hp)));
    lines.push_back(text("Attack " + std::to_string(effective.attack) + "   Armor " +
                         std::to_string(effective.armor)));
  } else {
    lines.push_back(text("No stats."));
  }

  lines.push_back(separator());
  lines.push_back(text("Skills") | bold);

  auto* party_member = reg_.try_get<fl::ecs::components::PartyMember>(member_id);
  if (party_member && party_member->closet_entity_id() != entt::null &&
      reg_.all_of<fl::ecs::components::Closet>(party_member->closet_entity_id())) {
    const auto& skill_slots = party_member->closet().skill_slots;
    const auto* casting = reg_.try_get<fl::ecs::components::CastingSkill>(member_id);
    for (int i = 0; i < fl::ecs::components::Closet::kSkillSlotCount; ++i) {
      const auto& slot = skill_slots[static_cast<std::size_t>(i)];
      auto skill_name =
          text(slot.has_value() ? fl::skills::display_name(*slot) : std::string{"-"}) | flex;
      if (slot.has_value() && casting != nullptr && casting->skill == *slot) {
        skill_name = skill_name | bgcolor(fl::lospec500::color_at(14)) |
                     color(fl::lospec500::color_at(0)) | bold;
      }
      auto line = hbox({
          text(std::to_string(i + 1) + ": ") | dim,
          std::move(skill_name),
      });
      if (focused_ && i == selection_cursor_) {
        line = line | inverted;
      }
      lines.push_back(std::move(line));
    }
  } else {
    lines.push_back(text("No skill data."));
  }

  lines.push_back(separator());
  lines.push_back(text("Gear") | bold);

  const bool has_closet =
      party_member && party_member->closet_entity_id() != entt::null &&
      reg_.all_of<fl::ecs::components::Closet>(party_member->closet_entity_id());
  if (!has_closet) {
    lines.push_back(text("No gear data."));
  } else {
    const auto& closet = party_member->closet();
    for (const auto slot : fl::loot::kEquipmentSlots) {
      lines.push_back(hbox({
          text(std::string(fl::ecs::components::to_string(slot)) + ": ") | dim,
          equipment_label(reg_, closet.equipped_item(slot)) | flex,
      }));
    }
  }

  lines.push_back(separator());
  auto inventory = hbox({text("Inventory") | bold, filler()});
  if (focused_ && selection_cursor_ == fl::ecs::components::Closet::kSkillSlotCount) {
    inventory = inventory | inverted;
  }
  lines.push_back(std::move(inventory));

  scroll_cursor_ = std::clamp(scroll_cursor_, 0, std::max(0, static_cast<int>(lines.size()) - 1));
  auto content = vbox(std::move(lines));
  if (focused_) {
    content = content | focusPosition(0, scroll_cursor_);
  }

  return window(title, std::move(content) | yframe | vscroll_indicator | flex);
}

void PlayerDetailsPane::set_focused(bool focused) {
  focused_ = focused;
  notify_skill_selection();
}

void PlayerDetailsPane::notify_skill_selection() {
  if (!on_skill_selected_) {
    return;
  }
  if (!focused_ || selection_cursor_ < 0 ||
      selection_cursor_ >= fl::ecs::components::Closet::kSkillSlotCount || cursor_ < 0 ||
      cursor_ >= static_cast<int>(party_.members().size())) {
    on_skill_selected_(std::nullopt);
    return;
  }

  const auto member = party_.members()[static_cast<std::size_t>(cursor_)].member_id();
  const auto* party_member = reg_.try_get<fl::ecs::components::PartyMember>(member);
  if (party_member == nullptr) {
    on_skill_selected_(std::nullopt);
    return;
  }

  on_skill_selected_(
      party_member->closet().skill_slots[static_cast<std::size_t>(selection_cursor_)]);
}

} // namespace fl::widgets
