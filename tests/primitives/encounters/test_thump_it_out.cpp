// tests/encounter_builder_tests.cpp
#include <catch2/catch_test_macros.hpp>

#include <algorithm>

#include <entt/entt.hpp>

#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/ecs/components/visual_effects.hpp"
#include "fl/ecs/systems/visual_resolver.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/grand_central.hpp"
#include "fl/lospec500.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "fl/skills/skill_sequence.hpp"
#include "fl/skills/thump.hpp"
#include "sr/atb_events.hpp"
#include "sr/timed_scheduler.hpp"

namespace {

TEST_CASE("EncounterBuilder::thump_it_out returns the created EncounterData",
          "[encounter_builder][encounter][combat]") {
  fl::GrandCentral gc{1, 1, 3};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);
  auto &encounter = builder.thump_it_out();

  REQUIRE(&encounter == &party_ctx.party_data().encounter_data());
}

TEST_CASE("EncounterBuilder::thump_it_out wires encounter teams and members",
          "[encounter_builder][encounter][combat]") {
  fl::GrandCentral gc{1, 1, 3};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);
  auto &encounter = builder.thump_it_out();

  REQUIRE(encounter.defenders().members().size() == 3);
  REQUIRE(encounter.attackers().members().size() ==
          fl::primitives::EncounterBuilder::kEnemyPartySize);

  REQUIRE(encounter.entities_to_cleanup().size() ==
          fl::primitives::EncounterBuilder::kEnemyPartySize);
  for (const entt::entity enemy : encounter.attackers().members()) {
    CHECK(party_ctx.reg().valid(enemy));
    CHECK(party_ctx.reg().all_of<fl::ecs::components::Stats>(enemy));
    CHECK(encounter.owns_entity(enemy));
  }
}

TEST_CASE("EncounterBuilder::thump_it_out enrolls party members as defenders",
          "[encounter_builder][encounter][combat]") {
  fl::GrandCentral gc{1, 1, 3};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);
  auto &encounter = builder.thump_it_out();

  REQUIRE(encounter.defenders().members().size() == 3);

  for (const entt::entity member : encounter.defenders().members()) {
    CHECK(party_ctx.reg().valid(member));
    CHECK_FALSE(encounter.owns_entity(member));
    CHECK(encounter.is_good_guy(member));
  }
}

TEST_CASE(
    "EncounterBuilder::thump_it_out creates an enemy party owned by encounter",
    "[encounter_builder][encounter][combat]") {
  fl::GrandCentral gc{1, 1, 3};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);
  auto &encounter = builder.thump_it_out();

  REQUIRE(encounter.attackers().members().size() ==
          fl::primitives::EncounterBuilder::kEnemyPartySize);

  REQUIRE(encounter.entities_to_cleanup().size() ==
          fl::primitives::EncounterBuilder::kEnemyPartySize);

  for (const entt::entity enemy : encounter.attackers().members()) {
    CHECK(party_ctx.reg().valid(enemy));
    CHECK(encounter.owns_entity(enemy));
    CHECK(encounter.is_bad_guy(enemy));
  }
}

TEST_CASE("EncounterBuilder gives each enemy a stable combatant bus",
          "[encounter_builder][encounter][combat][events]") {
  fl::GrandCentral gc{1, 1, 3};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);
  auto &encounter = builder.thump_it_out();

  REQUIRE(encounter.enemy_combatant_buses().size() ==
          encounter.attackers().members().size());

  const auto first_enemy = encounter.attackers().members().front();
  auto *first_bus = encounter.enemy_combatant_bus(first_enemy);
  REQUIRE(first_bus != nullptr);

  int first_bus_events = 0;
  fl::events::ScopedCombatantListener first_sub{
      *first_bus, std::in_place_type<fl::events::PlayerDied>,
      [&](const fl::events::PlayerDied &) { ++first_bus_events; }};

  first_bus->emit(fl::events::CombatantEvent{
      fl::events::PlayerDied{.player = first_enemy, .killer = entt::null}});
  REQUIRE(first_bus_events == 1);

  for (const entt::entity enemy : encounter.attackers().members()) {
    CHECK(encounter.enemy_combatant_bus(enemy) != nullptr);
  }

  const auto *stable_first_bus = encounter.enemy_combatant_bus(first_enemy);
  REQUIRE(stable_first_bus == first_bus);
}

TEST_CASE("EncounterBuilder common woodland pool includes new status monsters",
          "[encounter_builder][encounter][combat][status]") {
  const auto &pool = fl::primitives::EncounterBuilder::kCommonWoodland;

  REQUIRE(std::find(pool.begin(), pool.end(),
                    fl::monster::MonsterKind::PoisonToad) != pool.end());
  REQUIRE(std::find(pool.begin(), pool.end(),
                    fl::monster::MonsterKind::ScaredyCat) != pool.end());
  REQUIRE(std::find(pool.begin(), pool.end(), fl::monster::MonsterKind::Yeti) !=
          pool.end());
  REQUIRE(std::find(pool.begin(), pool.end(),
                    fl::monster::MonsterKind::Salamander) != pool.end());
  REQUIRE(std::find(pool.begin(), pool.end(),
                    fl::monster::MonsterKind::StormtickImp) != pool.end());
  REQUIRE(std::find(pool.begin(), pool.end(),
                    fl::monster::MonsterKind::CeilingGrudge) != pool.end());
  REQUIRE(std::find(pool.begin(), pool.end(),
                    fl::monster::MonsterKind::MiasmaToad) != pool.end());
  REQUIRE(std::find(pool.begin(), pool.end(),
                    fl::monster::MonsterKind::ChoirWisp) != pool.end());
  REQUIRE(std::find(pool.begin(), pool.end(),
                    fl::monster::MonsterKind::GorecapSprout) != pool.end());
  REQUIRE(std::find(pool.begin(), pool.end(),
                    fl::monster::MonsterKind::RimefangHare) != pool.end());
  REQUIRE(std::find(pool.begin(), pool.end(),
                    fl::monster::MonsterKind::NullMote) != pool.end());
}

TEST_CASE("EncounterBuilder rare woodland pool includes Fire Drake",
          "[encounter_builder][encounter][combat][rare]") {
  const auto &pool = fl::primitives::EncounterBuilder::kRareWoodland;

  REQUIRE(std::find(pool.begin(), pool.end(),
                    fl::monster::MonsterKind::HoneyBadger) != pool.end());
  REQUIRE(std::find(pool.begin(), pool.end(),
                    fl::monster::MonsterKind::FireDrake) != pool.end());
}

TEST_CASE("Thump ranks produce distinct damage bands",
          "[encounter][skills][rank][thump]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);
  auto &encounter = builder.thump_it_out();

  const entt::entity attacker = encounter.attackers().members().front();
  const entt::entity target = encounter.defenders().members().front();
  auto &target_stats = party_ctx.reg().get<fl::ecs::components::Stats>(target);
  target_stats.max_hp_ = 100;

  auto damage_for = [&](fl::skills::SkillKey skill) {
    target_stats.hp_ = target_stats.max_hp_;
    fl::skills::Thump thump;
    return thump.thump(
        fl::context::AttackCtx::make_attack(party_ctx, attacker, target),
        skill);
  };

  const auto thump_i = fl::skills::SkillKey{fl::skills::SkillId::Thump};
  const auto thump_iii = fl::skills::SkillKey{
      fl::skills::SkillId::Thump, fl::skills::SkillRank::require(3)};
  const auto thump_v = fl::skills::SkillKey{fl::skills::SkillId::Thump,
                                            fl::skills::SkillRank::require(5)};

  const int damage_i = damage_for(thump_i);
  const int damage_iii = damage_for(thump_iii);
  const int damage_v = damage_for(thump_v);

  REQUIRE(damage_i >= 1);
  REQUIRE(damage_i <= 5);
  REQUIRE(damage_iii >= 7);
  REQUIRE(damage_iii <= 11);
  REQUIRE(damage_v >= 13);
  REQUIRE(damage_v <= 17);
  REQUIRE(damage_i < damage_iii);
  REQUIRE(damage_iii < damage_v);
}

TEST_CASE("Thump ranks pay distinct tempo costs",
          "[encounter][timing][rank][thump]") {
  auto run_rank = [](fl::skills::SkillKey skill, int damage_beat,
                     int finish_beat) {
    fl::GrandCentral gc{1, 1, 1};

    auto account_ctx = gc.account_context(0);
    auto party_ctx = account_ctx.party_context(0);

    fl::primitives::EncounterBuilder builder(party_ctx);
    auto &encounter = builder.thump_it_out();

    const entt::entity attacker = encounter.attackers().members().front();
    const entt::entity target = encounter.defenders().members().front();
    auto &target_stats =
        party_ctx.reg().get<fl::ecs::components::Stats>(target);
    target_stats.max_hp_ = 100;
    target_stats.hp_ = 100;

    bool finished = false;
    seerin::TimedScheduler<seerin::AtbOutEvent> scheduler;
    fl::skills::SkillSequencer sequencer{
        party_ctx, scheduler,
        [&](entt::entity entity) { finished = entity == attacker; }};
    sequencer.schedule(attacker, target, skill);

    int current_beat = 0;
    auto advance_to = [&](int beat) {
      while (current_beat < beat) {
        scheduler.on_beat();
        ++current_beat;
      }
    };

    advance_to(damage_beat - 1);
    REQUIRE(target_stats.hp_ == 100);
    REQUIRE_FALSE(finished);

    advance_to(damage_beat);
    REQUIRE(target_stats.hp_ < 100);
    REQUIRE_FALSE(finished);

    advance_to(finish_beat - 1);
    REQUIRE_FALSE(finished);

    advance_to(finish_beat);
    REQUIRE(finished);
  };

  run_rank(fl::skills::SkillKey{fl::skills::SkillId::Thump}, 26, 31);
  run_rank(fl::skills::SkillKey{fl::skills::SkillId::Thump,
                                fl::skills::SkillRank::require(3)},
           30, 37);
  run_rank(fl::skills::SkillKey{fl::skills::SkillId::Thump,
                                fl::skills::SkillRank::require(5)},
           34, 43);
}

TEST_CASE("SkillSequencer thump-like flash uses two red pulses then yellow",
          "[encounter][timing][color]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);
  auto &encounter = builder.thump_it_out();

  const entt::entity attacker = encounter.attackers().members().front();
  const entt::entity target = encounter.defenders().members().front();
  auto &reg = party_ctx.reg();

  const auto red = fl::lospec500::color_at(4);
  const auto yellow = fl::lospec500::color_at(14);

  REQUIRE_FALSE(
      reg.any_of<fl::ecs::components::ResolvedColorOverride>(attacker));

  seerin::TimedScheduler<seerin::AtbOutEvent> scheduler;
  fl::skills::SkillSequencer sequencer{party_ctx, scheduler,
                                       [](entt::entity) {}};
  sequencer.schedule(attacker, target, fl::skills::SkillId::Thump);

  int current_beat = 0;
  auto advance_to = [&](int beat) {
    while (current_beat < beat) {
      scheduler.on_beat();
      ++current_beat;
      fl::ecs::systems::VisualResolver::resolve(reg, scheduler.now());
    }
  };

  advance_to(10);
  REQUIRE(
      reg.get<fl::ecs::components::ResolvedColorOverride>(attacker).color.Print(
          false) == red.Print(false));

  advance_to(14);
  REQUIRE_FALSE(reg.any_of<fl::ecs::components::DamageFlash>(attacker));
  REQUIRE_FALSE(
      reg.any_of<fl::ecs::components::ResolvedColorOverride>(attacker));

  advance_to(18);
  REQUIRE(
      reg.get<fl::ecs::components::ResolvedColorOverride>(attacker).color.Print(
          false) == red.Print(false));

  advance_to(22);
  REQUIRE_FALSE(reg.any_of<fl::ecs::components::DamageFlash>(attacker));
  REQUIRE(
      reg.get<fl::ecs::components::ResolvedColorOverride>(target).color.Print(
          false) == yellow.Print(false));

  advance_to(30);
  REQUIRE_FALSE(reg.any_of<fl::ecs::components::DamageFlash>(target));
  REQUIRE_FALSE(reg.any_of<fl::ecs::components::ResolvedColorOverride>(target));
}

TEST_CASE("SkillSequencer Flame Strike animates before damage",
          "[encounter][timing][flame_strike]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);
  auto &encounter = builder.thump_it_out();

  const entt::entity attacker = encounter.attackers().members().front();
  const entt::entity target = encounter.defenders().members().front();
  auto &reg = party_ctx.reg();

  seerin::TimedScheduler<seerin::AtbOutEvent> scheduler;
  bool finished = false;
  fl::skills::SkillSequencer sequencer{
      party_ctx, scheduler,
      [&](entt::entity entity) { finished = entity == attacker; }};

  sequencer.schedule(attacker, target, fl::skills::SkillId::FlameStrike);
  const auto log_size_after_schedule = party_ctx.log().size();

  REQUIRE(reg.any_of<fl::ecs::components::CombatantDecals>(target));
  REQUIRE(reg.get<fl::ecs::components::CombatantDecals>(target)
              .effects.front()
              .duration == std::chrono::milliseconds{1000});

  for (int i = 0; i < seerin::BEATS_PER_SEC - 1; ++i) {
    scheduler.on_beat();
    fl::ecs::systems::VisualResolver::resolve(reg, scheduler.now());
  }

  REQUIRE(party_ctx.log().size() == log_size_after_schedule);
  REQUIRE(reg.any_of<fl::ecs::components::CombatantDecals>(target));
  REQUIRE_FALSE(finished);

  scheduler.on_beat();
  fl::ecs::systems::VisualResolver::resolve(reg, scheduler.now());

  REQUIRE(party_ctx.log().size() > log_size_after_schedule);
  REQUIRE(reg.any_of<fl::ecs::components::CombatantDecals>(target));
  REQUIRE_FALSE(finished);

  scheduler.on_beat();
  fl::ecs::systems::VisualResolver::resolve(reg, scheduler.now());

  REQUIRE(finished);
}

TEST_CASE("SkillSequencer Flame Wave staggers all alive opponents",
          "[encounter][timing][flame_wave]") {
  fl::GrandCentral gc{1, 1, 3};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);
  auto &encounter = builder.thump_it_out();

  const entt::entity attacker = encounter.attackers().members().front();
  const auto targets = encounter.defenders().alive_members(party_ctx);
  REQUIRE(targets.size() == 3);

  auto &reg = party_ctx.reg();
  seerin::TimedScheduler<seerin::AtbOutEvent> scheduler;
  bool finished = false;
  fl::skills::SkillSequencer sequencer{
      party_ctx, scheduler,
      [&](entt::entity entity) { finished = entity == attacker; }};

  sequencer.schedule(attacker, targets.front(), fl::skills::SkillId::FlameWave);
  const auto log_size_after_schedule = party_ctx.log().size();

  scheduler.on_beat();
  fl::ecs::systems::VisualResolver::resolve(reg, scheduler.now());
  REQUIRE(reg.any_of<fl::ecs::components::CombatantDecals>(targets[0]));
  REQUIRE_FALSE(reg.any_of<fl::ecs::components::CombatantDecals>(targets[1]));
  REQUIRE_FALSE(reg.any_of<fl::ecs::components::CombatantDecals>(targets[2]));

  for (int i = 0; i < 2; ++i) {
    scheduler.on_beat();
    fl::ecs::systems::VisualResolver::resolve(reg, scheduler.now());
  }
  REQUIRE(reg.any_of<fl::ecs::components::CombatantDecals>(targets[1]));
  REQUIRE_FALSE(reg.any_of<fl::ecs::components::CombatantDecals>(targets[2]));

  for (int i = 0; i < 3; ++i) {
    scheduler.on_beat();
    fl::ecs::systems::VisualResolver::resolve(reg, scheduler.now());
  }
  REQUIRE(reg.any_of<fl::ecs::components::CombatantDecals>(targets[2]));

  for (int i = 0; i < 5; ++i) {
    scheduler.on_beat();
    fl::ecs::systems::VisualResolver::resolve(reg, scheduler.now());
  }
  REQUIRE(party_ctx.log().size() == log_size_after_schedule);

  scheduler.on_beat();
  fl::ecs::systems::VisualResolver::resolve(reg, scheduler.now());
  REQUIRE(party_ctx.log().size() > log_size_after_schedule);
  REQUIRE_FALSE(finished);

  for (int i = 0; i < 7; ++i) {
    scheduler.on_beat();
    fl::ecs::systems::VisualResolver::resolve(reg, scheduler.now());
  }
  REQUIRE(finished);
}

TEST_CASE("Mercyburst targets the lowest-health teammate",
          "[encounter][skills][mercyburst]") {
  fl::GrandCentral gc{1, 1, 3};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);
  auto &encounter = builder.thump_it_out();

  auto &reg = party_ctx.reg();
  const auto enemies = encounter.attackers().members();
  REQUIRE(enemies.size() >= 2);
  const entt::entity enemy_attacker = enemies[0];
  const entt::entity wounded_enemy = enemies[1];
  reg.get<fl::ecs::components::Stats>(enemy_attacker).hp_ = 8;
  reg.get<fl::ecs::components::Stats>(wounded_enemy).hp_ = 2;

  REQUIRE(encounter.target_for_skill(enemy_attacker,
                                     fl::skills::SkillId::Mercyburst) ==
          wounded_enemy);

  const auto defenders = encounter.defenders().members();
  REQUIRE(defenders.size() >= 2);
  const entt::entity defender_attacker = defenders[0];
  const entt::entity wounded_defender = defenders[1];
  reg.get<fl::ecs::components::Stats>(defender_attacker).hp_ = 7;
  reg.get<fl::ecs::components::Stats>(wounded_defender).hp_ = 1;

  REQUIRE(encounter.target_for_skill(defender_attacker,
                                     fl::skills::SkillId::Mercyburst) ==
          wounded_defender);
}

TEST_CASE("SkillSequencer Mercyburst heals and clamps at max HP",
          "[encounter][skills][mercyburst]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);
  auto &encounter = builder.thump_it_out();

  const entt::entity attacker = encounter.attackers().members().front();
  const entt::entity target = encounter.attackers().members().back();
  auto &reg = party_ctx.reg();
  auto &stats = reg.get<fl::ecs::components::Stats>(target);
  stats.max_hp_ = 10;
  stats.hp_ = 8;

  seerin::TimedScheduler<seerin::AtbOutEvent> scheduler;
  bool finished = false;
  fl::skills::SkillSequencer sequencer{
      party_ctx, scheduler,
      [&](entt::entity entity) { finished = entity == attacker; }};

  sequencer.schedule(attacker, target, fl::skills::SkillId::Mercyburst);
  REQUIRE(reg.any_of<fl::ecs::components::CombatantDecals>(target));

  for (int i = 0; i < seerin::BEATS_PER_SEC; ++i) {
    scheduler.on_beat();
    fl::ecs::systems::VisualResolver::resolve(reg, scheduler.now());
  }

  REQUIRE(stats.hp_ == stats.max_hp_);
  REQUIRE_FALSE(finished);

  scheduler.on_beat();
  fl::ecs::systems::VisualResolver::resolve(reg, scheduler.now());
  REQUIRE(finished);
  REQUIRE(reg.any_of<fl::ecs::components::CombatantDecals>(target));
}

TEST_CASE("Flee emits combat events and grants no XP on successful flee",
          "[encounter][skills][flee]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);
  auto &encounter = builder.thump_it_out();

  const entt::entity attacker = encounter.attackers().members().front();
  const entt::entity target = encounter.defenders().members().front();

  auto &reg = party_ctx.reg();
  const auto xp_before = reg.get<fl::ecs::components::TrackXP>(target).xp_;

  int flee_attempt_events = 0;
  int fled_events = 0;
  bool saw_success = false;

  fl::events::ScopedCombatantListener flee_attempt_sub{
      party_ctx.party_data().encounter_data().combatant_bus(attacker),
      std::in_place_type<fl::events::FleeAttempted>,
      [&](const fl::events::FleeAttempted &ev) {
        ++flee_attempt_events;
        if (ev.success) {
          saw_success = true;
        }
      }};
  fl::events::ScopedCombatantListener fled_sub{
      party_ctx.party_data().encounter_data().combatant_bus(attacker),
      std::in_place_type<fl::events::CombatantFled>,
      [&](const fl::events::CombatantFled &) { ++fled_events; }};

  seerin::TimedScheduler<seerin::AtbOutEvent> scheduler;
  fl::skills::SkillSequencer sequencer{party_ctx, scheduler,
                                       [](entt::entity) {}};

  for (int attempt = 0; attempt < 12 && !saw_success; ++attempt) {
    sequencer.schedule(attacker, target, fl::skills::SkillId::Flee);
    scheduler.on_beat();
    scheduler.on_beat();
  }

  REQUIRE(flee_attempt_events >= 1);
  REQUIRE(saw_success);
  REQUIRE(fled_events >= 1);
  REQUIRE(reg.get<fl::ecs::components::Stats>(attacker).hp_ == 0);
  REQUIRE(reg.get<fl::ecs::components::TrackXP>(target).xp_ == xp_before);
}

} // namespace
