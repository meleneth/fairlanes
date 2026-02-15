#pragma once
#include "entt/entity/fwd.hpp"
#include <cstdint>

#include <entt/entt.hpp>

namespace seerin {

// Keep these tiny and boring. They’re the “unit of identity” for core combat.
using CombatantId = entt::entity;
using TeamId = int32_t;
using SkillId = int32_t;

} // namespace seerin
