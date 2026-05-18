#include <catch2/catch_test_macros.hpp>

#include <chrono>

#include <entt/entt.hpp>
#include <ftxui/screen/color.hpp>

#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/visual_effects.hpp"
#include "fl/ecs/systems/visual_resolver.hpp"
#include "fl/lospec500.hpp"
#include "sr/uWu.hpp"

namespace {

std::string print(ftxui::Color color) { return color.Print(false); }

} // namespace

TEST_CASE("VisualResolver expires transient visual effects",
          "[ecs][systems][visual]") {
  entt::registry reg;
  const auto entity = reg.create();

  reg.emplace<fl::ecs::components::DamageFlash>(
      entity,
      fl::ecs::components::DamageFlash{ftxui::Color::Red, seerin::uWu{10}});

  fl::ecs::systems::VisualResolver::resolve(reg, seerin::uWu{9});
  REQUIRE(reg.any_of<fl::ecs::components::DamageFlash>(entity));
  REQUIRE(reg.any_of<fl::ecs::components::ResolvedColorOverride>(entity));

  fl::ecs::systems::VisualResolver::resolve(reg, seerin::uWu{10});
  REQUIRE_FALSE(reg.any_of<fl::ecs::components::DamageFlash>(entity));
  REQUIRE_FALSE(reg.any_of<fl::ecs::components::ResolvedColorOverride>(entity));
}

TEST_CASE("VisualResolver removes FlameWaveDecal after render duration",
          "[ecs][visuals][flame]") {
  entt::registry reg;
  const auto entity = reg.create();

  reg.emplace<fl::ecs::components::FlameWaveDecal>(
      entity, fl::ecs::components::FlameWaveDecal{
                  seerin::uWu{1000},
                  fl::ecs::components::FlameWaveDecal::Clock::now() -
                      std::chrono::milliseconds{1000},
                  std::chrono::milliseconds{1000}});

  fl::ecs::systems::VisualResolver::resolve_entity(reg, entity, seerin::uWu{1});

  REQUIRE_FALSE(reg.any_of<fl::ecs::components::FlameWaveDecal>(entity));
}

TEST_CASE("VisualResolver gives dead visuals precedence over effects",
          "[ecs][systems][visual]") {
  entt::registry reg;
  const auto entity = reg.create();

  auto &stats = reg.emplace<fl::ecs::components::Stats>(entity);
  stats.hp_ = 0;
  reg.emplace<fl::ecs::components::DamageFlash>(
      entity,
      fl::ecs::components::DamageFlash{ftxui::Color::Red, seerin::uWu{100}});
  reg.emplace<fl::ecs::components::StatusTint>(
      entity,
      fl::ecs::components::StatusTint{ftxui::Color::Green, ftxui::Color::Yellow,
                                      ftxui::Color::Blue});
  reg.emplace<fl::ecs::components::ResolvedBackgroundColorOverride>(
      entity,
      fl::ecs::components::ResolvedBackgroundColorOverride{ftxui::Color::Blue});

  fl::ecs::systems::VisualResolver::resolve(reg, seerin::uWu{1});

  REQUIRE(print(reg.get<fl::ecs::components::ResolvedColorOverride>(entity)
                    .color) == print(fl::lospec500::color_at(6)));
  REQUIRE_FALSE(
      reg.any_of<fl::ecs::components::ResolvedHPBarColorOverride>(entity));
  REQUIRE_FALSE(
      reg.any_of<fl::ecs::components::ResolvedBackgroundColorOverride>(entity));
}

TEST_CASE("VisualResolver resolves overlapping effects deterministically",
          "[ecs][systems][visual]") {
  entt::registry reg;
  const auto entity = reg.create();

  reg.emplace<fl::ecs::components::BaseVisual>(
      entity, fl::ecs::components::BaseVisual{ftxui::Color::Blue});
  reg.emplace<fl::ecs::components::StatusTint>(
      entity, fl::ecs::components::StatusTint{ftxui::Color::Green,
                                              ftxui::Color::Yellow});
  reg.emplace<fl::ecs::components::ActiveGlow>(
      entity,
      fl::ecs::components::ActiveGlow{ftxui::Color::Cyan, seerin::uWu{100}});
  reg.emplace<fl::ecs::components::DamageFlash>(
      entity,
      fl::ecs::components::DamageFlash{ftxui::Color::Red, seerin::uWu{100}});

  fl::ecs::systems::VisualResolver::resolve(reg, seerin::uWu{1});

  REQUIRE(reg.get<fl::ecs::components::ResolvedColorOverride>(entity).color ==
          ftxui::Color::Red);
  REQUIRE(
      reg.get<fl::ecs::components::ResolvedHPBarColorOverride>(entity).color ==
      ftxui::Color::Yellow);

  reg.get<fl::ecs::components::StatusTint>(entity).background_color =
      ftxui::Color::BlueLight;
  fl::ecs::systems::VisualResolver::resolve(reg, seerin::uWu{1});
  REQUIRE(reg.get<fl::ecs::components::ResolvedBackgroundColorOverride>(entity)
              .color == ftxui::Color::BlueLight);

  reg.remove<fl::ecs::components::DamageFlash>(entity);
  fl::ecs::systems::VisualResolver::resolve(reg, seerin::uWu{2});
  REQUIRE(reg.get<fl::ecs::components::ResolvedColorOverride>(entity).color ==
          ftxui::Color::Cyan);

  reg.remove<fl::ecs::components::ActiveGlow>(entity);
  fl::ecs::systems::VisualResolver::resolve(reg, seerin::uWu{3});
  REQUIRE(reg.get<fl::ecs::components::ResolvedColorOverride>(entity).color ==
          ftxui::Color::Green);

  reg.remove<fl::ecs::components::StatusTint>(entity);
  fl::ecs::systems::VisualResolver::resolve(reg, seerin::uWu{4});
  REQUIRE(reg.get<fl::ecs::components::ResolvedColorOverride>(entity).color ==
          ftxui::Color::Blue);
}
