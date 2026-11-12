#include "equipment_label.hpp"

#include <string>
#include <vector>

#include <fmt/format.h>

#include "fl/lospec500.hpp"

namespace fl::widgets {

std::string equipment_modifier_text(const fl::ecs::components::Equipment& equipment) {
  std::vector<std::string> modifiers;
  if (equipment.attack() != 0) modifiers.push_back(fmt::format("+{} Atk", equipment.attack()));
  if (equipment.armor() != 0) modifiers.push_back(fmt::format("+{} Arm", equipment.armor()));
  if (equipment.max_hp() != 0) modifiers.push_back(fmt::format("+{} HP", equipment.max_hp()));
  const auto& r = equipment.resistances();
  if (r.physical != 0) modifiers.push_back(fmt::format("+{}% Phys", r.physical));
  if (r.magical != 0) modifiers.push_back(fmt::format("+{}% Mag", r.magical));
  if (r.fire != 0) modifiers.push_back(fmt::format("+{}% Fire", r.fire));
  if (r.ice != 0) modifiers.push_back(fmt::format("+{}% Ice", r.ice));
  if (r.lightning != 0) modifiers.push_back(fmt::format("+{}% Lit", r.lightning));
  std::string result;
  for (std::size_t i = 0; i < modifiers.size(); ++i) {
    if (i != 0) result += " ";
    result += modifiers[i];
  }
  return result;
}

ftxui::Element equipment_name_label(const fl::ecs::components::Equipment& equipment) {
  auto elements = ftxui::Elements{ftxui::text(std::string(equipment.name())) |
                                  fl::lospec500::at(equipment.palette_index())};
  const auto modifiers = equipment_modifier_text(equipment);
  if (!modifiers.empty()) elements.push_back(ftxui::text(" (" + modifiers + ")") | ftxui::dim);
  return ftxui::hbox(std::move(elements));
}

ftxui::Element equipment_inventory_label(const fl::ecs::components::Equipment& equipment) {
  return ftxui::hbox({
      equipment_name_label(equipment),
      ftxui::text(" [") | ftxui::dim,
      ftxui::text(std::string(fl::ecs::components::to_string(equipment.slot()))) | ftxui::dim,
      ftxui::text("]") | ftxui::dim,
  });
}

} // namespace fl::widgets
