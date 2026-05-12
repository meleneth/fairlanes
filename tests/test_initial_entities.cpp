// tests/test_entity_builder.cpp
#include <catch2/catch_test_macros.hpp>
#include <entt/entt.hpp>
#include <nlohmann/json.hpp>

#include "fl/context.hpp"
#include "fl/ecs/components/skill_slots.hpp"
#include "fl/ecs/components/tags.hpp"
#include "fl/grand_central.hpp"
#include "fl/primitives/component_builder.hpp"
#include "fl/primitives/entity_builder.hpp"
#include "fl/primitives/random_hub.hpp"
#include "fl/widgets/fancy_log.hpp"

using json = nlohmann::json;

using fl::ecs::components::Stats;
using fl::ecs::components::Tags;
using fl::primitives::ComponentBuilder;
using fl::primitives::EntityBuilder;

TEST_CASE("EntityBuilder + ComponentBuilder basics", "[entity][builder]") {
  fl::GrandCentral gc{1, 1, 1};
  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto &reg = gc.reg();

  SECTION("Stats defaults are installed") {
    auto build_ctx = party_ctx.build_context();
    auto e = EntityBuilder{build_ctx}.with_default<Stats>().build();
    const auto &s = reg.get<Stats>(e);
    REQUIRE(s.hp_ == 10);
    REQUIRE(s.mp_ == 0);
  }

  SECTION("Stats JSON overrides apply on top of defaults") {
    auto build_ctx = party_ctx.build_context();
    auto e = EntityBuilder{build_ctx}.with_default<Stats>().build();
    auto &s = reg.get<Stats>(e);

    json j = R"({ "hp": 25, "mp": 3 })"_json;
    ComponentBuilder<Stats>::apply(s, j);

    REQUIRE(s.hp_ == 25);
    REQUIRE(s.mp_ == 3);
  }

  SECTION("Tags default + JSON array assignment") {
    using fl::ecs::components::Tags;
    auto build_ctx = party_ctx.build_context();
    auto e = EntityBuilder{build_ctx}.with_default<Tags>().build();
    auto &t = reg.get<Tags>(e);

    json j = R"({ "values": ["dps", "glass"] })"_json;
    ComponentBuilder<Tags>::apply(t, j);

    REQUIRE(t.values.size() == 2);
    REQUIRE(t.values[0] == "dps");
    REQUIRE(t.values[1] == "glass");
  }

  SECTION("Preset/trait composition via a lambda") {
    auto build_ctx = party_ctx.build_context();
    auto e = EntityBuilder{build_ctx}
                 .with_default<Stats>()
                 .with_default<Tags>()
                 .build();

    // a "trait": GlassCannon
    auto glass_cannon = [&](entt::entity x) {
      auto &s = reg.get<Stats>(x);
      s.hp_ = 1;
      s.mp_ = 50;

      auto &t = reg.get<Tags>(x);
      t.values = {"glass", "dps"};
    };

    glass_cannon(e);

    const auto &s = reg.get<Stats>(e);
    const auto &t = reg.get<Tags>(e);
    REQUIRE(s.hp_ == 1);
    REQUIRE(s.mp_ == 50);
    REQUIRE(t.values.size() == 2);
  }
  SECTION("Monster/Field Mouse") {
    auto build_ctx = party_ctx.build_context();
    auto e = EntityBuilder{build_ctx}
                 .monster(fl::monster::MonsterKind::FieldMouse)
                 .build();
    const auto &stats = reg.get<Stats>(e);
    REQUIRE(stats.hp_ == 5);
    REQUIRE(stats.name_ == "Field Mouse");
  }

  SECTION("Initial players know Observe and have open skill slots") {
    const auto member = party_ctx.party_data().members().front().member_id();
    const auto &skills = reg.get<fl::ecs::components::SkillSlots>(member);

    REQUIRE(skills.knows(fl::skills::SkillId::Observe));
    REQUIRE(skills.has_open_slot());
  }
}
