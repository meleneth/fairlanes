#include "equipment_label.hpp"

#include <string>

#include "fl/lospec500.hpp"

namespace fl::widgets {

ftxui::Element equipment_name_label(
    const fl::ecs::components::Equipment &equipment) {
  return ftxui::text(std::string(equipment.name())) |
         fl::lospec500::at(equipment.palette_index());
}

ftxui::Element equipment_inventory_label(
    const fl::ecs::components::Equipment &equipment) {
  return ftxui::hbox({
      equipment_name_label(equipment),
      ftxui::text(" [") | ftxui::dim,
      ftxui::text(std::string(fl::ecs::components::to_string(
          equipment.slot()))) | ftxui::dim,
      ftxui::text("]") | ftxui::dim,
  });
}

} // namespace fl::widgets
