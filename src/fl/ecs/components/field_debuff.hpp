#pragma once

#include <entt/entt.hpp>

#include <string>
#include <vector>

namespace fl::ecs::components {

enum class FieldTeam {
  Attackers,
  Defenders,
};

enum class FieldDebuffKind {
  AccuracyDown,
  DamageDown,
  Vulnerable,
};

struct FieldDebuffEffect {
  FieldTeam team{FieldTeam::Defenders};
  FieldDebuffKind kind{FieldDebuffKind::AccuracyDown};
  std::string name;
  entt::entity source{entt::null};
  int value{0};
  int duration_seconds{0};
  bool removable{true};
};

struct FieldDebuffs {
  std::vector<FieldDebuffEffect> effects;
};

} // namespace fl::ecs::components
