#include <catch2/catch_test_macros.hpp>

#include <algorithm>

#include "fl/consumables/consumable_kind.hpp"
#include "fl/ecs/components/combat_resources.hpp"
#include "fl/ecs/components/consumable.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/resource_cast_snapshot.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/timed_buffs.hpp"
#include "fl/ecs/systems/combat_resource_system.hpp"
#include "fl/ecs/systems/consumable_system.hpp"
#include "fl/ecs/systems/effective_combat_stats.hpp"
#include "fl/grand_central.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "fl/skills/skill_scaling.hpp"
#include "fl/skills/skill_definition.hpp"
#include "fl/skills/skill_selection.hpp"

namespace {

struct CombatFixture {
  fl::GrandCentral game{1, 1, 1};
  fl::context::PartyCtx party{game.account_context(0).party_context(0)};
  entt::entity hero{party.party_data().members().front().member_id()};
  fl::primitives::EncounterData& encounter{
      fl::primitives::EncounterBuilder{party}.thump_it_out()};
  fl::context::EncounterCtx& combat{encounter.context()};
};

} // namespace

TEST_CASE("Combat resource pools clamp generation, restoration, and expenditure",
          "[resources][bounds]") {
  fl::ecs::components::CombatResources resources;
  using enum fl::resources::CombatResource;
  REQUIRE(resources.get(Adrenaline).current == 0);
  REQUIRE(resources.get(Reflexes).current == 10);
  REQUIRE(resources.modify(Adrenaline, 99) == 10);
  REQUIRE(resources.get(Adrenaline).current == 10);
  REQUIRE(resources.modify(Adrenaline, -99) == -10);
  REQUIRE(resources.get(Adrenaline).current == 0);
  resources.get(Reflexes).maximum = 6;
  REQUIRE(resources.set(Reflexes, 99) == 6);
}

TEST_CASE("The first Origin recruit receives the finite demonstration starter pack",
          "[resources][consumables][inventory]") {
  fl::GrandCentral game{1, 1, 0, true, true, 42, true};
  auto party = game.account_context(0).party_context(0);
  REQUIRE(party.party_data().items().empty());
  REQUIRE(party.party_data().recruit_origin_member("Starter") != entt::entity{entt::null});
  REQUIRE(std::ranges::count_if(party.party_data().items(), [&](const auto item) {
            return game.reg().all_of<fl::ecs::components::Consumable>(item);
          }) == 6);
}

TEST_CASE("Resource scaling uses the pre-expenditure cast snapshot", "[resources][scaling]") {
  entt::registry reg;
  const auto actor = reg.create();
  auto& stats = reg.emplace<fl::ecs::components::Stats>(actor, "resource tester");
  stats.attack_ = 0;
  auto& resources = reg.emplace<fl::ecs::components::CombatResources>(actor);
  resources.set(fl::resources::CombatResource::Adrenaline, 6);
  fl::ecs::components::ResourceCastSnapshot snapshot{.skill = fl::skills::SkillId::ThumpAround};
  snapshot.current[fl::resources::index(fl::resources::CombatResource::Adrenaline)] = 6;
  snapshot.maximum[fl::resources::index(fl::resources::CombatResource::Adrenaline)] = 10;
  reg.emplace<fl::ecs::components::ResourceCastSnapshot>(actor, snapshot);
  resources.modify(fl::resources::CombatResource::Adrenaline, -4);

  fl::skills::SkillScalingRule rule;
  rule.resource = fl::resources::CombatResource::Adrenaline;
  rule.resource_source = fl::skills::ResourceScalingSource::Current;
  rule.resource_numerator = 1;
  rule.resource_denominator = 1;
  rule.resource_maximum_bonus = 6;
  REQUIRE(fl::skills::evaluate_scaling(reg, actor, rule, 1, fl::skills::SkillId::ThumpAround)
              .resource_bonus == 6);
  REQUIRE(fl::skills::evaluate_scaling(reg, actor, rule, 1, fl::skills::SkillId::VenomNeedle)
              .resource_bonus == 2);
  reg.remove<fl::ecs::components::ResourceCastSnapshot>(actor);
  REQUIRE(fl::skills::evaluate_scaling(reg, actor, rule, 1).resource_bonus == 2);
}

TEST_CASE("Unused resources do not alter resource-independent skills", "[resources][scaling]") {
  entt::registry reg;
  const auto actor = reg.create();
  auto& stats = reg.emplace<fl::ecs::components::Stats>(actor, "ordinary tester");
  stats.attack_ = 4;
  auto& resources = reg.emplace<fl::ecs::components::CombatResources>(actor);
  const auto& rule = fl::skills::definition(fl::skills::SkillId::Bump).scaling;
  const int empty = fl::skills::evaluate_scaling(reg, actor, rule, rule.base_effect).total;
  resources.set(fl::resources::CombatResource::Adrenaline, 10);
  resources.set(fl::resources::CombatResource::Reflexes, 0);
  REQUIRE(fl::skills::evaluate_scaling(reg, actor, rule, rule.base_effect).total == empty);
}

TEST_CASE("An unaffordable skill is skipped while an ordinary fallback remains productive",
          "[resources][selection]") {
  CombatFixture fixture;
  auto& member = fixture.game.reg().get<fl::ecs::components::PartyMember>(fixture.hero);
  member.closet().skill_slots.fill(std::nullopt);
  member.closet().skill_slots[0] = fl::skills::SkillId::ThumpAround;
  member.closet().skill_slots[1] = fl::skills::SkillId::Thump;
  fixture.game.reg().get<fl::ecs::components::CombatResources>(fixture.hero)
      .set(fl::resources::CombatResource::Adrenaline, 0);
  REQUIRE_FALSE(fl::ecs::systems::CombatResourceSystem::eligibility(
                    fixture.game.reg(), fixture.hero, fl::skills::SkillId::ThumpAround)
                    .eligible);
  REQUIRE(fl::skills::choose_skill(fixture.game.reg(), fixture.game.rng(), fixture.hero).base ==
          fl::skills::SkillId::Thump);
}

TEST_CASE("Skill costs commit once and generation cannot exceed capacity",
          "[resources][action-lifecycle]") {
  CombatFixture fixture;
  auto& resources = fixture.game.reg().get<fl::ecs::components::CombatResources>(fixture.hero);
  resources.set(fl::resources::CombatResource::Adrenaline, 0);
  REQUIRE_FALSE(fl::ecs::systems::CombatResourceSystem::begin_skill(
      fixture.combat, fixture.hero, fl::skills::SkillId::ThumpAround));
  REQUIRE(resources.get(fl::resources::CombatResource::Adrenaline).current == 0);
  REQUIRE_FALSE(fixture.game.reg().all_of<fl::ecs::components::ResourceCastSnapshot>(fixture.hero));
  resources.set(fl::resources::CombatResource::Adrenaline, 6);
  REQUIRE(fl::ecs::systems::CombatResourceSystem::begin_skill(
      fixture.combat, fixture.hero, fl::skills::SkillId::ThumpAround));
  REQUIRE(resources.get(fl::resources::CombatResource::Adrenaline).current == 2);
  fl::ecs::systems::CombatResourceSystem::resolve_skill(
      fixture.combat, fixture.hero, fixture.encounter.attackers().members().front(),
      fl::skills::SkillId::ThumpAround, true);
  fl::ecs::systems::CombatResourceSystem::resolve_skill(
      fixture.combat, fixture.hero, fixture.encounter.attackers().members().front(),
      fl::skills::SkillId::ThumpAround, true);
  REQUIRE(resources.get(fl::resources::CombatResource::Adrenaline).current == 2);
  fl::ecs::systems::CombatResourceSystem::finish_skill(fixture.game.reg(), fixture.hero);

  // Once an eligible action begins, cancellation/interruption clears its cast
  // snapshot but does not refund the already committed cost.
  resources.set(fl::resources::CombatResource::Adrenaline, 6);
  REQUIRE(fl::ecs::systems::CombatResourceSystem::begin_skill(
      fixture.combat, fixture.hero, fl::skills::SkillId::ThumpAround));
  fl::ecs::systems::CombatResourceSystem::finish_skill(fixture.game.reg(), fixture.hero);
  REQUIRE(resources.get(fl::resources::CombatResource::Adrenaline).current == 2);
  REQUIRE_FALSE(fixture.game.reg().all_of<fl::ecs::components::ResourceCastSnapshot>(fixture.hero));

  resources.set(fl::resources::CombatResource::Adrenaline, 9);
  REQUIRE(fl::ecs::systems::CombatResourceSystem::begin_skill(
      fixture.combat, fixture.hero, fl::skills::SkillId::Thump));
  fl::ecs::systems::CombatResourceSystem::resolve_skill(
      fixture.combat, fixture.hero, fixture.encounter.attackers().members().front(),
      fl::skills::SkillId::Thump, true);
  REQUIRE(resources.get(fl::resources::CombatResource::Adrenaline).current == 10);
}

TEST_CASE("Healing potions consume one owned item and cap effective healing",
          "[consumables][potions]") {
  CombatFixture fixture;
  auto& stats = fixture.game.reg().get<fl::ecs::components::Stats>(fixture.hero);
  stats.hp_ = stats.max_hp_ - 5;
  const auto potion = fl::ecs::systems::ConsumableSystem::create(
      fixture.game.reg(), fl::consumables::ConsumableKind::HealingPotion, 2);
  fixture.party.party_data().add_item(potion);
  REQUIRE(fl::ecs::systems::ConsumableSystem::use_potion(fixture.combat, fixture.hero, potion));
  REQUIRE(stats.hp_ == stats.max_hp_);
  REQUIRE(fixture.game.reg().get<fl::ecs::components::Consumable>(potion).quantity == 1);
  REQUIRE(fl::ecs::systems::ConsumableSystem::use_potion(fixture.combat, fixture.hero, potion));
  REQUIRE_FALSE(fixture.game.reg().valid(potion));
  REQUIRE(std::ranges::find(fixture.party.party_data().items(), potion) ==
          fixture.party.party_data().items().end());
}

TEST_CASE("Autonomous potion policy avoids full health and uses emergency healing",
          "[consumables][autonomy]") {
  CombatFixture fixture;
  auto& stats = fixture.game.reg().get<fl::ecs::components::Stats>(fixture.hero);
  REQUIRE_FALSE(fl::ecs::systems::ConsumableSystem::choose_combat_potion(
      fixture.combat, fixture.hero));
  stats.hp_ = std::max(1, stats.max_hp_ / 4);
  const auto chosen = fl::ecs::systems::ConsumableSystem::choose_combat_potion(
      fixture.combat, fixture.hero);
  REQUIRE(chosen.has_value());
  REQUIRE(fixture.game.reg().get<fl::ecs::components::Consumable>(*chosen).kind ==
          fl::consumables::ConsumableKind::HealingPotion);
}

TEST_CASE("Resource potions restore the declared pool once and respect capacity",
          "[resources][consumables][potions]") {
  CombatFixture fixture;
  auto& resources = fixture.game.reg().get<fl::ecs::components::CombatResources>(fixture.hero);
  resources.set(fl::resources::CombatResource::Adrenaline, 8);
  const auto potion = fl::ecs::systems::ConsumableSystem::create(
      fixture.game.reg(), fl::consumables::ConsumableKind::AdrenalinePotion);
  REQUIRE_FALSE(
      fl::ecs::systems::ConsumableSystem::use_potion(fixture.combat, fixture.hero, potion));
  fixture.party.party_data().add_item(potion);
  REQUIRE(fl::ecs::systems::ConsumableSystem::use_potion(fixture.combat, fixture.hero, potion));
  REQUIRE(resources.get(fl::resources::CombatResource::Adrenaline).current == 10);
  REQUIRE_FALSE(fixture.game.reg().valid(potion));
}

TEST_CASE("Identical flasks replace their buff and expire on simulated world beats",
          "[consumables][flasks]") {
  fl::GrandCentral game{1, 1, 1};
  auto party = game.account_context(0).party_context(0);
  const auto hero = party.party_data().members().front().member_id();
  const int base_attack = fl::ecs::systems::effective_combat_stats(game.reg(), hero).attack;
  const auto first = fl::ecs::systems::ConsumableSystem::create(
      game.reg(), fl::consumables::ConsumableKind::AttackFlask);
  const auto second = fl::ecs::systems::ConsumableSystem::create(
      game.reg(), fl::consumables::ConsumableKind::AttackFlask);
  party.party_data().add_item(first);
  party.party_data().add_item(second);
  REQUIRE(fl::ecs::systems::ConsumableSystem::prepare_flask(party, hero, first, 10));
  REQUIRE(fl::ecs::systems::effective_combat_stats(game.reg(), hero).attack == base_attack + 3);
  REQUIRE(fl::ecs::systems::ConsumableSystem::prepare_flask(party, hero, second, 20));
  const auto& buffs = game.reg().get<fl::ecs::components::TimedBuffs>(hero);
  REQUIRE(buffs.effects.size() == 1);
  REQUIRE(fl::ecs::systems::effective_combat_stats(game.reg(), hero).attack == base_attack + 3);
  const auto tempo = std::ranges::find_if(party.party_data().items(), [&](const auto item) {
    const auto* value = game.reg().try_get<fl::ecs::components::Consumable>(item);
    return value && value->kind == fl::consumables::ConsumableKind::TempoFlask;
  });
  REQUIRE(tempo != party.party_data().items().end());
  REQUIRE(fl::ecs::systems::ConsumableSystem::prepare_flask(party, hero, *tempo, 20));
  REQUIRE(fl::ecs::systems::effective_combat_stats(game.reg(), hero).atb_charge_rate_percent ==
          120);
  party.party_data().revitalize_members();
  REQUIRE(game.reg().get<fl::ecs::components::TimedBuffs>(hero).effects.size() == 2);
  for (auto& effect : game.reg().get<fl::ecs::components::TimedBuffs>(hero).effects)
    effect.expires_world_beat = game.world_clock().elapsed_beats() + 1;
  game.advance_beat();
  REQUIRE(game.reg().get<fl::ecs::components::TimedBuffs>(hero).effects.empty());
  REQUIRE(fl::ecs::systems::effective_combat_stats(game.reg(), hero).attack == base_attack);
}

TEST_CASE("Encounter cleanup preserves character resources and timed buffs",
          "[resources][consumables][encounter-boundary]") {
  fl::GrandCentral game{1, 1, 1};
  auto party = game.account_context(0).party_context(0);
  const auto hero = party.party_data().members().front().member_id();
  game.reg().get<fl::ecs::components::CombatResources>(hero).set(
      fl::resources::CombatResource::Adrenaline, 7);
  const auto flask = std::ranges::find_if(party.party_data().items(), [&](const auto item) {
    const auto* value = game.reg().try_get<fl::ecs::components::Consumable>(item);
    return value && value->kind == fl::consumables::ConsumableKind::ArmorFlask;
  });
  REQUIRE(flask != party.party_data().items().end());
  REQUIRE(fl::ecs::systems::ConsumableSystem::prepare_flask(party, hero, *flask, 0));
  fl::primitives::EncounterBuilder{party}.thump_it_out();
  party.party_data().leave_combat();
  REQUIRE(game.reg()
              .get<fl::ecs::components::CombatResources>(hero)
              .get(fl::resources::CombatResource::Adrenaline)
              .current == 7);
  REQUIRE(game.reg().get<fl::ecs::components::TimedBuffs>(hero).effects.size() == 1);
}
