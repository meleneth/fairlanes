#pragma once
#include <entt/entt.hpp>
#include <vector>

#include "fl/context.hpp"

namespace fl::concepts {

struct EncounterBuilder {
  fl::context::PartyCtx &ctx_;

  EncounterBuilder(fl::context::PartyCtx &ctx) : ctx_(ctx) {};

  void thump_it_out();

  void add_field_mouse();

  void add_to_enemy_team(entt::entity entity);
};

} // namespace fl::concepts
