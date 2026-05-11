// tests/monsters/test_field_mouse.cpp
#include <catch2/catch_test_macros.hpp>

#include "fl/ecs/components/stats.hpp"
#include "fl/grand_central.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/primitives/entity_builder.hpp"

TEST_CASE("Field mouse monster archetype has expected stats", "[monsters]") {
	fl::GrandCentral gc{1, 1, 1};
	auto account_ctx = gc.account_context(0);
	auto party_ctx = account_ctx.party_context(0);

	auto context = party_ctx.build_context();
	const auto e = fl::primitives::EntityBuilder(context)
										 .monster(fl::monster::MonsterKind::FieldMouse)
										 .build();

	const auto &stats = party_ctx.reg().get<fl::ecs::components::Stats>(e);
	REQUIRE(stats.name_ == "Field Mouse");
	REQUIRE(stats.hp_ == 5);
	REQUIRE(stats.max_hp_ == 5);
}

TEST_CASE("Honey badger monster archetype has expected stats", "[monsters]") {
	fl::GrandCentral gc{1, 1, 1};
	auto account_ctx = gc.account_context(0);
	auto party_ctx = account_ctx.party_context(0);

	auto context = party_ctx.build_context();
	const auto e = fl::primitives::EntityBuilder(context)
										 .monster(fl::monster::MonsterKind::HoneyBadger)
										 .build();

	const auto &stats = party_ctx.reg().get<fl::ecs::components::Stats>(e);
	REQUIRE(stats.name_ == "Honey Badger");
	REQUIRE(stats.hp_ == 500);
	REQUIRE(stats.max_hp_ == 500);
}
