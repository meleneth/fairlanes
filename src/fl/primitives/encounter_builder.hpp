#pragma once
#include <entt/entt.hpp>
#include <vector>

#include "fl/context.hpp"

namespace fl::concepts {

struct EncounterBuilder {

  EncounterBuilder();

  void thump_it_out(fl::context::PartyCtx &ctx);

  void add_field_mouse(fl::context::PartyCtx &ctx);

  void add_to_enemy_team(entt::entity entity);
};

} // namespace fl::concepts
