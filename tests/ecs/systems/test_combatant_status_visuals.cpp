#include <catch2/catch_test_macros.hpp>

#include <algorithm>

#include "fl/context.hpp"
#include "fl/ecs/components/combat_status.hpp"
#include "fl/ecs/components/dire_bleed.hpp"
#include "fl/ecs/components/freeze.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/poison.hpp"
#include "fl/ecs/systems/combat_status_system.hpp"
#include "fl/ecs/systems/combatant_status_visuals.hpp"
#include "fl/grand_central.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "fl/primitives/entity_builder.hpp"
#include "fl/widgets/effects/decal.hpp"

namespace {

using fl::ecs::components::CombatStatusKind;
using fl::ecs::systems::CombatantVisualLayer;
using fl::ecs::systems::CombatantVisualRegion;
using fl::ecs::systems::CombatStatusSystem;

struct BuiltEncounter {
  fl::GrandCentral gc{1, 1, 2};
  fl::context::AccountCtx account_ctx{gc.account_context(0)};
  fl::context::PartyCtx party_ctx{account_ctx.party_context(0)};
  fl::primitives::EncounterBuilder builder{party_ctx};
  fl::primitives::EncounterData &encounter{builder.thump_it_out()};

  entt::entity attacker() const {
    return encounter.attackers().members().front();
  }
  entt::entity defender() const {
    return encounter.defenders().members().front();
  }

  CombatStatusSystem::Scheduler &scheduler() {
    return encounter.atb_engine().scheduler();
  }
};

bool has_visual(entt::registry &reg, entt::entity entity, CombatStatusKind kind,
                CombatantVisualLayer layer, CombatantVisualRegion region) {
  const auto visuals = fl::ecs::systems::combatant_status_visuals(reg, entity);
  return std::ranges::any_of(visuals, [&](const auto &visual) {
    return visual.status_kind == kind && visual.layer == layer &&
           visual.region == region;
  });
}

bool has_decal(entt::registry &reg, entt::entity entity,
               fl::widgets::effects::DecalAnimationKind kind) {
  const auto visuals = fl::ecs::systems::combatant_status_visuals(reg, entity);
  return std::ranges::any_of(visuals, [&](const auto &visual) {
    return visual.decal.has_value() && visual.decal->animation_kind == kind;
  });
}

} // namespace

TEST_CASE("Shared combat statuses expose centralized combatant visuals",
          "[combat-status][visuals]") {
  BuiltEncounter h;
  const auto actor = h.defender();

  REQUIRE(CombatStatusSystem::apply_status(h.party_ctx, h.scheduler(),
                                           {.kind = CombatStatusKind::Shield,
                                            .name = "Test Shield",
                                            .source = actor,
                                            .target = actor,
                                            .value = 30,
                                            .negative = false}));
  REQUIRE(CombatStatusSystem::apply_status(h.party_ctx, h.scheduler(),
                                           {.kind = CombatStatusKind::Haste,
                                            .name = "Test Haste",
                                            .source = actor,
                                            .target = actor,
                                            .value = 20,
                                            .negative = false}));
  REQUIRE(CombatStatusSystem::apply_status(h.party_ctx, h.scheduler(),
                                           {.kind = CombatStatusKind::Slow,
                                            .name = "Test Slow",
                                            .source = h.attacker(),
                                            .target = actor,
                                            .value = 20}));
  REQUIRE(CombatStatusSystem::apply_status(h.party_ctx, h.scheduler(),
                                           {.kind = CombatStatusKind::Silence,
                                            .name = "Test Silence",
                                            .source = h.attacker(),
                                            .target = actor}));
  REQUIRE(CombatStatusSystem::apply_status(h.party_ctx, h.scheduler(),
                                           {.kind = CombatStatusKind::Stun,
                                            .name = "Test Stun",
                                            .source = h.attacker(),
                                            .target = actor}));

  auto &reg = h.party_ctx.reg();
  REQUIRE(has_visual(reg, actor, CombatStatusKind::Shield,
                     CombatantVisualLayer::Underlay,
                     CombatantVisualRegion::WholeCombatant));
  REQUIRE(has_visual(reg, actor, CombatStatusKind::Haste,
                     CombatantVisualLayer::Underlay,
                     CombatantVisualRegion::Feet));
  REQUIRE(has_visual(reg, actor, CombatStatusKind::Slow,
                     CombatantVisualLayer::Underlay,
                     CombatantVisualRegion::Feet));
  REQUIRE(has_visual(reg, actor, CombatStatusKind::Silence,
                     CombatantVisualLayer::StatusText,
                     CombatantVisualRegion::Nameplate));
  REQUIRE(has_visual(reg, actor, CombatStatusKind::Stun,
                     CombatantVisualLayer::StatusText,
                     CombatantVisualRegion::Head));
}

TEST_CASE("Removing shared combat statuses removes derived combatant visuals",
          "[combat-status][visuals][cleanse]") {
  BuiltEncounter h;
  const auto actor = h.defender();

  REQUIRE(CombatStatusSystem::apply_status(h.party_ctx, h.scheduler(),
                                           {.kind = CombatStatusKind::Slow,
                                            .name = "Test Slow",
                                            .source = h.attacker(),
                                            .target = actor,
                                            .value = 20}));
  REQUIRE(has_visual(h.party_ctx.reg(), actor, CombatStatusKind::Slow,
                     CombatantVisualLayer::Underlay,
                     CombatantVisualRegion::Feet));

  REQUIRE(CombatStatusSystem::cleanse(h.party_ctx, actor, actor) == 1);
  REQUIRE_FALSE(has_visual(h.party_ctx.reg(), actor, CombatStatusKind::Slow,
                           CombatantVisualLayer::Underlay,
                           CombatantVisualRegion::Feet));
}

TEST_CASE("Equipped Observe wires and unwires a whole-combatant underlay",
          "[combat-status][visuals][observe]") {
  BuiltEncounter h;
  const auto actor = h.defender();
  auto &reg = h.party_ctx.reg();

  REQUIRE(has_decal(reg, actor,
                    fl::widgets::effects::DecalAnimationKind::Observe));

  auto &member = reg.get<fl::ecs::components::PartyMember>(actor);
  REQUIRE(member.closet().unequip_skill(fl::skills::SkillId::Observe));

  REQUIRE_FALSE(has_decal(
      reg, actor, fl::widgets::effects::DecalAnimationKind::Observe));

  REQUIRE(member.closet().equip_skill(fl::skills::SkillId::Observe));
  REQUIRE(has_decal(reg, actor,
                    fl::widgets::effects::DecalAnimationKind::Observe));
}

TEST_CASE("Legacy targeted status visuals are discoverable without flattening",
          "[combat-status][visuals][legacy]") {
  BuiltEncounter h;
  const auto actor = h.defender();
  auto &reg = h.party_ctx.reg();

  reg.emplace_or_replace<fl::ecs::components::Poison>(
      actor, fl::ecs::components::Poison{});
  reg.emplace_or_replace<fl::ecs::components::DireBleed>(
      actor, fl::ecs::components::DireBleed{});
  reg.emplace_or_replace<fl::ecs::components::Freeze>(
      actor, fl::ecs::components::Freeze{});

  const auto visuals = fl::ecs::systems::combatant_status_visuals(reg, actor);
  const auto targeted_rows = std::ranges::count_if(visuals, [](const auto &v) {
    return v.layer == CombatantVisualLayer::StatusText &&
           v.region == CombatantVisualRegion::StatusRows &&
           v.preserves_existing_targeted_behavior;
  });
  REQUIRE(targeted_rows == 3);
  REQUIRE_FALSE(has_decal(
      reg, actor, fl::widgets::effects::DecalAnimationKind::PoisonCloud));
}

TEST_CASE("Starfire Drift custom underlay is bridged and preserved",
          "[combat-status][visuals][starfire]") {
  fl::GrandCentral gc{1, 1, 1};
  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto context = party_ctx.build_context();

  const auto entity = fl::primitives::EntityBuilder(context)
                          .monster(fl::monster::MonsterKind::StarfireAnomaly)
                          .build();

  REQUIRE(has_decal(party_ctx.reg(), entity,
                    fl::widgets::effects::DecalAnimationKind::Starfire));
  REQUIRE_FALSE(
      has_decal(party_ctx.reg(), entity,
                fl::widgets::effects::DecalAnimationKind::Projectile));
}
