#include "fl/primitives/raid_data.hpp"
#include "fl/ecs/systems/raid_loot.hpp"

#include "fl/ecs/components/field_debuff.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/entity_builder.hpp"
#include <stdexcept>

namespace fl::primitives {
RaidData::RaidData(fl::context::AccountCtx ctx, fl::events::RaidBus &events,
                   std::uint64_t id,
                   std::span<const fl::monster::MonsterKind> enemies)
    : ctx_(ctx), events_(events), id_(id), owner_(ctx.reg().create()),
      encounter_(ctx.reg(), ctx.rng(), ctx.log(), combat_bus_, owner_, false) {
  encounter_.mark_as_raid();
  encounter_.innervate_event_system();
  for (auto &party : ctx.account_data().parties()) {
    party.join_raid(encounter_);
    parties_.push_back(&party);
    for (const auto &member : party.members()) {
      const auto entity = member.member_id();
      encounter_.defenders().members().push_back(entity);
      encounter_.add_party_combatant_bus(entity);
      encounter_.atb_in().emit(
          seerin::AtbInEvent{seerin::AddCombatant{entity}});
    }
  }
  auto build = encounter_.context().build_context();
  for (auto kind : enemies) {
    auto enemy = EntityBuilder{build}.monster(kind).build();
    encounter_.attackers().members().push_back(enemy);
    encounter_.entities_to_cleanup().push_back(enemy);
    encounter_.add_enemy_combatant_bus(enemy);
    encounter_.atb_in().emit(seerin::AtbInEvent{seerin::AddCombatant{enemy}});
    for (auto *party : parties_)
      party->party_bus().emit(
          fl::events::PartyEvent{fl::events::MonsterEncountered{kind}});
  }
  // Forward discovery/observation facts to every participating party's
  // observers. The callback is owned by this bus, whose lifetime is bounded by
  // RaidData.
  witness_sub_ = fl::events::ScopedPartyListener{
      combat_bus_, std::in_place_type<fl::events::SkillWitnessed>,
      [this](const auto &event) {
        for (auto *party : parties_)
          party->party_bus().emit(fl::events::PartyEvent{event});
      }};
  ctx.log().append_plain(
      "The Visitor has arrived. All parties are summoned to the raid.");
}

void RaidData::announce_started() {
  if (announced_)
    return;
  announced_ = true;
  events_.emit(fl::events::RaidEvent{
      fl::events::RaidStarted{id_, ctx_.self(), parties_.size()}});
}

RaidData::~RaidData() {
  cleanup();
  for (auto *party : parties_)
    party->detach_raid();
  if (ctx_.reg().valid(owner_))
    ctx_.reg().destroy(owner_);
}

void RaidData::cleanup() {
  if (cleaned_)
    return;
  cleaned_ = true;
  combat_bus_.emit(fl::events::PartyEvent{fl::events::PartyLeftCombat{}});
  encounter_.clear_pending_events();
  if (auto *field =
          ctx_.reg().try_get<fl::ecs::components::FieldDebuffs>(owner_)) {
    for (auto &effect : field->effects)
      if (ctx_.reg().valid(effect.effect_id))
        ctx_.reg().destroy(effect.effect_id);
    ctx_.reg().remove<fl::ecs::components::FieldDebuffs>(owner_);
  }
  encounter_.finalize();
}

void RaidData::tick() {
  if (!active())
    return;
  auto &ctx = encounter_.context();
  if (encounter_.defenders().has_alive_members(ctx) &&
      encounter_.has_alive_enemies()) {
    ++combat_beats_;
    encounter_.atb_in().emit(seerin::Beat{});
  }
  // Decide after the complete beat, so simultaneous effects cannot announce an
  // early victory. Individual party wipes never tear this encounter down.
  const bool players = encounter_.defenders().has_alive_members(ctx);
  const bool enemies = encounter_.has_alive_enemies();
  if (!players)
    resolve(enemies ? fl::events::RaidResult::Defeat
                    : fl::events::RaidResult::MutualDestruction);
  else if (!enemies)
    resolve(fl::events::RaidResult::Victory);
}

void RaidData::resolve(fl::events::RaidResult result) {
  if (!active())
    return;
  result_ = result;
  cleanup();
  const bool victory = result == fl::events::RaidResult::Victory;
  if (victory) fl::ecs::systems::RaidLootSystem::commit(ctx_, events_, id_, parties_);
  for (auto *party : parties_)
    party->resolve_raid(victory);
  ctx_.log().append_plain(victory ? "Raid victory!"
                                  : "Raid wiped. Await the next Visitor.");
  events_.emit(fl::events::RaidEvent{fl::events::RaidResolved{
      id_, ctx_.self(), result, parties_.size(), combat_beats_}});
}
} // namespace fl::primitives
