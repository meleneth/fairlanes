#pragma once

#include <ftxui/dom/elements.hpp>

#include "fl/ecs/components/equipment.hpp"

namespace fl::widgets {

std::string equipment_modifier_text(const fl::ecs::components::Equipment& equipment);

ftxui::Element equipment_name_label(const fl::ecs::components::Equipment& equipment);

ftxui::Element equipment_inventory_label(const fl::ecs::components::Equipment& equipment);

} // namespace fl::widgets
