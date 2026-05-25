#pragma once

#include <entt/entt.hpp>

#include "fl/ecs/components/status_effect.hpp"

namespace fl::ecs::components {

struct Freeze {
  entt::entity source{entt::null};
  int clear_after_beats{0};
  StatusEffectInstance effect;
};

} // namespace fl::ecs::components
