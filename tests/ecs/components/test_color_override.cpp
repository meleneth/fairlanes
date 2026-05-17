// tests/ecs/components/color_override.test.cpp
#include <catch2/catch_test_macros.hpp>

#include <entt/entt.hpp>
#include <ftxui/component/component.hpp>

#include "fl/ecs/components/color_override.hpp"

namespace fl::ecs::components {

TEST_CASE("ColorOverride shim: safe_add_color ignores invalid entity",
          "[ecs][components][color_override]") {
  entt::registry reg;

  safe_add_color(reg, entt::null, ftxui::Color::Red);

  REQUIRE_FALSE(reg.any_of<DamageFlash>(entt::null));
}

TEST_CASE("ColorOverride shim: safe_clear_color ignores invalid entity",
          "[ecs][components][color_override]") {
  entt::registry reg;

  safe_clear_color(reg, entt::null);

  REQUIRE_FALSE(reg.any_of<DamageFlash>(entt::null));
}

TEST_CASE("ColorOverride shim: safe_add_color records DamageFlash",
          "[ecs][components][color_override]") {
  entt::registry reg;
  const entt::entity e = reg.create();

  safe_add_color(reg, e, ftxui::Color::Red);

  REQUIRE(reg.any_of<DamageFlash>(e));
  REQUIRE(reg.get<DamageFlash>(e).color == ftxui::Color::Red);
  REQUIRE_FALSE(reg.any_of<ColorOverride>(e));
}

TEST_CASE("ColorOverride shim: safe_add_color replaces DamageFlash",
          "[ecs][components][color_override]") {
  entt::registry reg;
  const entt::entity e = reg.create();

  safe_add_color(reg, e, ftxui::Color::Red);
  safe_add_color(reg, e, ftxui::Color::Yellow);

  REQUIRE(reg.any_of<DamageFlash>(e));
  REQUIRE(reg.get<DamageFlash>(e).color == ftxui::Color::Yellow);
}

TEST_CASE("ColorOverride shim: safe_clear_color removes DamageFlash",
          "[ecs][components][color_override]") {
  entt::registry reg;
  const entt::entity e = reg.create();

  safe_add_color(reg, e, ftxui::Color::Red);
  REQUIRE(reg.any_of<DamageFlash>(e));

  safe_clear_color(reg, e);

  REQUIRE_FALSE(reg.any_of<DamageFlash>(e));
}

TEST_CASE("ColorOverride shim: safe_clear_color is idempotent",
          "[ecs][components][color_override]") {
  entt::registry reg;
  const entt::entity e = reg.create();

  safe_clear_color(reg, e);
  REQUIRE_FALSE(reg.any_of<DamageFlash>(e));

  safe_add_color(reg, e, ftxui::Color::Red);
  safe_clear_color(reg, e);
  safe_clear_color(reg, e);

  REQUIRE_FALSE(reg.any_of<DamageFlash>(e));
}

} // namespace fl::ecs::components
