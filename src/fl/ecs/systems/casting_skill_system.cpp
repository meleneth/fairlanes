#include "fl/ecs/systems/casting_skill_system.hpp"

#include <entt/entity/registry.hpp>

#include "fl/ecs/components/casting_skill.hpp"

namespace fl::ecs::systems {

fl::events::ScopedCombatantListener
CastingSkillSystem::bind_started_listener(entt::registry& reg, fl::events::CombatantBus& bus) {
  return {bus, std::in_place_type<fl::events::SkillCastStarted>,
          [&reg](const fl::events::SkillCastStarted& event) {
            if (!reg.valid(event.caster)) {
              return;
            }
            reg.emplace_or_replace<fl::ecs::components::CastingSkill>(event.caster, event.skill);
          }};
}

fl::events::ScopedCombatantListener
CastingSkillSystem::bind_finished_listener(entt::registry& reg, fl::events::CombatantBus& bus) {
  return {bus, std::in_place_type<fl::events::SkillCastFinished>,
          [&reg](const fl::events::SkillCastFinished& event) {
            if (reg.valid(event.caster)) {
              reg.remove<fl::ecs::components::CastingSkill>(event.caster);
            }
          }};
}

} // namespace fl::ecs::systems
