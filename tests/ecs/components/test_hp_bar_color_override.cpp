#include <catch2/catch_test_macros.hpp>

#include <entt/entt.hpp>
#include <ftxui/component/component.hpp>

#include "fl/ecs/components/hp_bar_color_override.hpp"
#include "fl/ecs/systems/visual_resolver.hpp"

namespace fl::ecs::components {

TEST_CASE(
    "HPBarColorOverride shim: safe_add_hp_bar_color ignores invalid entity",
    "[ecs][components][hp_bar_color_override]") {
  entt::registry reg;

  safe_add_hp_bar_color(reg, entt::null, ftxui::Color::Red);

  REQUIRE_FALSE(reg.any_of<StatusTint>(entt::null));
}

TEST_CASE("HPBarColorOverride shim: safe_add_hp_bar_color adds status tint",
          "[ecs][components][hp_bar_color_override]") {
  entt::registry reg;
  const auto entity = reg.create();

  safe_add_hp_bar_color(reg, entity, ftxui::Color::Red);
  safe_add_hp_bar_color(reg, entity, ftxui::Color::Yellow);

  REQUIRE(reg.any_of<StatusTint>(entity));
  REQUIRE(reg.get<StatusTint>(entity).hp_bar_color == ftxui::Color::Yellow);
}

TEST_CASE("HPBarColorOverride shim: resolver emits final HP bar color",
          "[ecs][components][hp_bar_color_override]") {
  entt::registry reg;
  const auto entity = reg.create();

  safe_add_hp_bar_color(reg, entity, ftxui::Color::Red);
  fl::ecs::systems::VisualResolver::resolve(reg, seerin::uWu{0});

  REQUIRE(reg.any_of<HPBarColorOverride>(entity));
  REQUIRE(reg.get<HPBarColorOverride>(entity).color == ftxui::Color::Red);
}

TEST_CASE(
    "HPBarColorOverride shim: safe_clear_hp_bar_color removes status tint",
    "[ecs][components][hp_bar_color_override]") {
  entt::registry reg;
  const auto entity = reg.create();

  safe_clear_hp_bar_color(reg, entity);
  safe_add_hp_bar_color(reg, entity, ftxui::Color::Red);
  safe_clear_hp_bar_color(reg, entity);
  safe_clear_hp_bar_color(reg, entity);

  REQUIRE_FALSE(reg.any_of<StatusTint>(entity));
}

} // namespace fl::ecs::components
