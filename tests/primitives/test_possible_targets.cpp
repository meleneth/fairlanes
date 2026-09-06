#include <catch2/catch_test_macros.hpp>

#include <array>
#include <ranges>

#include "fl/targeting/possible_targets.hpp"
#include "fl/targeting/status_filters.hpp"

namespace {
using namespace fl::targeting;
using fl::ecs::components::Stats;

entt::entity combatant(entt::registry &reg, int hp) {
  auto entity = reg.create();
  reg.emplace<Stats>(entity).hp_ = hp;
  return entity;
}
} // namespace

TEST_CASE("Target ranges distinguish encounter sides and living and dead "
          "participants",
          "[targeting]") {
  entt::registry reg;
  auto actor = combatant(reg, 10);
  auto ally = combatant(reg, 20);
  auto dead_ally = combatant(reg, 0);
  auto enemy = combatant(reg, 30);
  auto dead_enemy = combatant(reg, 0);
  auto outsider = combatant(reg, 10);
  auto stale = combatant(reg, 10);
  reg.destroy(stale);
  auto no_stats = reg.create();
  const std::array friends{actor, ally,     dead_ally,
                           stale, no_stats, entt::entity{entt::null}};
  const std::array enemies{enemy, dead_enemy};
  PossibleTargets targets{reg, enemies, friends};
  REQUIRE(snapshot_targets(targets.FriendlyPossibleTargets(actor)) ==
          std::vector{actor, ally});
  REQUIRE(snapshot_targets(targets.FriendlyDeadPossibleTargets(actor)) ==
          std::vector{dead_ally});
  REQUIRE(snapshot_targets(targets.EnemyPossibleTargets(actor)) ==
          std::vector{enemy});
  REQUIRE(snapshot_targets(targets.EnemyDeadPossibleTargets(actor)) ==
          std::vector{dead_enemy});
  REQUIRE(snapshot_targets(targets.FriendlyPossibleTargets(enemy)) ==
          std::vector{enemy});
  REQUIRE(snapshot_targets(targets.FriendlyDeadPossibleTargets(enemy)) ==
          std::vector{dead_enemy});
  REQUIRE(snapshot_targets(targets.EnemyPossibleTargets(enemy)) ==
          std::vector{actor, ally});
  REQUIRE(snapshot_targets(targets.EnemyDeadPossibleTargets(enemy)) ==
          std::vector{dead_ally});
  for (auto invalid : {outsider, stale, entt::entity{entt::null}}) {
    REQUIRE(std::ranges::empty(targets.FriendlyPossibleTargets(invalid)));
    REQUIRE(std::ranges::empty(targets.FriendlyDeadPossibleTargets(invalid)));
    REQUIRE(std::ranges::empty(targets.EnemyPossibleTargets(invalid)));
    REQUIRE(std::ranges::empty(targets.EnemyDeadPossibleTargets(invalid)));
  }
}

TEST_CASE(
    "Target ranges read eligibility lazily and refresh after state changes",
    "[targeting]") {
  entt::registry reg;
  auto actor = combatant(reg, 10);
  auto enemy = combatant(reg, 10);
  std::array friends{actor};
  std::array enemies{enemy};
  PossibleTargets targets{reg, enemies, friends};
  auto range = targets.EnemyPossibleTargets(actor);
  reg.get<Stats>(enemy).hp_ = 0;
  REQUIRE(std::ranges::empty(range));
  REQUIRE(snapshot_targets(targets.EnemyDeadPossibleTargets(actor)) ==
          std::vector{enemy});
  reg.get<Stats>(enemy).hp_ = 5;
  REQUIRE(snapshot_targets(targets.EnemyPossibleTargets(actor)) ==
          std::vector{enemy});
  reg.destroy(enemy);
  REQUIRE(std::ranges::empty(targets.EnemyDeadPossibleTargets(actor)));
}

TEST_CASE("Chained status filters match the same candidate and compose with "
          "life state",
          "[targeting][status]") {
  using namespace fl::ecs::components;
  entt::registry reg;
  auto actor = combatant(reg, 10);
  auto poison = combatant(reg, 10);
  auto both = combatant(reg, 10);
  auto frozen = combatant(reg, 10);
  auto dead = combatant(reg, 0);
  for (auto e : {poison, both, dead})
    reg.emplace<Poison>(e);
  for (auto e : {both, frozen})
    reg.emplace<Freeze>(e);
  auto &buffs = reg.emplace<CombatStatuses>(poison);
  buffs.effects.emplace_back().kind = CombatStatusKind::Shield;
  const std::array friends{actor, poison, both, frozen, dead};
  PossibleTargets targets{reg, {}, friends};
  auto range = targets.FriendlyPossibleTargets(actor) |
               HasStatus(reg, TargetStatus::Poison) |
               LacksStatus(reg, TargetStatus::Freeze);
  REQUIRE(snapshot_targets(range) == std::vector{poison});
  REQUIRE(snapshot_targets(targets.FriendlyPossibleTargets(actor) |
                           HasAllStatuses(reg, TargetStatus::Poison,
                                          TargetStatus::Freeze)) ==
          std::vector{both});
  REQUIRE(snapshot_targets(
              targets.FriendlyPossibleTargets(actor) |
              HasAnyStatus(reg, TargetStatus::Shield, TargetStatus::Freeze)) ==
          std::vector{poison, both, frozen});
  REQUIRE(snapshot_targets(targets.FriendlyDeadPossibleTargets(actor) |
                           HasStatus(reg, TargetStatus::Poison)) ==
          std::vector{dead});
  REQUIRE(snapshot_targets(targets.FriendlyPossibleTargets(actor) |
                           ExcludeSelf(actor)) ==
          std::vector{poison, both, frozen});
}
