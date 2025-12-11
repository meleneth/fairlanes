// fl/events/combat.hpp
#pragma once
#include <chrono>
#include <entt/entt.hpp>

namespace fl::events {

struct Tick {
  std::chrono::milliseconds dt;
};

struct PlayerCommandAttack {
  entt::entity attacker;
  entt::entity target;
};

struct AttackResolved {
  entt::entity attacker;
  entt::entity target;
  int damage;
};

} // namespace fl::events
