#include <string>

#include <catch2/catch_test_macros.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include "fl/ecs/components/atb_charge.hpp"
#include "fl/grand_central.hpp"
#include "fl/widgets/combatant.hpp"

TEST_CASE("Combatant renders ATB from ECS AtbCharge", "[widgets][combatant]") {
  fl::GrandCentral gc{1, 1, 1};

  auto account_ctx = gc.account_context(0);
  auto party_ctx = account_ctx.party_context(0);
  auto entity = party_ctx.party_data().members().front().member_id();

  party_ctx.reg().emplace_or_replace<fl::ecs::components::AtbCharge>(entity,
                                                                      160,
                                                                      4800);

  fl::widgets::Combatant combatant{party_ctx.reg(), entity, true};
  auto element = combatant.Render();

  auto screen =
      ftxui::Screen::Create(ftxui::Dimension::Fit(element), ftxui::Dimension::Fit(element));
  ftxui::Render(screen, element);

  const std::string rendered = screen.ToString();
  REQUIRE(rendered.find("ATB: [") != std::string::npos);
  REQUIRE(rendered.find("160/4800") != std::string::npos);
}
