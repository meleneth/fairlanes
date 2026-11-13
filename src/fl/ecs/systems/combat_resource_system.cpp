#include "fl/ecs/systems/combat_resource_system.hpp"

#include <fmt/format.h>
#include <entt/entity/handle.hpp>

#include "fl/ecs/components/combat_resources.hpp"
#include "fl/ecs/components/resource_cast_snapshot.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/primitives/encounter_data.hpp"
#include "fl/skills/skill_definition.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::ecs::systems {

SkillResourceEligibility CombatResourceSystem::eligibility(entt::registry& reg,
                                                            entt::entity actor,
                                                            fl::skills::SkillKey skill) {
  const auto& policy = fl::skills::definition(skill).resources;
  if (policy.cost <= 0) return {};
  const auto* resources = reg.try_get<fl::ecs::components::CombatResources>(actor);
  const int available = resources ? resources->get(policy.resource).current : 0;
  if (available >= policy.cost) return {};
  return {.eligible = false,
          .resource = policy.resource,
          .required = policy.cost,
          .available = available,
          .reason = fmt::format("needs {} {} (has {})", policy.cost,
                                fl::resources::display_name(policy.resource), available)};
}

int CombatResourceSystem::modify(fl::context::EncounterCtx& ctx, entt::entity actor,
                                 fl::resources::CombatResource resource, int delta,
                                 fl::resources::ResourceChangeReason reason,
                                 fl::skills::SkillKey skill) {
  if (!ctx.reg().valid(actor)) return 0;
  auto& resources = ctx.reg().get_or_emplace<fl::ecs::components::CombatResources>(actor);
  const int before = resources.get(resource).current;
  const int applied = resources.modify(resource, delta);
  if (applied != 0) {
    ctx.encounter().combatant_bus(actor).emit(fl::events::CombatantEvent{
        fl::events::ResourceChanged{actor, resource, reason, before,
                                    resources.get(resource).current, skill}});
  }
  return applied;
}

bool CombatResourceSystem::begin_skill(fl::context::EncounterCtx& ctx, entt::entity actor,
                                       fl::skills::SkillKey skill) {
  const auto check = eligibility(ctx.reg(), actor, skill);
  if (!check.eligible) {
    ctx.encounter().combatant_bus(actor).emit(fl::events::CombatantEvent{
        fl::events::SkillResourceRejected{actor, skill, check.resource, check.required,
                                          check.available}});
    ctx.log().append_markup(fmt::format("{} cannot use [ability]({}): [warn]({}).",
                                       ctx.log().name_tag_for(entt::handle{ctx.reg(), actor}),
                                       fl::skills::display_name(skill), check.reason));
    return false;
  }
  auto& resources = ctx.reg().get_or_emplace<fl::ecs::components::CombatResources>(actor);
  auto snapshot = fl::ecs::components::ResourceCastSnapshot{.skill = skill};
  for (const auto resource : fl::resources::kCombatResources) {
    snapshot.current[fl::resources::index(resource)] = resources.get(resource).current;
    snapshot.maximum[fl::resources::index(resource)] = resources.get(resource).maximum;
  }
  ctx.reg().emplace_or_replace<fl::ecs::components::ResourceCastSnapshot>(actor,
                                                                          std::move(snapshot));
  const auto& policy = fl::skills::definition(skill).resources;
  if (policy.cost > 0)
    modify(ctx, actor, policy.resource, -policy.cost,
           fl::resources::ResourceChangeReason::SkillCost, skill);
  return true;
}

void CombatResourceSystem::finish_skill(entt::registry& reg, entt::entity actor) {
  if (reg.valid(actor)) reg.remove<fl::ecs::components::ResourceCastSnapshot>(actor);
}

void CombatResourceSystem::resolve_skill(fl::context::EncounterCtx& ctx, entt::entity actor,
                                         entt::entity target, fl::skills::SkillKey skill,
                                         bool hit) {
  auto* snapshot = ctx.reg().try_get<fl::ecs::components::ResourceCastSnapshot>(actor);
  if (!snapshot || snapshot->skill != skill || snapshot->resolution_applied) return;
  snapshot->resolution_applied = true;
  const auto& policy = fl::skills::definition(skill).resources;
  if (hit && policy.generate_on_hit > 0)
    modify(ctx, actor, policy.resource, policy.generate_on_hit,
           fl::resources::ResourceChangeReason::SkillGeneration, skill);
  if (policy.restore_on_resolution > 0)
    modify(ctx, actor, policy.resource, policy.restore_on_resolution,
           fl::resources::ResourceChangeReason::SkillRestoration, skill);
  if (policy.target_delta != 0 && ctx.reg().valid(target))
    modify(ctx, target, policy.target_resource, policy.target_delta,
           fl::resources::ResourceChangeReason::SkillTargetModification, skill);
}

} // namespace fl::ecs::systems
