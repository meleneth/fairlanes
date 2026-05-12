#include "inventory_list.hpp"

#include <algorithm>
#include <string>
#include <type_traits>

#include "fl/ecs/components/equipment.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/widgets/equipment_label.hpp"

namespace fl::widgets {

namespace {

std::string entity_label(entt::entity entity) {
  return "#" + std::to_string(static_cast<std::underlying_type_t<entt::entity>>(
                   entity));
}

} // namespace

InventoryList::InventoryList(entt::registry &reg,
                             fl::primitives::PartyData &party)
    : reg_(reg), party_(party) {}

bool InventoryList::OnEvent(ftxui::Event event) {
  const int item_count = static_cast<int>(party_.items().size());
  if (item_count <= 0) {
    cursor_ = 0;
    return false;
  }

  if (event == ftxui::Event::ArrowDown ||
      event == ftxui::Event::Character("j")) {
    cursor_ = std::min(cursor_ + 1, item_count - 1);
    return true;
  }

  if (event == ftxui::Event::ArrowUp || event == ftxui::Event::Character("k")) {
    cursor_ = std::max(cursor_ - 1, 0);
    return true;
  }

  if (event == ftxui::Event::PageDown) {
    cursor_ = std::min(cursor_ + 8, item_count - 1);
    return true;
  }

  if (event == ftxui::Event::PageUp) {
    cursor_ = std::max(cursor_ - 8, 0);
    return true;
  }

  return false;
}

ftxui::Element InventoryList::Render() {
  using namespace ftxui;

  auto items = party_.items();
  const int item_count = static_cast<int>(items.size());
  if (item_count <= 0) {
    cursor_ = 0;
    auto title = focused_ ? text("Inventory") | inverted : text("Inventory");
    return window(title, text("No items."));
  }

  cursor_ = std::clamp(cursor_, 0, item_count - 1);

  Elements lines;
  lines.reserve(items.size());

  for (int i = 0; i < item_count; ++i) {
    const auto item = items[static_cast<std::size_t>(i)];
    Element line = text(entity_label(item));

    if (auto *equipment =
            reg_.try_get<fl::ecs::components::Equipment>(item)) {
      line = equipment_inventory_label(*equipment);
    } else if (auto *stats =
                   reg_.try_get<fl::ecs::components::Stats>(item)) {
      line = text(stats->name_);
    }

    if (i == cursor_) {
      line = line | inverted | focusPosition(0, i);
    }
    lines.push_back(line);
  }

  auto title = focused_ ? text("Inventory") | inverted : text("Inventory");
  return window(title, vbox(std::move(lines)) | yframe | vscroll_indicator |
                           flex);
}

void InventoryList::set_focused(bool focused) noexcept { focused_ = focused; }

} // namespace fl::widgets
