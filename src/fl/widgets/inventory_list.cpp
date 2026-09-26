#include "inventory_list.hpp"

#include <algorithm>
#include <string>
#include <type_traits>

#include "fl/ecs/components/equipment.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/raid_trinket.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/skills/skill_definition.hpp"
#include "fl/widgets/equipment_label.hpp"

namespace fl::widgets {
namespace {

std::string entity_label(entt::entity entity) {
  return "#" + std::to_string(
                   static_cast<std::underlying_type_t<entt::entity>>(entity));
}

int wrap_cursor(int cursor, int count, int delta) {
  if (count <= 0) {
    return 0;
  }
  return (cursor + count + delta) % count;
}

} // namespace

InventoryList::InventoryList(entt::registry &reg,
                             fl::primitives::PartyData &party)
    : reg_(reg), party_(party) {}

bool InventoryList::OnEvent(ftxui::Event event) {
  if (mode_ == Mode::skill_information) {
    if (event == ftxui::Event::Backspace || event == ftxui::Event::ArrowLeft) {
      hide_skill_information();
      return true;
    }
    return false;
  }

  if (mode_ == Mode::equipment_slots) {
    const int slot_count = static_cast<int>(fl::loot::kEquipmentSlots.size());
    slot_cursor_ = std::clamp(slot_cursor_, 0, slot_count - 1);
    if (event == ftxui::Event::ArrowDown ||
        event == ftxui::Event::Character("j")) {
      slot_cursor_ = wrap_cursor(slot_cursor_, slot_count, 1);
      return true;
    }
    if (event == ftxui::Event::ArrowUp ||
        event == ftxui::Event::Character("k")) {
      slot_cursor_ = wrap_cursor(slot_cursor_, slot_count, -1);
      return true;
    }
    if (event == ftxui::Event::Return) {
      mode_ = Mode::filtered_items;
      item_cursor_ = 0;
      return true;
    }
    if (event == ftxui::Event::Backspace || event == ftxui::Event::ArrowLeft) {
      mode_ = Mode::party_inventory;
      item_cursor_ = 0;
      return true;
    }
    return false;
  }

  if (mode_ == Mode::filtered_items) {
    if (event == ftxui::Event::Backspace || event == ftxui::Event::ArrowLeft) {
      mode_ = Mode::equipment_slots;
      item_cursor_ = 0;
      return true;
    }

    const auto items = filtered_items();
    const int item_count = static_cast<int>(items.size());
    if (item_count <= 0) {
      item_cursor_ = 0;
      return false;
    }
    item_cursor_ = std::clamp(item_cursor_, 0, item_count - 1);
    if (event == ftxui::Event::ArrowDown ||
        event == ftxui::Event::Character("j")) {
      item_cursor_ = wrap_cursor(item_cursor_, item_count, 1);
      return true;
    }
    if (event == ftxui::Event::ArrowUp ||
        event == ftxui::Event::Character("k")) {
      item_cursor_ = wrap_cursor(item_cursor_, item_count, -1);
      return true;
    }
    if (event == ftxui::Event::Return) {
      party_.party_bus().emit(
          fl::events::PartyEvent{fl::events::EquipmentEquipRequested{
              selected_member_, selected_slot(),
              items[static_cast<std::size_t>(item_cursor_)]}});
      mode_ = Mode::equipment_slots;
      item_cursor_ = 0;
      return true;
    }
    return false;
  }

  const int item_count = static_cast<int>(party_.items().size());
  if (item_count <= 0) {
    item_cursor_ = 0;
    return false;
  }
  item_cursor_ = std::clamp(item_cursor_, 0, item_count - 1);

  if (event == ftxui::Event::ArrowDown ||
      event == ftxui::Event::Character("j")) {
    item_cursor_ = std::min(item_cursor_ + 1, item_count - 1);
    return true;
  }
  if (event == ftxui::Event::ArrowUp || event == ftxui::Event::Character("k")) {
    item_cursor_ = std::max(item_cursor_ - 1, 0);
    return true;
  }
  if (event == ftxui::Event::PageDown) {
    item_cursor_ = std::min(item_cursor_ + 8, item_count - 1);
    return true;
  }
  if (event == ftxui::Event::PageUp) {
    item_cursor_ = std::max(item_cursor_ - 8, 0);
    return true;
  }
  return false;
}

ftxui::Element InventoryList::Render() {
  switch (mode_) {
  case Mode::party_inventory:
    return render_party_inventory();
  case Mode::equipment_slots:
    return render_equipment_slots();
  case Mode::filtered_items:
    return render_filtered_items();
  case Mode::skill_information:
    return render_skill_information();
  }
  return ftxui::text("");
}

ftxui::Element InventoryList::render_party_inventory() {
  using namespace ftxui;

  auto items = party_.items();
  const int item_count = static_cast<int>(items.size());
  auto title = focused_ ? text("Inventory") | inverted : text("Inventory");
  if (item_count <= 0) {
    item_cursor_ = 0;
    return window(title, text("No items."));
  }
  item_cursor_ = std::clamp(item_cursor_, 0, item_count - 1);

  Elements lines;
  lines.reserve(items.size());
  for (int i = 0; i < item_count; ++i) {
    const auto item = items[static_cast<std::size_t>(i)];
    Element line = text(entity_label(item));
    if (auto *equipment = reg_.try_get<fl::ecs::components::Equipment>(item)) {
      line = equipment_inventory_label(*equipment);
    } else if (reg_.all_of<fl::ecs::components::RaidTrinket>(item)) {
      line = text("Raid trinket [exclusive]") | color(Color::MagentaLight);
    } else if (auto *stats = reg_.try_get<fl::ecs::components::Stats>(item)) {
      line = text(stats->name_);
    }
    if (i == item_cursor_) {
      line = line | inverted | focusPosition(0, i);
    }
    lines.push_back(std::move(line));
  }
  return window(title,
                vbox(std::move(lines)) | yframe | vscroll_indicator | flex);
}

ftxui::Element InventoryList::render_equipment_slots() {
  using namespace ftxui;

  auto title =
      focused_ ? text("Equipment slots") | inverted : text("Equipment slots");
  if (!selected_member_is_valid()) {
    return window(title, text("No selected player."));
  }

  auto &closet =
      reg_.get<fl::ecs::components::PartyMember>(selected_member_).closet();
  slot_cursor_ = std::clamp(
      slot_cursor_, 0, static_cast<int>(fl::loot::kEquipmentSlots.size()) - 1);
  Elements lines;
  lines.reserve(fl::loot::kEquipmentSlots.size());
  for (std::size_t i = 0; i < fl::loot::kEquipmentSlots.size(); ++i) {
    const auto slot = fl::loot::kEquipmentSlots[i];
    const auto item = closet.equipped_item(slot);
    Element equipped = text("-");
    if (item != entt::null && reg_.valid(item)) {
      if (const auto *equipment =
              reg_.try_get<fl::ecs::components::Equipment>(item)) {
        equipped = equipment_name_label(*equipment);
      }
    }
    auto line = hbox({
        text(std::string(fl::ecs::components::to_string(slot)) + ": ") | dim,
        std::move(equipped),
    });
    if (focused_ && static_cast<int>(i) == slot_cursor_) {
      line = line | inverted | focusPosition(0, static_cast<int>(i));
    }
    lines.push_back(std::move(line));
  }
  return window(title,
                vbox(std::move(lines)) | yframe | vscroll_indicator | flex);
}

ftxui::Element InventoryList::render_filtered_items() {
  using namespace ftxui;

  const auto slot = selected_slot();
  const auto slot_name = std::string(fl::ecs::components::to_string(slot));
  auto title = focused_ ? text(slot_name + " items") | inverted
                        : text(slot_name + " items");
  const auto items = filtered_items();
  if (items.empty()) {
    item_cursor_ = 0;
    return window(title, text("No matching equipment."));
  }
  item_cursor_ =
      std::clamp(item_cursor_, 0, static_cast<int>(items.size()) - 1);

  Elements lines;
  lines.reserve(items.size());
  for (std::size_t i = 0; i < items.size(); ++i) {
    auto line = equipment_name_label(
        reg_.get<fl::ecs::components::Equipment>(items[i]));
    if (focused_ && static_cast<int>(i) == item_cursor_) {
      line = line | inverted | focusPosition(0, static_cast<int>(i));
    }
    lines.push_back(std::move(line));
  }
  return window(title,
                vbox(std::move(lines)) | yframe | vscroll_indicator | flex);
}

ftxui::Element InventoryList::render_skill_information() {
  using namespace ftxui;

  auto title = text("Skill information") | bold;
  if (!selected_skill_.has_value()) {
    return window(title, text("No selected skill."));
  }

  const auto skill = *selected_skill_;
  const auto &definition = fl::skills::definition(skill);
  return window(
      title,
      vbox({text(fl::skills::display_name(skill)) | bold,
            text("Rank " + std::to_string(skill.rank.value())) | dim,
            separator(), paragraph(std::string{definition.description})}) |
          yframe | vscroll_indicator | flex);
}

std::vector<entt::entity> InventoryList::filtered_items() const {
  std::vector<entt::entity> matches;
  const auto slot = selected_slot();
  for (const auto item : party_.items()) {
    const auto *equipment = reg_.try_get<fl::ecs::components::Equipment>(item);
    if (equipment != nullptr && equipment->slot() == slot) {
      matches.push_back(item);
    }
  }
  return matches;
}

fl::loot::EquipmentSlot InventoryList::selected_slot() const noexcept {
  const auto index = std::clamp(
      slot_cursor_, 0, static_cast<int>(fl::loot::kEquipmentSlots.size()) - 1);
  return fl::loot::kEquipmentSlots[static_cast<std::size_t>(index)];
}

bool InventoryList::selected_member_is_valid() const {
  if (selected_member_ == entt::null || !reg_.valid(selected_member_) ||
      !reg_.all_of<fl::ecs::components::PartyMember>(selected_member_)) {
    return false;
  }
  return std::any_of(party_.members().begin(), party_.members().end(),
                     [&](const auto &member) {
                       return member.member_id() == selected_member_;
                     });
}

void InventoryList::show_equipment_slots(entt::entity member) noexcept {
  selected_member_ = member;
  selected_skill_.reset();
  slot_cursor_ = 0;
  item_cursor_ = 0;
  mode_ = Mode::equipment_slots;
}

void InventoryList::show_skill_information(
    fl::skills::SkillKey skill) noexcept {
  selected_skill_ = skill;
  mode_ = Mode::skill_information;
}

void InventoryList::hide_skill_information() noexcept {
  if (mode_ == Mode::skill_information) {
    selected_skill_.reset();
    mode_ = Mode::party_inventory;
  }
}

void InventoryList::set_focused(bool focused) noexcept { focused_ = focused; }

} // namespace fl::widgets
