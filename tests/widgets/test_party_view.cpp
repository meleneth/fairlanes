#include <optional>
#include <string>
#include <string_view>

#include <catch2/catch_test_macros.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include "fl/grand_central.hpp"
#include "fl/widgets/battle_render_budget.hpp"
#include "fl/widgets/party_view.hpp"

namespace {

std::optional<std::string> line_containing(const std::string &rendered,
                                           std::string_view needle) {
  std::size_t start = 0;
  while (start <= rendered.size()) {
    const auto end = rendered.find('\n', start);
    const auto line = rendered.substr(
        start, end == std::string::npos ? std::string::npos : end - start);
    if (line.find(needle) != std::string::npos)
      return line;
    if (end == std::string::npos)
      break;
    start = end + 1;
  }
  return std::nullopt;
}

} // namespace

TEST_CASE("Party inventory shares only the bottom row with player details",
          "[widgets][party][layout]") {
  fl::GrandCentral gc{1, 1, 5};
  fl::widgets::PartyView view{gc.account_context(0), 0};
  fl::widgets::force_battle_render_target(
      fl::widgets::BattleLayoutProfile::Wide);
  auto element = view.Render() | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 180) |
                 ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 48);
  auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(180),
                                      ftxui::Dimension::Fixed(48));
  ftxui::Render(screen, element);
  fl::widgets::clear_forced_battle_render_target();

  const auto inventory_line = line_containing(screen.ToString(), "Inventory");
  REQUIRE(inventory_line.has_value());
  REQUIRE(inventory_line->find("Player") != std::string::npos);
}
