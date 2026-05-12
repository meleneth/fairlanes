#include <catch2/catch_test_macros.hpp>

#include <optional>

#include "fl/context.hpp"
#include "fl/ecs/components/dire_bleed.hpp"
#include "fl/ecs/components/hp_bar_color_override.hpp"
#include "fl/ecs/components/monster_identity.hpp"
#include "fl/ecs/components/skill_slots.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/take_damage.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/grand_central.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "fl/primitives/entity_builder.hpp"
#include "fl/primitives/world_clock.hpp"
#include "fl/skills/skill_learning.hpp"
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

TEST_CASE("EncounterBuilder::thump_it_out adds one enemy attacker",
          "[encounter][builder]") {
  fl::GrandCentral gc{1, 1, 3};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);

  auto &encounter = builder.thump_it_out();

  REQUIRE(encounter.attackers().members().size() == 1);
  REQUIRE(encounter.entities_to_cleanup().size() == 1);

  auto attacker = encounter.attackers().members().front();
  REQUIRE(encounter.entities_to_cleanup().front() == attacker);
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

  (void)party_ctx.party_data().party_bus().on<fl::events::PlayerDied>(
      [&](const fl::events::PlayerDied &ev) {
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
  REQUIRE(party_ctx.reg().all_of<fl::ecs::components::HPBarColorOverride>(
      defender));
  REQUIRE(party_ctx.reg()
              .get<fl::ecs::components::DireBleed>(defender)
              .damage_per_tick == 10);

  tick_party(party_ctx, 35);
  REQUIRE(stats.hp_ == 100);

  tick_party(party_ctx, 1);
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
  REQUIRE_FALSE(party_ctx.reg().any_of<fl::ecs::components::HPBarColorOverride>(
      defender));
  REQUIRE(party_ctx.reg().get<fl::ecs::components::Stats>(defender).hp_ == 0);

  tick_party(party_ctx, 36);
  REQUIRE_FALSE(
      party_ctx.reg().any_of<fl::ecs::components::DireBleed>(defender));
  REQUIRE_FALSE(party_ctx.reg().any_of<fl::ecs::components::HPBarColorOverride>(
      defender));
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
  encounter.atb_in().emit(seerin::AtbInEvent{seerin::AddCombatant{defender}});

  auto honey_badger = add_honey_badger(party_ctx, encounter);
  encounter.atb_out().emit(seerin::AtbOutEvent{seerin::BecameActive{
      honey_badger,
  }});
  tick_party(party_ctx, 24);
  REQUIRE(party_ctx.reg().all_of<fl::ecs::components::DireBleed>(defender));
  REQUIRE(party_ctx.reg().all_of<fl::ecs::components::HPBarColorOverride>(
      defender));

  party.leave_combat();

  REQUIRE_FALSE(
      party_ctx.reg().any_of<fl::ecs::components::DireBleed>(defender));
  REQUIRE_FALSE(party_ctx.reg().any_of<fl::ecs::components::HPBarColorOverride>(
      defender));
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
  encounter.atb_in().emit(seerin::AtbInEvent{seerin::AddCombatant{defender}});

  auto honey_badger = add_honey_badger(party_ctx, encounter);
  encounter.atb_out().emit(seerin::AtbOutEvent{seerin::BecameActive{
      honey_badger,
  }});
  tick_party(party_ctx, 24);
  REQUIRE(party_ctx.reg().all_of<fl::ecs::components::DireBleed>(defender));

  party_ctx.reg().destroy(honey_badger);
  tick_party(party_ctx, fl::primitives::WorldClock::beats_from_seconds(3));

  REQUIRE_FALSE(
      party_ctx.reg().any_of<fl::ecs::components::DireBleed>(defender));
  REQUIRE_FALSE(party_ctx.reg().any_of<fl::ecs::components::HPBarColorOverride>(
      defender));
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
