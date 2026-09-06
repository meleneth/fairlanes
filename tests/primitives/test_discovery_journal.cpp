#include <catch2/catch_test_macros.hpp>

#include "fl/ecs/components/monster_identity.hpp"
#include "fl/grand_central.hpp"
#include "fl/primitives/discovery_journal.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "fl/primitives/entity_builder.hpp"
#include "fl/skills/skill_definition.hpp"
#include "fl/skills/skill_sequence.hpp"

using fl::monster::MonsterKind;
using fl::skills::SkillId;

TEST_CASE("Bestiary reveals only skills witnessed from that monster",
          "[discovery]") {
  fl::primitives::DiscoveryJournal journal;
  REQUIRE(journal.skill_slots(MonsterKind::ScaredyCat).empty());
  journal.encounter(MonsterKind::ScaredyCat);
  auto slots = journal.skill_slots(MonsterKind::ScaredyCat);
  REQUIRE(slots.size() == 2);
  REQUIRE_FALSE(slots[0]);
  REQUIRE_FALSE(slots[1]);

  journal.witness(SkillId::Thump, MonsterKind::FieldMouse);
  REQUIRE(journal.knows(SkillId::Thump));
  REQUIRE_FALSE(journal.skill_slots(MonsterKind::ScaredyCat)[0]);
  journal.witness(SkillId::Thump, MonsterKind::ScaredyCat);
  slots = journal.skill_slots(MonsterKind::ScaredyCat);
  REQUIRE_FALSE(slots[0]);
  REQUIRE(slots[1] == SkillId::Thump);
  journal.witness(SkillId::Thump, MonsterKind::ScaredyCat);
  REQUIRE(journal.skill_slots(MonsterKind::ScaredyCat) == slots);
}

TEST_CASE("Discovery listener disconnects before its captures die",
          "[discovery]") {
  entt::registry reg;
  fl::events::PartyBus bus;
  fl::primitives::DiscoveryJournal journal;
  const auto user = reg.create();
  reg.emplace<fl::ecs::components::MonsterIdentity>(user,
                                                    MonsterKind::FieldMouse);
  {
    fl::primitives::DiscoveryJournalListener listener{journal, reg};
    listener.bind(bus);
    bus.emit(fl::events::PartyEvent{
        fl::events::MonsterEncountered{MonsterKind::FieldMouse}});
    bus.emit(fl::events::PartyEvent{
        fl::events::SkillWitnessed{user, SkillId::Thump}});
    REQUIRE(journal.knows(SkillId::Thump));
  }
  bus.emit(
      fl::events::PartyEvent{fl::events::SkillWitnessed{user, SkillId::Flee}});
  REQUIRE_FALSE(journal.knows(SkillId::Flee));
}

TEST_CASE("Normal accounts share discovery and attract games leave it unwired",
          "[discovery]") {
  fl::GrandCentral game{2, 1, 1};
  game.accounts()[0].party(0).party_bus().emit(fl::events::PartyEvent{
      fl::events::MonsterEncountered{MonsterKind::FieldMouse}});
  game.accounts()[1].party(0).party_bus().emit(
      fl::events::PartyEvent{fl::events::SkillWitnessed{
          game.accounts()[1].party(0).members()[0].member_id(),
          SkillId::Observe}});
  REQUIRE(game.discoveries().monsters().contains(MonsterKind::FieldMouse));
  REQUIRE(game.discoveries().knows(SkillId::Observe));

  fl::GrandCentral attract{1, 1, 1, false};
  attract.accounts()[0].party(0).party_bus().emit(fl::events::PartyEvent{
      fl::events::MonsterEncountered{MonsterKind::FieldMouse}});
  attract.accounts()[0].party(0).party_bus().emit(fl::events::PartyEvent{
      fl::events::SkillWitnessed{entt::null, SkillId::Thump}});
  REQUIRE(attract.discoveries().monsters().empty());
  REQUIRE_FALSE(attract.discoveries().knows(SkillId::Thump));
}

TEST_CASE("Every libram skill has reference prose", "[discovery][content]") {
  for (const auto *definition : fl::skills::all_definitions()) {
    CAPTURE(definition->display_name);
    REQUIRE_FALSE(definition->description.empty());
  }
}

TEST_CASE("Encounter and action producers publish discovery without learning",
          "[discovery]") {
  fl::GrandCentral game{1, 1, 1};
  auto ctx = game.account_context(0).party_context(0);
  auto &encounter = ctx.party_data().create_encounter();
  auto build = ctx.build_context();
  const auto mouse = fl::primitives::EntityBuilder{build}
                         .monster(MonsterKind::FieldMouse)
                         .build();
  fl::primitives::EncounterBuilder builder{ctx};
  builder.add_to_enemy_team(mouse);
  REQUIRE(game.discoveries().monsters().contains(MonsterKind::FieldMouse));
  REQUIRE_FALSE(game.discoveries().knows(SkillId::Thump));
  fl::skills::SkillSequencer sequencer{ctx, encounter.atb_engine().scheduler(),
                                       [](entt::entity) {}};
  sequencer.schedule(mouse, ctx.party_data().members()[0].member_id(),
                     SkillId::Thump);
  REQUIRE(game.discoveries().knows(SkillId::Thump));
  REQUIRE(game.discoveries().skill_slots(MonsterKind::FieldMouse)[0] ==
          SkillId::Thump);
}
