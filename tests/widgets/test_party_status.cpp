#include <string>

#include <catch2/catch_test_macros.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include "fl/ecs/components/stats.hpp"
#include "fl/grand_central.hpp"
#include "fl/widgets/party_status.hpp"

TEST_CASE("PartyStatus keeps every noncombat state to three lines",
          "[widgets][party_status][layout]") {
  fl::GrandCentral gc{1, 1, 1};
  auto party_ctx = gc.account_context(0).party_context(0);
  auto &party = party_ctx.party_data();

  fl::widgets::PartyStatus idle_status{party};
  auto idle = idle_status.Render();
  REQUIRE(ftxui::Dimension::Fit(idle).dimy == 3);

  party.start_town_penalty();
  fl::widgets::PartyStatus fixing_status{party};
  auto fixing = fixing_status.Render();
  REQUIRE(ftxui::Dimension::Fit(fixing).dimy == 3);
}

TEST_CASE("Dead PartyStatus keeps its label in a three-line frame",
          "[widgets][party_status]") {
  fl::GrandCentral gc{1, 1, 1};
  auto party_ctx = gc.account_context(0).party_context(0);
  auto &party = party_ctx.party_data();

  for (const auto &member : party.members()) {
    party_ctx.reg().get<fl::ecs::components::Stats>(member.member_id()).hp_ = 0;
  }

  fl::widgets::PartyStatus status{party};
  auto element = status.Render();
  const auto dimensions = ftxui::Dimension::Fit(element);

  REQUIRE(dimensions.dimy == 3);

  auto screen = ftxui::Screen::Create(ftxui::Dimension::Fit(element),
                                      ftxui::Dimension::Fit(element));
  ftxui::Render(screen, element);
  const std::string rendered = screen.ToString();
  const auto first_newline = rendered.find('\n');

  REQUIRE(first_newline != std::string::npos);
  REQUIRE(rendered.substr(0, first_newline).find("DEAD") != std::string::npos);
  REQUIRE(rendered.substr(first_newline + 1).find("DEAD") == std::string::npos);
}
