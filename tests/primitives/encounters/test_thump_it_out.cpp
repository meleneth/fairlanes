// tests/encounter_builder_tests.cpp
#include <catch2/catch_test_macros.hpp>

#include <entt/entt.hpp>

#include "fl/ecs/components/color_override.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/grand_central.hpp"
#include "fl/lospec500.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "sr/atb_events.hpp"

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
  REQUIRE(encounter.attackers().members().size() == 1);

  const entt::entity enemy = encounter.attackers().members().front();

  CHECK(party_ctx.reg().valid(enemy));
  CHECK(party_ctx.reg().all_of<fl::ecs::components::Stats>(enemy));

  REQUIRE(encounter.entities_to_cleanup().size() == 1);
  CHECK(encounter.entities_to_cleanup().front() == enemy);
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

TEST_CASE("EncounterBuilder::thump_it_out creates one enemy owned by encounter",
          "[encounter_builder][encounter][combat]") {
  fl::GrandCentral gc{1, 1, 3};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);
  auto &encounter = builder.thump_it_out();

  REQUIRE(encounter.attackers().members().size() == 1);

  const entt::entity enemy = encounter.attackers().members().front();

  CHECK(party_ctx.reg().valid(enemy));
  CHECK(encounter.owns_entity(enemy));
  CHECK(encounter.is_bad_guy(enemy));

  REQUIRE(encounter.entities_to_cleanup().size() == 1);
  CHECK(encounter.entities_to_cleanup().front() == enemy);
}

TEST_CASE("EncounterData::schedule_reek_fade applies and clears ColorOverride",
          "[encounter][timing][color]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);

  fl::primitives::EncounterBuilder builder(party_ctx);
  auto &encounter = builder.thump_it_out();

  const entt::entity target = encounter.defenders().members().front();
  auto &reg = party_ctx.reg();

  const auto from = fl::lospec500::color_at(4);
  const auto to = fl::lospec500::color_at(0);

  REQUIRE_FALSE(reg.any_of<fl::ecs::components::ColorOverride>(target));

  encounter.schedule_reek_fade(target, "test: fade", 1, 2, from, to);

  encounter.atb_in().emit(seerin::AtbInEvent{seerin::Beat{}});
  REQUIRE(reg.get<fl::ecs::components::ColorOverride>(target)
              .color.Print(false) ==
          ftxui::Color::Interpolate(0.0F, from, to).Print(false));

  encounter.atb_in().emit(seerin::AtbInEvent{seerin::Beat{}});
  REQUIRE(reg.get<fl::ecs::components::ColorOverride>(target)
              .color.Print(false) ==
          ftxui::Color::Interpolate(1.0F, from, to).Print(false));

  encounter.atb_in().emit(seerin::AtbInEvent{seerin::Beat{}});
  REQUIRE_FALSE(reg.any_of<fl::ecs::components::ColorOverride>(target));
}

} // namespace
