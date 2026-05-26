#include <catch2/catch_test_macros.hpp>

#include <optional>

#include "fl/context.hpp"
#include "fl/ecs/components/atb_charge.hpp"
#include "fl/ecs/components/dire_bleed.hpp"
#include "fl/ecs/components/freeze.hpp"
#include "fl/ecs/components/hp_bar_color_override.hpp"
#include "fl/ecs/components/monster_identity.hpp"
#include "fl/ecs/components/poison.hpp"
#include "fl/ecs/components/skill_slots.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/visual_effects.hpp"
#include "fl/ecs/systems/take_damage.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/grand_central.hpp"
#include "fl/lospec500.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "fl/primitives/entity_builder.hpp"
#include "fl/primitives/world_clock.hpp"
#include "fl/skills/skill_learning.hpp"
#include "fl/skills/skill_sequence.hpp"
#include "sr/atb_events.hpp"

namespace {

void tick_party(fl::context::PartyCtx &party_ctx, int beats) {
  for (int i = 0; i < beats; ++i) {
    party_ctx.bus().emit(fl::events::PartyEvent{fl::events::PartyTick{}});
  }
}

entt::entity add_honey_badger(fl::context::PartyCtx &party_ctx,
                              fl::primitives::EncounterData &encounter) {
  auto build_ctx = party_ctx.build_context();
  auto honey_badger = fl::primitives::EntityBuilder(build_ctx)
                          .monster(fl::monster::MonsterKind::HoneyBadger)
                          .build();
  encounter.attackers().members().push_back(honey_badger);
  encounter.entities_to_cleanup().push_back(honey_badger);
  encounter.add_enemy_combatant_bus(honey_badger);
  encounter.atb_in().emit(
      seerin::AtbInEvent{seerin::AddCombatant{honey_badger}});
  return honey_badger;
}

entt::entity find_dire_bleed_target(fl::context::PartyCtx &party_ctx) {
  for (const auto &member : party_ctx.party_data().members()) {
    if (party_ctx.reg().all_of<fl::ecs::components::DireBleed>(
            member.member_id())) {
      return member.member_id();
    }
  }

  return entt::null;
}

entt::entity find_poison_target(fl::context::PartyCtx &party_ctx) {
  for (const auto &member : party_ctx.party_data().members()) {
    if (party_ctx.reg().all_of<fl::ecs::components::Poison>(
            member.member_id())) {
      return member.member_id();
    }
  }

  return entt::null;
}

} // namespace

TEST_CASE("EncounterBuilder::thump_it_out returns the created EncounterData",
          "[encounter][builder]") {
  fl::GrandCentral gc{1, 1, 3};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);

  auto &encounter = builder.thump_it_out();

  REQUIRE(&encounter == &party_ctx.party_data().encounter_data());
}

TEST_CASE("EncounterBuilder::thump_it_out enrolls party members as defenders",
          "[encounter][builder]") {
  fl::GrandCentral gc{1, 1, 3};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);

  auto &encounter = builder.thump_it_out();

  REQUIRE(encounter.defenders().members().size() == 3);
}

TEST_CASE("EncounterBuilder::thump_it_out adds a full enemy party",
          "[encounter][builder]") {
  fl::GrandCentral gc{1, 1, 3};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);

  auto &encounter = builder.thump_it_out();

  REQUIRE(encounter.attackers().members().size() ==
          fl::primitives::EncounterBuilder::kEnemyPartySize);
  REQUIRE(encounter.entities_to_cleanup().size() ==
          fl::primitives::EncounterBuilder::kEnemyPartySize);

  for (auto attacker : encounter.attackers().members()) {
    REQUIRE(encounter.owns_entity(attacker));
  }
}

TEST_CASE("TakeDamage emits PlayerDied when a party member dies",
          "[encounter][events][death]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);
  auto &encounter = builder.thump_it_out();

  const auto attacker = encounter.attackers().members().front();
  const auto defender = encounter.defenders().members().front();

  bool saw_player_died = false;
  entt::entity seen_player = entt::null;
  entt::entity seen_killer = entt::null;

  (void)party_ctx.party_data()
      .encounter_data()
      .combatant_bus(defender)
      .on<fl::events::PlayerDied>([&](const fl::events::PlayerDied &ev) {
        saw_player_died = true;
        seen_player = ev.player;
        seen_killer = ev.killer;
      });

  auto attack_ctx =
      fl::context::AttackCtx::make_attack(party_ctx, attacker, defender);
  attack_ctx.damage().physical = 9999;
  fl::ecs::systems::TakeDamage::commit(attack_ctx);

  REQUIRE(saw_player_died);
  REQUIRE(seen_player == defender);
  REQUIRE(seen_killer == attacker);
}

TEST_CASE("TakeDamage requests loot when a player defender kills an enemy",
          "[encounter][events][loot]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);
  auto &encounter = builder.thump_it_out();

  const auto attacker = encounter.defenders().members().front();
  const auto defender = encounter.attackers().members().front();

  bool saw_loot_drop = false;
  entt::entity seen_source = entt::null;
  entt::entity seen_party = entt::null;

  (void)party_ctx.party_data().party_bus().on<fl::events::LootDropRequested>(
      [&](const fl::events::LootDropRequested &ev) {
        saw_loot_drop = true;
        seen_source = ev.source;
        seen_party = ev.party;
      });

  auto attack_ctx =
      fl::context::AttackCtx::make_attack(party_ctx, attacker, defender);
  attack_ctx.damage().physical = 9999;
  fl::ecs::systems::TakeDamage::commit(attack_ctx);

  REQUIRE(saw_loot_drop);
  REQUIRE(seen_source == defender);
  REQUIRE(seen_party == party_ctx.self());
}

TEST_CASE("TakeDamage emits PartyWiped when the final party member dies",
          "[encounter][events][death]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);
  auto &encounter = builder.thump_it_out();

  const auto attacker = encounter.attackers().members().front();
  const auto defender = encounter.defenders().members().front();

  bool saw_party_wiped = false;

  (void)party_ctx.party_data().party_bus().on<fl::events::PartyWiped>(
      [&](const fl::events::PartyWiped &) { saw_party_wiped = true; });

  auto attack_ctx =
      fl::context::AttackCtx::make_attack(party_ctx, attacker, defender);
  attack_ctx.damage().physical = 9999;
  fl::ecs::systems::TakeDamage::commit(attack_ctx);

  REQUIRE(saw_party_wiped);
}

TEST_CASE(
    "TakeDamage marks dead player state and clears pending encounter events",
    "[encounter][events][death]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);
  auto &encounter = builder.thump_it_out();

  const auto attacker = encounter.attackers().members().front();
  const auto defender = encounter.defenders().members().front();

  encounter.atb_out().emit(seerin::AtbOutEvent{seerin::BecameActive{
      attacker,
  }});
  REQUIRE(encounter.pending_scheduled_events() > 0);

  auto attack_ctx =
      fl::context::AttackCtx::make_attack(party_ctx, attacker, defender);
  attack_ctx.damage().physical = 9999;
  fl::ecs::systems::TakeDamage::commit(attack_ctx);

  REQUIRE(encounter.pending_scheduled_events() == 0);
  const auto &stats = party_ctx.reg().get<fl::ecs::components::Stats>(defender);
  REQUIRE(stats.hp_ == 0);
}

TEST_CASE("Encounter skill choice reads SkillSlots instead of MonsterIdentity",
          "[encounter][skills]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();
  auto &encounter = party.create_encounter();

  auto honey_badger = add_honey_badger(party_ctx, encounter);
  party_ctx.reg().emplace_or_replace<fl::ecs::components::SkillSlots>(
      honey_badger,
      fl::ecs::components::SkillSlots::with_known(fl::skills::SkillId::Smack));

  REQUIRE(party_ctx.reg()
              .get<fl::ecs::components::MonsterIdentity>(honey_badger)
              .kind == fl::monster::MonsterKind::HoneyBadger);
  REQUIRE(encounter.choose_skill(honey_badger) == fl::skills::SkillId::Smack);
}

TEST_CASE("Party wipes across different parties do not crash and leave combat",
          "[encounter][events][death][multi_party]") {
  fl::GrandCentral gc{1, 5, 1};

  auto account_ctx = gc.account_context(0);

  for (std::size_t i = 0; i < account_ctx.account_data().parties().size();
       ++i) {
    auto party_ctx = account_ctx.party_context(i);

    fl::primitives::EncounterBuilder builder(party_ctx);
    auto &encounter = builder.thump_it_out();

    const auto attacker = encounter.attackers().members().front();
    const auto defender = encounter.defenders().members().front();

    auto attack_ctx =
        fl::context::AttackCtx::make_attack(party_ctx, attacker, defender);
    attack_ctx.damage().physical = 9999;
    fl::ecs::systems::TakeDamage::commit(attack_ctx);

    REQUIRE_FALSE(party_ctx.party_data().in_combat());

    const auto &stats =
        party_ctx.reg().get<fl::ecs::components::Stats>(defender);
    REQUIRE(stats.hp_ == 0);
    REQUIRE(party_ctx.party_data().town_penalty_active());
  }
}

TEST_CASE(
    "Honey Badger Eviscerate applies Dire Bleed that ticks for 10 percent",
    "[encounter][skills][bleed]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();
  auto &encounter = party.create_encounter();

  const auto defender = party.members().front().member_id();
  encounter.defenders().members().push_back(defender);
  encounter.add_party_combatant_bus(defender);
  encounter.atb_in().emit(seerin::AtbInEvent{seerin::AddCombatant{defender}});

  auto honey_badger = add_honey_badger(party_ctx, encounter);
  REQUIRE(party_ctx.reg()
              .get<fl::ecs::components::MonsterIdentity>(honey_badger)
              .kind == fl::monster::MonsterKind::HoneyBadger);

  auto &stats = party_ctx.reg().get<fl::ecs::components::Stats>(defender);
  stats.max_hp_ = 100;
  stats.hp_ = 100;

  encounter.atb_out().emit(seerin::AtbOutEvent{seerin::BecameActive{
      honey_badger,
  }});
  tick_party(party_ctx, 24);

  REQUIRE(party_ctx.reg().all_of<fl::ecs::components::DireBleed>(defender));
  REQUIRE(party_ctx.reg().all_of<fl::ecs::components::StatusTint>(defender));
  REQUIRE(party_ctx.reg()
              .get<fl::ecs::components::DireBleed>(defender)
              .damage_per_tick == 10);

  tick_party(party_ctx, 35);
  REQUIRE(stats.hp_ == 100);

  tick_party(party_ctx, 1);
  REQUIRE(stats.hp_ == 90);
}

TEST_CASE("Dire Bleed releases the active combatant when the effect is applied",
          "[encounter][skills][bleed][atb]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();
  auto &encounter = party.create_encounter();

  const auto defender = party.members().front().member_id();
  encounter.defenders().members().push_back(defender);
  encounter.add_party_combatant_bus(defender);
  encounter.atb_in().emit(seerin::AtbInEvent{seerin::AddCombatant{defender}});

  auto honey_badger = add_honey_badger(party_ctx, encounter);

  auto &stats = party_ctx.reg().get<fl::ecs::components::Stats>(defender);
  stats.max_hp_ = 100;
  stats.hp_ = 100;

  encounter.atb_engine().active_combatant() = honey_badger;
  encounter.atb_out().emit(seerin::AtbOutEvent{seerin::BecameActive{
      honey_badger,
  }});

  tick_party(party_ctx, 23);
  REQUIRE(encounter.atb_engine().active_combatant() == honey_badger);
  REQUIRE_FALSE(
      party_ctx.reg().any_of<fl::ecs::components::DireBleed>(defender));

  tick_party(party_ctx, 1);
  REQUIRE(encounter.atb_engine().active_combatant() == entt::entity{});
  REQUIRE(party_ctx.reg().all_of<fl::ecs::components::DireBleed>(defender));

  tick_party(party_ctx, fl::primitives::WorldClock::beats_from_seconds(3));
  REQUIRE(stats.hp_ == 90);
}

TEST_CASE("Dire Bleed is removed and pending target work is cleared on death",
          "[encounter][skills][bleed][death]") {
  fl::GrandCentral gc{1, 1, 2};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();
  auto &encounter = party.create_encounter();

  for (const auto &member : party.members()) {
    encounter.defenders().members().push_back(member.member_id());
    encounter.atb_in().emit(
        seerin::AtbInEvent{seerin::AddCombatant{member.member_id()}});
  }

  auto honey_badger = add_honey_badger(party_ctx, encounter);
  encounter.atb_out().emit(seerin::AtbOutEvent{seerin::BecameActive{
      honey_badger,
  }});
  tick_party(party_ctx, 24);
  const auto defender = find_dire_bleed_target(party_ctx);
  REQUIRE(party_ctx.reg().valid(defender));
  REQUIRE(party_ctx.reg().all_of<fl::ecs::components::DireBleed>(defender));

  auto attack_ctx =
      fl::context::AttackCtx::make_attack(party_ctx, honey_badger, defender);
  attack_ctx.damage().physical = 9999;
  fl::ecs::systems::TakeDamage::commit(attack_ctx);

  REQUIRE_FALSE(
      party_ctx.reg().any_of<fl::ecs::components::DireBleed>(defender));
  REQUIRE_FALSE(
      party_ctx.reg().any_of<fl::ecs::components::StatusTint>(defender));
  REQUIRE(party_ctx.reg().get<fl::ecs::components::Stats>(defender).hp_ == 0);

  tick_party(party_ctx, 36);
  REQUIRE_FALSE(
      party_ctx.reg().any_of<fl::ecs::components::DireBleed>(defender));
  REQUIRE_FALSE(
      party_ctx.reg().any_of<fl::ecs::components::StatusTint>(defender));
  REQUIRE(party_ctx.reg().get<fl::ecs::components::Stats>(defender).hp_ == 0);
}

TEST_CASE("Dire Bleed is removed when the party leaves combat",
          "[encounter][skills][bleed][cleanup]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();
  auto &encounter = party.create_encounter();

  const auto defender = party.members().front().member_id();
  encounter.defenders().members().push_back(defender);
  encounter.add_party_combatant_bus(defender);
  encounter.atb_in().emit(seerin::AtbInEvent{seerin::AddCombatant{defender}});

  auto honey_badger = add_honey_badger(party_ctx, encounter);
  encounter.atb_out().emit(seerin::AtbOutEvent{seerin::BecameActive{
      honey_badger,
  }});
  tick_party(party_ctx, 24);
  REQUIRE(party_ctx.reg().all_of<fl::ecs::components::DireBleed>(defender));
  REQUIRE(party_ctx.reg().all_of<fl::ecs::components::StatusTint>(defender));

  party.leave_combat();

  REQUIRE_FALSE(
      party_ctx.reg().any_of<fl::ecs::components::DireBleed>(defender));
  REQUIRE_FALSE(
      party_ctx.reg().any_of<fl::ecs::components::StatusTint>(defender));
  REQUIRE_FALSE(party.in_combat());
}

TEST_CASE("Dire Bleed clears safely if the source is gone before a tick",
          "[encounter][skills][bleed][cleanup]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();
  auto &encounter = party.create_encounter();

  const auto defender = party.members().front().member_id();
  encounter.defenders().members().push_back(defender);
  encounter.add_party_combatant_bus(defender);
  encounter.atb_in().emit(seerin::AtbInEvent{seerin::AddCombatant{defender}});

  auto honey_badger = add_honey_badger(party_ctx, encounter);
  encounter.atb_out().emit(seerin::AtbOutEvent{seerin::BecameActive{
      honey_badger,
  }});
  tick_party(party_ctx, 24);
  REQUIRE(party_ctx.reg().all_of<fl::ecs::components::DireBleed>(defender));

  party_ctx.reg().destroy(honey_badger);
  tick_party(party_ctx, fl::primitives::WorldClock::beats_from_seconds(5));

  REQUIRE_FALSE(
      party_ctx.reg().any_of<fl::ecs::components::DireBleed>(defender));
  REQUIRE_FALSE(
      party_ctx.reg().any_of<fl::ecs::components::StatusTint>(defender));
}

TEST_CASE("Poison applies flat damage every three seconds for its duration",
          "[encounter][skills][poison]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();
  auto &encounter = party.create_encounter();

  const auto defender = party.members().front().member_id();
  encounter.defenders().members().push_back(defender);
  encounter.add_party_combatant_bus(defender);
  encounter.atb_in().emit(seerin::AtbInEvent{seerin::AddCombatant{defender}});

  auto build_ctx = party_ctx.build_context();
  auto source = fl::primitives::EntityBuilder(build_ctx)
                    .monster(fl::monster::MonsterKind::HoneyBadger)
                    .build();
  auto &stats = party_ctx.reg().get<fl::ecs::components::Stats>(defender);
  stats.max_hp_ = 100;
  stats.hp_ = 100;

  party_ctx.party_data().encounter_data().combatant_bus(defender).emit(
      fl::events::CombatantEvent{
          fl::events::PoisonApplied{source, defender, 1, 27}});

  REQUIRE(party_ctx.reg().all_of<fl::ecs::components::Poison>(defender));
  REQUIRE(party_ctx.reg().all_of<fl::ecs::components::StatusTint>(defender));
  REQUIRE(party_ctx.reg()
              .get<fl::ecs::components::StatusTint>(defender)
              .hp_bar_color == fl::lospec500::color_at(22));
  REQUIRE(party_ctx.reg()
              .get<fl::ecs::components::Poison>(defender)
              .damage_per_tick == 1);
  REQUIRE(party_ctx.reg()
              .get<fl::ecs::components::Poison>(defender)
              .ticks_remaining == 9);

  tick_party(party_ctx, fl::primitives::WorldClock::beats_from_seconds(1));
  REQUIRE(party_ctx.reg()
              .get<fl::ecs::components::StatusTint>(defender)
              .hp_bar_color == fl::lospec500::color_at(20));

  tick_party(party_ctx, fl::primitives::WorldClock::beats_from_seconds(1));
  REQUIRE(party_ctx.reg()
              .get<fl::ecs::components::StatusTint>(defender)
              .hp_bar_color == fl::lospec500::color_at(22));

  tick_party(party_ctx, fl::primitives::WorldClock::beats_from_seconds(1));
  REQUIRE(stats.hp_ == 99);
  REQUIRE(
      party_ctx.reg().any_of<fl::ecs::components::CombatantDecals>(defender));
  const auto &poison_tick_decals =
      party_ctx.reg().get<fl::ecs::components::CombatantDecals>(defender);
  REQUIRE(poison_tick_decals.effects.back().animation_kind ==
          fl::widgets::effects::DecalAnimationKind::HitpointNumber);
  REQUIRE(poison_tick_decals.effects.back().config.color ==
          fl::lospec500::color_at(22));
  REQUIRE(party_ctx.reg()
              .get<fl::ecs::components::Poison>(defender)
              .ticks_remaining == 8);

  tick_party(party_ctx, fl::primitives::WorldClock::beats_from_seconds(5));
  REQUIRE(stats.hp_ == 98);
  REQUIRE(party_ctx.reg()
              .get<fl::ecs::components::Poison>(defender)
              .ticks_remaining == 7);

  tick_party(party_ctx, fl::primitives::WorldClock::beats_from_seconds(19));
  REQUIRE(stats.hp_ == 91);
  REQUIRE_FALSE(party_ctx.reg().any_of<fl::ecs::components::Poison>(defender));
  REQUIRE_FALSE(
      party_ctx.reg().any_of<fl::ecs::components::StatusTint>(defender));
}

TEST_CASE("Poison releases the active combatant when the effect is applied",
          "[encounter][skills][poison][atb]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();
  auto &encounter = party.create_encounter();

  const auto defender = party.members().front().member_id();
  encounter.defenders().members().push_back(defender);
  encounter.add_party_combatant_bus(defender);
  encounter.atb_in().emit(seerin::AtbInEvent{seerin::AddCombatant{defender}});

  auto source = add_honey_badger(party_ctx, encounter);
  party_ctx.reg().emplace_or_replace<fl::ecs::components::SkillSlots>(
      source,
      fl::ecs::components::SkillSlots::with_known(fl::skills::SkillId::Poison));

  auto &stats = party_ctx.reg().get<fl::ecs::components::Stats>(defender);
  stats.max_hp_ = 100;
  stats.hp_ = 100;

  encounter.atb_engine().active_combatant() = source;
  encounter.atb_out().emit(seerin::AtbOutEvent{seerin::BecameActive{
      source,
  }});

  tick_party(party_ctx, 23);
  REQUIRE(encounter.atb_engine().active_combatant() == source);
  REQUIRE_FALSE(party_ctx.reg().any_of<fl::ecs::components::Poison>(defender));

  tick_party(party_ctx, 1);
  REQUIRE(encounter.atb_engine().active_combatant() == entt::entity{});
  REQUIRE(party_ctx.reg().all_of<fl::ecs::components::Poison>(defender));

  tick_party(party_ctx, fl::primitives::WorldClock::beats_from_seconds(3));
  REQUIRE(stats.hp_ == 99);
}

TEST_CASE("Poison is applied by event and clears when combat ends",
          "[encounter][skills][poison][cleanup]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();
  auto &encounter = party.create_encounter();

  const auto defender = party.members().front().member_id();
  encounter.defenders().members().push_back(defender);
  encounter.add_party_combatant_bus(defender);
  encounter.atb_in().emit(seerin::AtbInEvent{seerin::AddCombatant{defender}});

  auto build_ctx = party_ctx.build_context();
  auto source = fl::primitives::EntityBuilder(build_ctx)
                    .monster(fl::monster::MonsterKind::HoneyBadger)
                    .build();
  party_ctx.party_data().encounter_data().combatant_bus(defender).emit(
      fl::events::CombatantEvent{
          fl::events::PoisonApplied{source, defender, 1, 27}});

  REQUIRE(find_poison_target(party_ctx) == defender);

  party.leave_combat();

  REQUIRE_FALSE(party_ctx.reg().any_of<fl::ecs::components::Poison>(defender));
  REQUIRE_FALSE(
      party_ctx.reg().any_of<fl::ecs::components::StatusTint>(defender));
}

TEST_CASE("Freeze applies a frozen background and expires after thirty seconds",
          "[encounter][skills][freeze]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();
  auto &encounter = party.create_encounter();

  const auto defender = party.members().front().member_id();
  encounter.defenders().members().push_back(defender);
  encounter.add_party_combatant_bus(defender);
  encounter.atb_in().emit(seerin::AtbInEvent{seerin::AddCombatant{defender}});

  auto build_ctx = party_ctx.build_context();
  auto source = fl::primitives::EntityBuilder(build_ctx)
                    .monster(fl::monster::MonsterKind::HoneyBadger)
                    .build();
  party_ctx.party_data().encounter_data().combatant_bus(defender).emit(
      fl::events::CombatantEvent{
          fl::events::FreezeApplied{source, defender, 30}});

  REQUIRE(party_ctx.reg().all_of<fl::ecs::components::Freeze>(defender));
  REQUIRE(party_ctx.reg()
              .get<fl::ecs::components::Freeze>(defender)
              .clear_after_beats ==
          fl::primitives::WorldClock::beats_from_seconds(30));
  REQUIRE(party_ctx.reg().all_of<fl::ecs::components::StatusTint>(defender));
  REQUIRE(party_ctx.reg()
              .get<fl::ecs::components::StatusTint>(defender)
              .background_color == fl::lospec500::color_at(28));

  tick_party(party_ctx, fl::primitives::WorldClock::kBeatsPerSecond / 4);
  REQUIRE(party_ctx.reg()
              .get<fl::ecs::components::StatusTint>(defender)
              .background_color == fl::lospec500::color_at(16));

  tick_party(party_ctx, fl::primitives::WorldClock::beats_from_seconds(29));
  REQUIRE(party_ctx.reg().all_of<fl::ecs::components::Freeze>(defender));
  REQUIRE(party_ctx.reg()
              .get<fl::ecs::components::StatusTint>(defender)
              .background_color == fl::lospec500::color_at(16));

  tick_party(party_ctx, fl::primitives::WorldClock::beats_from_seconds(1));
  REQUIRE_FALSE(party_ctx.reg().any_of<fl::ecs::components::Freeze>(defender));
  REQUIRE_FALSE(
      party_ctx.reg().any_of<fl::ecs::components::StatusTint>(defender));
}

TEST_CASE("Cold Snap releases the active combatant when freeze is applied",
          "[encounter][skills][freeze][atb]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();
  auto &encounter = party.create_encounter();

  const auto defender = party.members().front().member_id();
  encounter.defenders().members().push_back(defender);
  encounter.add_party_combatant_bus(defender);
  encounter.atb_in().emit(seerin::AtbInEvent{seerin::AddCombatant{defender}});

  auto source = add_honey_badger(party_ctx, encounter);
  party_ctx.reg().emplace_or_replace<fl::ecs::components::SkillSlots>(
      source, fl::ecs::components::SkillSlots::with_known(
                  fl::skills::SkillId::ColdSnap));

  encounter.atb_engine().active_combatant() = source;
  encounter.atb_out().emit(seerin::AtbOutEvent{seerin::BecameActive{
      source,
  }});

  tick_party(party_ctx, 23);
  REQUIRE(encounter.atb_engine().active_combatant() == source);
  REQUIRE_FALSE(party_ctx.reg().any_of<fl::ecs::components::Freeze>(defender));

  tick_party(party_ctx, 1);
  REQUIRE(encounter.atb_engine().active_combatant() == entt::entity{});
  REQUIRE(party_ctx.reg().all_of<fl::ecs::components::Freeze>(defender));

  const auto frozen_charge =
      party_ctx.reg().get<fl::ecs::components::AtbCharge>(defender).charge;
  tick_party(party_ctx, 2);
  REQUIRE(
      party_ctx.reg().get<fl::ecs::components::AtbCharge>(defender).charge ==
      frozen_charge);
}

TEST_CASE("Freeze stops ATB accrual until it expires",
          "[encounter][skills][freeze][atb]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();
  auto &encounter = party.create_encounter();

  const auto defender = party.members().front().member_id();
  encounter.defenders().members().push_back(defender);
  encounter.add_party_combatant_bus(defender);
  encounter.atb_in().emit(seerin::AtbInEvent{seerin::AddCombatant{defender}});

  auto build_ctx = party_ctx.build_context();
  auto source = fl::primitives::EntityBuilder(build_ctx)
                    .monster(fl::monster::MonsterKind::HoneyBadger)
                    .build();
  party_ctx.party_data().encounter_data().combatant_bus(defender).emit(
      fl::events::CombatantEvent{
          fl::events::FreezeApplied{source, defender, 30}});

  tick_party(party_ctx, 2);
  REQUIRE(
      party_ctx.reg().get<fl::ecs::components::AtbCharge>(defender).charge ==
      0);

  tick_party(party_ctx, fl::primitives::WorldClock::beats_from_seconds(29));
  REQUIRE(party_ctx.reg().any_of<fl::ecs::components::Freeze>(defender));
  REQUIRE(
      party_ctx.reg().get<fl::ecs::components::AtbCharge>(defender).charge ==
      0);

  tick_party(party_ctx, fl::primitives::WorldClock::beats_from_seconds(1));
  REQUIRE_FALSE(party_ctx.reg().any_of<fl::ecs::components::Freeze>(defender));

  tick_party(party_ctx, 1);
  REQUIRE(party_ctx.reg().get<fl::ecs::components::AtbCharge>(defender).charge >
          0);
}

TEST_CASE("Second Freeze application shatters for half max HP",
          "[encounter][skills][freeze][shatter]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();
  auto &encounter = party.create_encounter();

  const auto defender = party.members().front().member_id();
  encounter.defenders().members().push_back(defender);
  encounter.add_party_combatant_bus(defender);
  encounter.atb_in().emit(seerin::AtbInEvent{seerin::AddCombatant{defender}});

  auto source = add_honey_badger(party_ctx, encounter);
  auto &stats = party_ctx.reg().get<fl::ecs::components::Stats>(defender);
  stats.max_hp_ = 100;
  stats.hp_ = 100;

  party_ctx.party_data().encounter_data().combatant_bus(defender).emit(
      fl::events::CombatantEvent{
          fl::events::FreezeApplied{source, defender, 30}});
  REQUIRE(stats.hp_ == 100);
  REQUIRE(party_ctx.reg().all_of<fl::ecs::components::Freeze>(defender));

  party_ctx.party_data().encounter_data().combatant_bus(defender).emit(
      fl::events::CombatantEvent{
          fl::events::FreezeApplied{source, defender, 30}});

  REQUIRE(stats.hp_ == 50);
  REQUIRE_FALSE(party_ctx.reg().any_of<fl::ecs::components::Freeze>(defender));
  REQUIRE_FALSE(
      party_ctx.reg().any_of<fl::ecs::components::StatusTint>(defender));
}

TEST_CASE("Observe can teach an eligible party member",
          "[encounter][skills][learning]") {
  fl::GrandCentral gc{1, 1, 2};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &members = party_ctx.party_data().members();

  const auto user = members.front().member_id();
  const auto observer = members.back().member_id();

  REQUIRE(fl::skills::learn_observed_skill_with_roll(
      party_ctx, observer, user, fl::skills::SkillId::Thump, 1));
  REQUIRE(party_ctx.reg().get<fl::ecs::components::SkillSlots>(observer).knows(
      fl::skills::SkillId::Thump));
}

TEST_CASE("Observe itself is not learned by observation",
          "[encounter][skills][learning]") {
  fl::GrandCentral gc{1, 1, 2};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &members = party_ctx.party_data().members();

  const auto user = members.front().member_id();
  const auto observer = members.back().member_id();
  auto &slots = party_ctx.reg().get<fl::ecs::components::SkillSlots>(observer);
  slots.slots.fill(std::nullopt);

  REQUIRE_FALSE(fl::skills::learn_observed_skill_with_roll(
      party_ctx, observer, user, fl::skills::SkillId::Observe, 1));
  REQUIRE_FALSE(slots.knows(fl::skills::SkillId::Observe));
}

TEST_CASE("Skills learned this combat are rolled back on party wipe",
          "[encounter][skills][learning][wipe]") {
  fl::GrandCentral gc{1, 1, 2};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();
  auto &members = party.members();

  fl::primitives::EncounterBuilder builder(party_ctx);
  (void)builder.thump_it_out();

  const auto user = members.front().member_id();
  const auto observer = members.back().member_id();
  auto &slots = party_ctx.reg().get<fl::ecs::components::SkillSlots>(observer);

  REQUIRE(fl::skills::learn_observed_skill_with_roll(
      party_ctx, observer, user, fl::skills::SkillId::Thump, 1));
  REQUIRE(slots.knows(fl::skills::SkillId::Thump));

  for (const auto &member : members) {
    party_ctx.reg().get<fl::ecs::components::Stats>(member.member_id()).hp_ = 0;
  }

  const auto log_size_before_wipe = party.log().size();

  party.party_bus().emit(fl::events::PartyEvent{fl::events::PartyWiped{}});

  REQUIRE_FALSE(slots.knows(fl::skills::SkillId::Thump));
  REQUIRE(party.log().size() > log_size_before_wipe);
}

TEST_CASE("Skills learned this combat persist after successful combat exit",
          "[encounter][skills][learning][victory]") {
  fl::GrandCentral gc{1, 1, 2};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();
  auto &members = party.members();

  fl::primitives::EncounterBuilder builder(party_ctx);
  (void)builder.thump_it_out();

  const auto user = members.front().member_id();
  const auto observer = members.back().member_id();
  auto &slots = party_ctx.reg().get<fl::ecs::components::SkillSlots>(observer);

  REQUIRE(fl::skills::learn_observed_skill_with_roll(
      party_ctx, observer, user, fl::skills::SkillId::Thump, 1));
  REQUIRE(slots.knows(fl::skills::SkillId::Thump));

  party.leave_combat();

  REQUIRE(slots.knows(fl::skills::SkillId::Thump));
}

TEST_CASE("Later combat wipe does not remove skill learned in prior victory",
          "[encounter][skills][learning][subscription-lifecycle]") {
  fl::GrandCentral gc{1, 1, 2};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();
  auto &members = party.members();

  fl::primitives::EncounterBuilder first_builder(party_ctx);
  (void)first_builder.thump_it_out();

  const auto user = members.front().member_id();
  const auto observer = members.back().member_id();
  auto &slots = party_ctx.reg().get<fl::ecs::components::SkillSlots>(observer);

  REQUIRE(fl::skills::learn_observed_skill_with_roll(
      party_ctx, observer, user, fl::skills::SkillId::Thump, 1));
  REQUIRE(slots.knows(fl::skills::SkillId::Thump));

  party.leave_combat();
  REQUIRE(slots.knows(fl::skills::SkillId::Thump));

  fl::primitives::EncounterBuilder second_builder(party_ctx);
  (void)second_builder.thump_it_out();

  for (const auto &member : members) {
    party_ctx.reg().get<fl::ecs::components::Stats>(member.member_id()).hp_ = 0;
  }

  party.party_bus().emit(fl::events::PartyEvent{fl::events::PartyWiped{}});

  REQUIRE(slots.knows(fl::skills::SkillId::Thump));
}

TEST_CASE(
    "Dire Bleed survives active-turn cleanup for the affected participant",
    "[encounter][skills][bleed][lifetime]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();
  auto &encounter = party.create_encounter();

  const auto defender = party.members().front().member_id();
  encounter.defenders().members().push_back(defender);
  encounter.add_party_combatant_bus(defender);
  encounter.atb_in().emit(seerin::AtbInEvent{seerin::AddCombatant{defender}});

  auto honey_badger = add_honey_badger(party_ctx, encounter);
  auto &stats = party_ctx.reg().get<fl::ecs::components::Stats>(defender);
  stats.max_hp_ = 100;
  stats.hp_ = 100;

  fl::skills::SkillSequencer sequencer{
      party_ctx, encounter.atb_engine().scheduler(), [](entt::entity) {}};
  sequencer.schedule(honey_badger, defender, fl::skills::SkillId::Eviscerate);
  tick_party(party_ctx, 24);
  REQUIRE(party_ctx.reg().all_of<fl::ecs::components::DireBleed>(defender));

  encounter.atb_engine().active_combatant() = defender;
  encounter.clear_active_turn_for(defender);
  REQUIRE(encounter.atb_engine().active_combatant() == entt::entity{});

  tick_party(party_ctx, fl::primitives::WorldClock::beats_from_seconds(3));
  REQUIRE(stats.hp_ == 90);
}

TEST_CASE("Poison survives active-turn cleanup for the affected participant",
          "[encounter][skills][poison][lifetime]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();
  auto &encounter = party.create_encounter();

  const auto defender = party.members().front().member_id();
  encounter.defenders().members().push_back(defender);
  encounter.add_party_combatant_bus(defender);
  encounter.atb_in().emit(seerin::AtbInEvent{seerin::AddCombatant{defender}});

  auto source = add_honey_badger(party_ctx, encounter);
  auto &stats = party_ctx.reg().get<fl::ecs::components::Stats>(defender);
  stats.max_hp_ = 100;
  stats.hp_ = 100;

  encounter.combatant_bus(defender).emit(fl::events::CombatantEvent{
      fl::events::PoisonApplied{source, defender, 1, 27}});
  REQUIRE(party_ctx.reg().all_of<fl::ecs::components::Poison>(defender));

  encounter.atb_engine().active_combatant() = defender;
  encounter.clear_active_turn_for(defender);
  REQUIRE(encounter.atb_engine().active_combatant() == entt::entity{});

  tick_party(party_ctx, fl::primitives::WorldClock::beats_from_seconds(3));
  REQUIRE(stats.hp_ == 99);
}

TEST_CASE("Freeze survives active-turn cleanup for the affected participant",
          "[encounter][skills][freeze][lifetime]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();
  auto &encounter = party.create_encounter();

  const auto defender = party.members().front().member_id();
  encounter.defenders().members().push_back(defender);
  encounter.add_party_combatant_bus(defender);
  encounter.atb_in().emit(seerin::AtbInEvent{seerin::AddCombatant{defender}});

  auto build_ctx = party_ctx.build_context();
  auto source = fl::primitives::EntityBuilder(build_ctx)
                    .monster(fl::monster::MonsterKind::HoneyBadger)
                    .build();
  encounter.combatant_bus(defender).emit(fl::events::CombatantEvent{
      fl::events::FreezeApplied{source, defender, 30}});
  REQUIRE(party_ctx.reg().all_of<fl::ecs::components::Freeze>(defender));

  encounter.atb_engine().active_combatant() = defender;
  encounter.clear_active_turn_for(defender);
  REQUIRE(encounter.atb_engine().active_combatant() == entt::entity{});

  tick_party(party_ctx, fl::primitives::WorldClock::beats_from_seconds(30));
  REQUIRE_FALSE(party_ctx.reg().any_of<fl::ecs::components::Freeze>(defender));
  REQUIRE_FALSE(
      party_ctx.reg().any_of<fl::ecs::components::StatusTint>(defender));
}

TEST_CASE("Status effects clear on party wipe and scheduled work does not leak",
          "[encounter][skills][status][lifetime][wipe]") {
  fl::GrandCentral gc{1, 1, 3};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &party = party_ctx.party_data();
  auto &encounter = party.create_encounter();

  for (const auto &member : party.members()) {
    encounter.defenders().members().push_back(member.member_id());
    encounter.add_party_combatant_bus(member.member_id());
    encounter.atb_in().emit(
        seerin::AtbInEvent{seerin::AddCombatant{member.member_id()}});
  }

  const auto bleed_target = party.members()[0].member_id();
  const auto poison_target = party.members()[1].member_id();
  const auto freeze_target = party.members()[2].member_id();
  auto source = add_honey_badger(party_ctx, encounter);

  auto &bleed_stats =
      party_ctx.reg().get<fl::ecs::components::Stats>(bleed_target);
  auto &poison_stats =
      party_ctx.reg().get<fl::ecs::components::Stats>(poison_target);
  bleed_stats.max_hp_ = 100;
  bleed_stats.hp_ = 100;
  poison_stats.max_hp_ = 100;
  poison_stats.hp_ = 100;

  fl::skills::SkillSequencer sequencer{
      party_ctx, encounter.atb_engine().scheduler(), [](entt::entity) {}};
  sequencer.schedule(source, bleed_target, fl::skills::SkillId::Eviscerate);
  tick_party(party_ctx, 24);
  encounter.combatant_bus(poison_target)
      .emit(fl::events::CombatantEvent{
          fl::events::PoisonApplied{source, poison_target, 1, 27}});
  encounter.combatant_bus(freeze_target)
      .emit(fl::events::CombatantEvent{
          fl::events::FreezeApplied{source, freeze_target, 30}});

  REQUIRE(party_ctx.reg().all_of<fl::ecs::components::DireBleed>(bleed_target));
  REQUIRE(party_ctx.reg().all_of<fl::ecs::components::Poison>(poison_target));
  REQUIRE(party_ctx.reg().all_of<fl::ecs::components::Freeze>(freeze_target));

  party.party_bus().emit(fl::events::PartyEvent{fl::events::PartyWiped{}});

  REQUIRE_FALSE(
      party_ctx.reg().any_of<fl::ecs::components::DireBleed>(bleed_target));
  REQUIRE_FALSE(
      party_ctx.reg().any_of<fl::ecs::components::Poison>(poison_target));
  REQUIRE_FALSE(
      party_ctx.reg().any_of<fl::ecs::components::Freeze>(freeze_target));

  tick_party(party_ctx, fl::primitives::WorldClock::beats_from_seconds(5));
  REQUIRE(bleed_stats.hp_ == 100);
  REQUIRE(poison_stats.hp_ == 100);
  REQUIRE_FALSE(
      party_ctx.reg().any_of<fl::ecs::components::StatusTint>(freeze_target));
}
