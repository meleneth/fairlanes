#pragma once

#include <entt/entity/entity.hpp>

#include "fl/ecs/components/status_effect.hpp"

namespace fl::ecs::components {

struct DireBleed {
  entt::entity source{entt::null};
  int damage_per_tick{0};
  StatusEffectInstance effect;
};

} // namespace fl::ecs::components
