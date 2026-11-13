#include "fl/ecs/systems/consumable_system.hpp"

#include <algorithm>
#include <array>
#include <entt/entity/handle.hpp>
#include <fmt/format.h>
#include <utility>
#include <vector>

#include "fl/ecs/components/combat_resources.hpp"
#include "fl/ecs/components/consumable.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/timed_buffs.hpp"
#include "fl/ecs/systems/combat_resource_system.hpp"
#include "fl/ecs/systems/effective_combat_stats.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/generated/consumable_content.hpp"
#include "fl/primitives/encounter_data.hpp"
#include "fl/primitives/member_data.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/world_clock.hpp"
#include "fl/skills/skill_definition.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::ecs::systems {
namespace {

bool party_owns(const fl::primitives::PartyData& party, entt::entity item) {
  return std::ranges::find(party.items(), item) != party.items().end();
}

void consume_one(fl::context::PartyCtx& ctx, entt::entity item) {
  auto& consumable = ctx.reg().get<fl::ecs::components::Consumable>(item);
  --consumable.quantity;
  if (consumable.quantity > 0) return;
  std::vector<entt::entity> remaining;
  for (const auto candidate : ctx.party_data().items())
    if (candidate != item) remaining.push_back(candidate);
  ctx.party_data().replace_items(std::move(remaining));
  ctx.reg().destroy(item);
}

void emit_to_member(entt::registry& reg, entt::entity actor, fl::events::CombatantEvent event) {
  if (auto* member = reg.try_get<fl::ecs::components::PartyMember>(actor))
    member->member_data().combatant_bus().emit(std::move(event));
}

} // namespace

entt::entity ConsumableSystem::create(entt::registry& reg, fl::consumables::ConsumableKind kind,
                                      int quantity) {
  const auto item = reg.create();
  const auto values = fl::consumables::generated_content::definition(kind);
  reg.emplace<fl::ecs::components::Consumable>(
      item, fl::ecs::components::Consumable{kind, std::max(1, quantity), values.magnitude,
                                            values.duration_seconds});
  return item;
}

void ConsumableSystem::grant_starter_pack(fl::context::PartyCtx& ctx) {
  if (std::ranges::any_of(ctx.party_data().items(), [&](const auto item) {
        return ctx.reg().all_of<fl::ecs::components::Consumable>(item);
      }))
    return;
  constexpr std::array pack{
      std::pair{fl::consumables::ConsumableKind::HealingPotion, 4},
      std::pair{fl::consumables::ConsumableKind::AdrenalinePotion, 2},
      std::pair{fl::consumables::ConsumableKind::ReflexPotion, 2},
      std::pair{fl::consumables::ConsumableKind::AttackFlask, 1},
      std::pair{fl::consumables::ConsumableKind::ArmorFlask, 1},
      std::pair{fl::consumables::ConsumableKind::TempoFlask, 1}};
  for (const auto& [kind, quantity] : pack)
    ctx.party_data().add_item(create(ctx.reg(), kind, quantity));
}

std::optional<entt::entity> ConsumableSystem::choose_combat_potion(
    fl::context::EncounterCtx& ctx, entt::entity actor) {
  auto* member = ctx.reg().try_get<fl::ecs::components::PartyMember>(actor);
  auto* stats = ctx.reg().try_get<fl::ecs::components::Stats>(actor);
  if (!member || !stats || !stats->is_alive()) return std::nullopt;
  auto& party = member->party().party_data();
  const int max_hp = effective_combat_stats(ctx.reg(), actor).max_hp;
  for (const auto item : party.items()) {
    const auto* consumable = ctx.reg().try_get<fl::ecs::components::Consumable>(item);
    if (!consumable || consumable->quantity <= 0 ||
        fl::consumables::category(consumable->kind) != fl::consumables::ConsumableCategory::Potion)
      continue;
    if (consumable->kind == fl::consumables::ConsumableKind::HealingPotion &&
        stats->hp_ * 100 <= max_hp * 35 && max_hp - stats->hp_ >= consumable->magnitude / 2)
      return item;
    if (consumable->kind == fl::consumables::ConsumableKind::HealingPotion) continue;
    auto* resources = ctx.reg().try_get<fl::ecs::components::CombatResources>(actor);
    if (!resources) continue;
    const auto resource = fl::consumables::potion_resource(consumable->kind);
    const auto& pool = resources->get(resource);
    if (pool.current >= pool.maximum) continue;
    for (const auto& equipped : member->closet().skill_slots) {
      if (equipped && fl::skills::definition(*equipped).resources.resource == resource &&
          !CombatResourceSystem::eligibility(ctx.reg(), actor, *equipped).eligible)
        return item;
    }
  }
  return std::nullopt;
}

bool ConsumableSystem::use_potion(fl::context::EncounterCtx& ctx, entt::entity actor,
                                  entt::entity item) {
  auto* member = ctx.reg().try_get<fl::ecs::components::PartyMember>(actor);
  auto* consumable = ctx.reg().try_get<fl::ecs::components::Consumable>(item);
  auto* stats = ctx.reg().try_get<fl::ecs::components::Stats>(actor);
  if (!member || !stats || !stats->is_alive() || !consumable || consumable->quantity <= 0 ||
      !party_owns(member->party().party_data(), item) ||
      fl::consumables::category(consumable->kind) != fl::consumables::ConsumableCategory::Potion)
    return false;
  int effective = 0;
  int wasted = 0;
  if (consumable->kind == fl::consumables::ConsumableKind::HealingPotion) {
    const int before = stats->hp_;
    stats->hp_ = std::min(effective_combat_stats(ctx.reg(), actor).max_hp,
                          stats->hp_ + consumable->magnitude);
    effective = stats->hp_ - before;
    wasted = consumable->magnitude - effective;
  } else {
    const auto resource = fl::consumables::potion_resource(consumable->kind);
    effective = CombatResourceSystem::modify(ctx, actor, resource, consumable->magnitude,
                                             fl::resources::ResourceChangeReason::Potion,
                                             fl::skills::SkillId::Observe);
    wasted = consumable->magnitude - effective;
  }
  const auto kind = consumable->kind;
  const int quantity_after = consumable->quantity - 1;
  ctx.encounter().combatant_bus(actor).emit(fl::events::CombatantEvent{
      fl::events::ConsumableUsed{actor, item, kind, quantity_after, effective, wasted}});
  ctx.log().append_markup(fmt::format("{} used [ability]({}); effective {}, wasted {}.",
                                     ctx.log().name_tag_for(entt::handle{ctx.reg(), actor}),
                                     fl::consumables::display_name(kind), effective, wasted));
  auto party_ctx = ctx.entity_context(ctx.self()).expect_party_ctx();
  consume_one(party_ctx, item);
  return true;
}

bool ConsumableSystem::prepare_flask(fl::context::PartyCtx& ctx, entt::entity actor,
                                     entt::entity item, std::uint64_t world_beat) {
  auto* member = ctx.reg().try_get<fl::ecs::components::PartyMember>(actor);
  auto* stats = ctx.reg().try_get<fl::ecs::components::Stats>(actor);
  auto* consumable = ctx.reg().try_get<fl::ecs::components::Consumable>(item);
  if (!member || !stats || !stats->is_alive() || !consumable || consumable->quantity <= 0 ||
      !party_owns(ctx.party_data(), item) ||
      fl::consumables::category(consumable->kind) != fl::consumables::ConsumableCategory::Flask)
    return false;
  const auto buff_kind = consumable->kind == fl::consumables::ConsumableKind::AttackFlask
                             ? fl::consumables::TimedBuffKind::Attack
                         : consumable->kind == fl::consumables::ConsumableKind::ArmorFlask
                             ? fl::consumables::TimedBuffKind::Armor
                             : fl::consumables::TimedBuffKind::AtbChargeRate;
  auto& buffs = ctx.reg().get_or_emplace<fl::ecs::components::TimedBuffs>(actor);
  auto existing = std::ranges::find_if(
      buffs.effects, [buff_kind](const auto& effect) { return effect.kind == buff_kind; });
  const bool replaced = existing != buffs.effects.end();
  const auto expires = world_beat + fl::primitives::WorldClock::beats_from_seconds(
                                        consumable->duration_seconds);
  const auto duration_beats = static_cast<std::uint64_t>(
      fl::primitives::WorldClock::beats_from_seconds(consumable->duration_seconds));
  const auto effect = fl::ecs::components::TimedBuff{buff_kind, consumable->magnitude, world_beat,
                                                      expires, duration_beats};
  if (replaced)
    *existing = effect;
  else
    buffs.effects.push_back(effect);
  emit_to_member(ctx.reg(), actor,
                 fl::events::CombatantEvent{fl::events::TimedBuffApplied{
                     actor, buff_kind, consumable->magnitude, expires, replaced}});
  consume_one(ctx, item);
  return true;
}

void ConsumableSystem::expire_timed_buffs(entt::registry& reg, std::uint64_t world_beat) {
  auto view = reg.view<fl::ecs::components::TimedBuffs>();
  for (const auto actor : view) {
    auto& effects = view.get<fl::ecs::components::TimedBuffs>(actor).effects;
    for (auto& effect : effects)
      effect.remaining_beats = effect.expires_world_beat > world_beat
                                   ? effect.expires_world_beat - world_beat
                                   : 0;
    for (const auto& effect : effects) {
      if (effect.expires_world_beat <= world_beat)
        emit_to_member(reg, actor, fl::events::CombatantEvent{fl::events::TimedBuffExpired{
                                       actor, effect.kind, effect.magnitude}});
    }
    std::erase_if(effects,
                  [world_beat](const auto& effect) { return effect.expires_world_beat <= world_beat; });
  }
}

} // namespace fl::ecs::systems
