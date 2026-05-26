#include <string>

#include <catch2/catch_test_macros.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include "fl/ecs/components/atb_charge.hpp"
#include "fl/ecs/components/visual_effects.hpp"
#include "fl/grand_central.hpp"
#include "fl/widgets/combatant.hpp"
#include "fl/widgets/effects/decal.hpp"

TEST_CASE("Combatant renders ATB from ECS AtbCharge", "[widgets][combatant]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto entity = party_ctx.party_data().members().front().member_id();

  party_ctx.reg().emplace_or_replace<fl::ecs::components::AtbCharge>(entity,
                                                                     160, 4800);

  fl::widgets::Combatant combatant{party_ctx.reg(), entity, true};
  auto element = combatant.Render();

  auto screen = ftxui::Screen::Create(ftxui::Dimension::Fit(element),
                                      ftxui::Dimension::Fit(element));
  ftxui::Render(screen, element);

  const std::string rendered = screen.ToString();
  REQUIRE(rendered.find("ATB: [") != std::string::npos);
  REQUIRE(rendered.find("160/4800") != std::string::npos);
}

TEST_CASE("Combatant flame decal does not change fitted widget dimensions",
          "[widgets][combatant][flame]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto entity = party_ctx.party_data().members().front().member_id();

  fl::widgets::Combatant baseline{party_ctx.reg(), entity, true};
  auto baseline_element = baseline.Render();
  auto baseline_width = ftxui::Dimension::Fit(baseline_element).dimx;
  auto baseline_height = ftxui::Dimension::Fit(baseline_element).dimy;

  party_ctx.reg().emplace_or_replace<fl::ecs::components::CombatantDecals>(
      entity,
      fl::ecs::components::CombatantDecals{fl::ecs::components::DecalEffect{
          seerin::uWu{1000},
          fl::ecs::components::DecalEffect::Clock::now(),
          std::chrono::milliseconds{1000},
          fl::widgets::effects::DecalAnimationKind::FlameWave,
          {},
          2}});

  fl::widgets::Combatant with_flame{party_ctx.reg(), entity, true};
  auto flame_element = with_flame.Render();

  REQUIRE(ftxui::Dimension::Fit(flame_element).dimx == baseline_width);
  REQUIRE(ftxui::Dimension::Fit(flame_element).dimy == baseline_height);
}

TEST_CASE("Combatant non-flame decals do not change fitted widget dimensions",
          "[widgets][combatant][decal]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto entity = party_ctx.party_data().members().front().member_id();

  fl::widgets::Combatant baseline{party_ctx.reg(), entity, true};
  auto baseline_element = baseline.Render();
  auto baseline_width = ftxui::Dimension::Fit(baseline_element).dimx;
  auto baseline_height = ftxui::Dimension::Fit(baseline_element).dimy;

  party_ctx.reg().emplace_or_replace<fl::ecs::components::CombatantDecals>(
      entity,
      fl::ecs::components::CombatantDecals{fl::ecs::components::DecalEffect{
          seerin::uWu{1000}, fl::ecs::components::DecalEffect::Clock::now(),
          std::chrono::milliseconds{1000},
          fl::widgets::effects::DecalAnimationKind::Shock}});

  const auto &decal =
      party_ctx.reg().get<fl::ecs::components::CombatantDecals>(entity);
  REQUIRE(decal.effects.size() == 1);
  REQUIRE(decal.effects.front().animation_kind ==
          fl::widgets::effects::DecalAnimationKind::Shock);

  fl::widgets::Combatant with_decal{party_ctx.reg(), entity, true};
  auto decal_element = with_decal.Render();

  REQUIRE(ftxui::Dimension::Fit(decal_element).dimx == baseline_width);
  REQUIRE(ftxui::Dimension::Fit(decal_element).dimy == baseline_height);
}

TEST_CASE("Combatant can render multiple decals without changing dimensions",
          "[widgets][combatant][decal]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto entity = party_ctx.party_data().members().front().member_id();

  fl::widgets::Combatant baseline{party_ctx.reg(), entity, true};
  auto baseline_element = baseline.Render();
  auto baseline_width = ftxui::Dimension::Fit(baseline_element).dimx;
  auto baseline_height = ftxui::Dimension::Fit(baseline_element).dimy;

  fl::ecs::components::add_combatant_decal(
      party_ctx.reg(), entity,
      fl::ecs::components::DecalEffect{
          seerin::uWu{1000}, fl::ecs::components::DecalEffect::Clock::now(),
          std::chrono::milliseconds{1000},
          fl::widgets::effects::DecalAnimationKind::Shock});

  fl::widgets::effects::DecalConfig config;
  config.color = ftxui::Color::Red;
  config.hitpoints = 12;
  fl::ecs::components::add_combatant_decal(
      party_ctx.reg(), entity,
      fl::ecs::components::DecalEffect{
          seerin::uWu{1000}, fl::ecs::components::DecalEffect::Clock::now(),
          std::chrono::milliseconds{1000},
          fl::widgets::effects::DecalAnimationKind::HitpointNumber, config, 2});

  REQUIRE(party_ctx.reg()
              .get<fl::ecs::components::CombatantDecals>(entity)
              .effects.size() == 2);

  fl::widgets::Combatant with_decals{party_ctx.reg(), entity, true};
  auto decal_element = with_decals.Render();

  REQUIRE(ftxui::Dimension::Fit(decal_element).dimx == baseline_width);
  REQUIRE(ftxui::Dimension::Fit(decal_element).dimy == baseline_height);
}
