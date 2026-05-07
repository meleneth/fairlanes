#pragma once
#include <entt/entt.hpp>
#include <vector>

#include "fl/context.hpp"
#include "encounter_data.hpp"

namespace fl::primitives {

struct EncounterBuilder {

  EncounterBuilder(fl::context::PartyCtx &ctx) : ctx_(ctx) {};

  EncounterData &thump_it_out();

  void add_field_mouse();

  void add_to_enemy_team(entt::entity entity);

private:
  fl::context::PartyCtx &ctx_;
};

} // namespace fl::primitives
