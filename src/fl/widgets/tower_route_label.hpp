#pragma once

#include <string>
#include <string_view>

#include "fl/primitives/party_data.hpp"

namespace fl::widgets {

inline std::string_view tower_biome_label(std::string_view biome_id) {
  if (biome_id == "origin_woodland_tower") return "Woodland";
  if (biome_id == "origin_plains_tower") return "Plains";
  if (biome_id == "origin_murky_swamp_tower") return "Murky Swamp";
  if (biome_id == "origin_caves_tower") return "Caves";
  if (biome_id == "origin_desert_tower") return "Desert";
  return biome_id;
}

inline std::string tower_route_label(const fl::primitives::PartyData& party) {
  std::string label{party.name()};
  const auto route = party.tower_route();
  if (!route) return label + " | Town | No tower selected";

  label += party.has_encounter() ? " | Origin " : " | Town | Next: Origin ";
  label += tower_biome_label(route->biome_id);
  label += " | Floor " + std::to_string(route->floor) + "/100";
  return label;
}

} // namespace fl::widgets
