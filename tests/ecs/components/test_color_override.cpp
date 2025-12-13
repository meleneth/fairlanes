// tests/ecs/components/color_override.test.cpp
#include <catch2/catch_test_macros.hpp>

#include <entt/entt.hpp>
#include <ftxui/component/component.hpp>

#include "fl/ecs/components/color_override.hpp"

namespace fl::ecs::components {

TEST_CASE("ColorOverride: safe_add_color does nothing for invalid entity",
          "[ecs][components][color_override]") {
  entt::registry reg;

  // entt::null is always invalid
  safe_add_color(reg, entt::null, ftxui::Color::Red);

  REQUIRE_FALSE(reg.any_of<ColorOverride>(entt::null));
}

TEST_CASE("ColorOverride: safe_clear_color does nothing for invalid entity",
          "[ecs][components][color_override]") {
  entt::registry reg;

  // Should not throw / crash
  safe_clear_color(reg, entt::null);

  REQUIRE_FALSE(reg.any_of<ColorOverride>(entt::null));
}

TEST_CASE(
    "ColorOverride: safe_add_color adds the component with the given color",
    "[ecs][components][color_override]") {
  entt::registry reg;
  const entt::entity e = reg.create();

  safe_add_color(reg, e, ftxui::Color::Red);

  REQUIRE(reg.any_of<ColorOverride>(e));
  REQUIRE(reg.get<ColorOverride>(e).color == ftxui::Color::Red);
}

TEST_CASE("ColorOverride: safe_add_color replaces existing ColorOverride",
          "[ecs][components][color_override]") {
  entt::registry reg;
  const entt::entity e = reg.create();

  safe_add_color(reg, e, ftxui::Color::Red);
  safe_add_color(reg, e, ftxui::Color::Yellow);

  REQUIRE(reg.any_of<ColorOverride>(e));
  REQUIRE(reg.get<ColorOverride>(e).color == ftxui::Color::Yellow);
}

TEST_CASE("ColorOverride: safe_clear_color removes the component if present",
          "[ecs][components][color_override]") {
  entt::registry reg;
  const entt::entity e = reg.create();

  safe_add_color(reg, e, ftxui::Color::Red);
  REQUIRE(reg.any_of<ColorOverride>(e));

  safe_clear_color(reg, e);

  REQUIRE_FALSE(reg.any_of<ColorOverride>(e));
}

TEST_CASE("ColorOverride: safe_clear_color is idempotent",
          "[ecs][components][color_override]") {
  entt::registry reg;
  const entt::entity e = reg.create();

  // No component yet; should be fine.
  safe_clear_color(reg, e);
  REQUIRE_FALSE(reg.any_of<ColorOverride>(e));

  // Add then clear twice; still fine.
  safe_add_color(reg, e, ftxui::Color::Red);
  safe_clear_color(reg, e);
  safe_clear_color(reg, e);

  REQUIRE_FALSE(reg.any_of<ColorOverride>(e));
}

} // namespace fl::ecs::components
