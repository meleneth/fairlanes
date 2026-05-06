#pragma once

#include <cstdlib>
#include <entt/entt.hpp>

namespace fl::ecs::components {

struct Closet {
  entt::entity chest;
  entt::entity helm;
  entt::entity pants;
  entt::entity belt;
  entt::entity boots;
  entt::entity gloves;
  entt::entity sleeves;
  entt::entity cape;

  entt::entity necklace;
  entt::entity ring_1;
  entt::entity ring_2;

  entt::entity mainhand;
  entt::entity offhand;

  entt::entity knife;

  Closet();

  bool empty() const;
};

}  // namespace fl#pragma once
