#include <catch2/catch_test_macros.hpp>

#include <entt/entt.hpp>
#include <ftxui/component/component.hpp>

#include "fl/ecs/components/hp_bar_color_override.hpp"

namespace fl::ecs::components {

TEST_CASE("HPBarColorOverride: safe_add_hp_bar_color ignores invalid entity",
          "[ecs][components][hp_bar_color_override]") {
  entt::registry reg;

  safe_add_hp_bar_color(reg, entt::null, ftxui::Color::Red);

  REQUIRE_FALSE(reg.any_of<HPBarColorOverride>(entt::null));
}

TEST_CASE("HPBarColorOverride: safe_add_hp_bar_color adds and replaces color",
          "[ecs][components][hp_bar_color_override]") {
  entt::registry reg;
  const auto entity = reg.create();

  safe_add_hp_bar_color(reg, entity, ftxui::Color::Red);
  safe_add_hp_bar_color(reg, entity, ftxui::Color::Yellow);

  REQUIRE(reg.any_of<HPBarColorOverride>(entity));
  REQUIRE(reg.get<HPBarColorOverride>(entity).color == ftxui::Color::Yellow);
}

TEST_CASE("HPBarColorOverride: safe_clear_hp_bar_color removes component",
          "[ecs][components][hp_bar_color_override]") {
  entt::registry reg;
  const auto entity = reg.create();

  safe_clear_hp_bar_color(reg, entity);
  safe_add_hp_bar_color(reg, entity, ftxui::Color::Red);
  safe_clear_hp_bar_color(reg, entity);
  safe_clear_hp_bar_color(reg, entity);

  REQUIRE_FALSE(reg.any_of<HPBarColorOverride>(entity));
}

} // namespace fl::ecs::components
