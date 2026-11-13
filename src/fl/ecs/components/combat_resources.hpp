#pragma once

#include <algorithm>
#include <array>

#include "fl/resources/combat_resource.hpp"

namespace fl::ecs::components {

struct ResourcePool {
  int current{0};
  int maximum{0};
};

struct CombatResources {
  std::array<ResourcePool, fl::resources::index(fl::resources::CombatResource::Count)> pools{
      ResourcePool{0, 10}, ResourcePool{10, 10}};

  ResourcePool& get(fl::resources::CombatResource resource) noexcept {
    return pools[fl::resources::index(resource)];
  }
  const ResourcePool& get(fl::resources::CombatResource resource) const noexcept {
    return pools[fl::resources::index(resource)];
  }

  int set(fl::resources::CombatResource resource, int amount) noexcept {
    auto& pool = get(resource);
    pool.maximum = std::max(0, pool.maximum);
    pool.current = std::clamp(amount, 0, pool.maximum);
    return pool.current;
  }

  int modify(fl::resources::CombatResource resource, int delta) noexcept {
    const int before = get(resource).current;
    set(resource, before + delta);
    return get(resource).current - before;
  }
};

} // namespace fl::ecs::components
