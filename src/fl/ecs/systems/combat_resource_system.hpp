#pragma once

#include <string>

#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>

#include "fl/context.hpp"
#include "fl/resources/combat_resource.hpp"
#include "fl/skills/skill.hpp"

namespace fl::ecs::systems {

struct SkillResourceEligibility {
  bool eligible{true};
  fl::resources::CombatResource resource{fl::resources::CombatResource::Adrenaline};
  int required{0};
  int available{0};
  std::string reason;
};

class CombatResourceSystem {
public:
  static SkillResourceEligibility eligibility(entt::registry& reg, entt::entity actor,
                                              fl::skills::SkillKey skill);
  static bool begin_skill(fl::context::EncounterCtx& ctx, entt::entity actor,
                          fl::skills::SkillKey skill);
  static void finish_skill(entt::registry& reg, entt::entity actor);
  static void resolve_skill(fl::context::EncounterCtx& ctx, entt::entity actor,
                            entt::entity target, fl::skills::SkillKey skill, bool hit);
  static int modify(fl::context::EncounterCtx& ctx, entt::entity actor,
                    fl::resources::CombatResource resource, int delta,
                    fl::resources::ResourceChangeReason reason, fl::skills::SkillKey skill);
};

} // namespace fl::ecs::systems
