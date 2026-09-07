#pragma once
#include <entt/entity/registry.hpp>

namespace fl::ecs {
using Registry = entt::registry;
using Entity = entt::entity;

// Small helpers
inline bool alive(const Registry &reg, Entity e) { return reg.valid(e); }
} // namespace fl::ecs
