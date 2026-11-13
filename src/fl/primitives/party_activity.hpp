#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <entt/entity/entity.hpp>

#include "fl/economy/material.hpp"
#include "fl/economy/profession.hpp"
#include "fl/economy/recipe.hpp"
#include "fl/skills/skill.hpp"

namespace fl::primitives {

enum class PartyActivity { CombatProgression, TargetedFarming, Gathering, Crafting, SkillStudy, Idle };

inline constexpr std::string_view display_name(PartyActivity activity) noexcept {
  switch (activity) {
  case PartyActivity::CombatProgression: return "Combat progression";
  case PartyActivity::TargetedFarming: return "Targeted farming";
  case PartyActivity::Gathering: return "Gathering";
  case PartyActivity::Crafting: return "Crafting";
  case PartyActivity::SkillStudy: return "Skill study";
  case PartyActivity::Idle: return "Idle";
  }
  return "Unknown";
}

struct PartyActivityAssignment {
  PartyActivity activity{PartyActivity::CombatProgression};
  fl::economy::Profession profession{fl::economy::Profession::Herbalism};
  std::string biome_id{};
  std::optional<fl::economy::Material> material{};
  std::optional<fl::economy::RecipeId> recipe{};
  entt::entity student{entt::null};
  fl::skills::SkillKey skill{fl::skills::SkillId::Observe};
};

struct ProductionOperation {
  PartyActivity activity{PartyActivity::Idle};
  fl::economy::Profession profession{fl::economy::Profession::Herbalism};
  entt::entity worker{entt::null};
  std::optional<fl::economy::Material> material{};
  std::optional<fl::economy::RecipeId> recipe{};
  fl::skills::SkillKey skill{fl::skills::SkillId::Observe};
  std::uint64_t started_world_beat{0};
  std::uint64_t completes_world_beat{0};
};

} // namespace fl::primitives
