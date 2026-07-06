#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <catch2/catch_test_macros.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include "fl/grand_central.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "fl/widgets/all_account_battle_screen.hpp"
#include "fl/widgets/battle_render_budget.hpp"
#include "fl/widgets/chaos_attract_root.hpp"
#include "fl/widgets/root_chrome.hpp"
#include "fl/widgets/root_component.hpp"

namespace {

constexpr int kRenderWidth = 120;
constexpr int kRenderHeight = 40;

std::string render_to_string(fl::widgets::RootComponent &root) {
  auto element = root.Render() | ftxui::size(ftxui::WIDTH, ftxui::EQUAL,
                                             kRenderWidth) |
                 ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, kRenderHeight);
  auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(kRenderWidth),
                                      ftxui::Dimension::Fixed(kRenderHeight));
  ftxui::Render(screen, element);
  return screen.ToString();
}

std::string render_to_string(fl::widgets::ChaosAttractRoot &root) {
  auto element = root.Render() | ftxui::size(ftxui::WIDTH, ftxui::EQUAL,
                                             kRenderWidth) |
                 ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, kRenderHeight);
  auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(kRenderWidth),
                                      ftxui::Dimension::Fixed(kRenderHeight));
  ftxui::Render(screen, element);
  return screen.ToString();
}

std::string render_to_string(fl::widgets::ChaosAttractRoot &root,
                             const fl::primitives::WorldClock &world_clock,
                             int width = kRenderWidth,
                             int height = kRenderHeight) {
  auto element = fl::widgets::render_root_chrome(world_clock, root.Render()) |
                 ftxui::size(ftxui::WIDTH, ftxui::EQUAL, width) |
                 ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, height);
  auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(width),
                                      ftxui::Dimension::Fixed(height));
  ftxui::Render(screen, element);
  return screen.ToString();
}

std::optional<ftxui::Pixel>
find_prompt_pixel(fl::widgets::ChaosAttractRoot &root) {
  constexpr std::string_view prompt = "ENTER TO BEGIN";
  auto element = root.Render() | ftxui::size(ftxui::WIDTH, ftxui::EQUAL,
                                             kRenderWidth) |
                 ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, kRenderHeight);
  auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(kRenderWidth),
                                      ftxui::Dimension::Fixed(kRenderHeight));
  ftxui::Render(screen, element);

  for (int y = 0; y < kRenderHeight; ++y) {
    for (int x = 0;
         x <= kRenderWidth - static_cast<int>(prompt.size()); ++x) {
      bool matches = true;
      for (std::size_t i = 0; i < prompt.size(); ++i) {
        if (screen.PixelAt(x + static_cast<int>(i), y).character !=
            std::string(1, prompt[i])) {
          matches = false;
          break;
        }
      }

      if (matches) {
        return screen.PixelAt(x, y);
      }
    }
  }

  return std::nullopt;
}

std::optional<std::size_t> find_line_containing(const std::string &rendered,
                                                std::string_view needle) {
  std::size_t line_index = 0;
  std::size_t start = 0;
  while (start <= rendered.size()) {
    const auto end = rendered.find('\n', start);
    const auto line =
        rendered.substr(start, end == std::string::npos ? std::string::npos
                                                        : end - start);
    if (line.find(needle) != std::string::npos) {
      return line_index;
    }
    if (end == std::string::npos) {
      break;
    }
    start = end + 1;
    ++line_index;
  }

  return std::nullopt;
}

std::optional<std::size_t>
find_line_containing(const std::string &rendered, std::string_view first,
                     std::string_view second) {
  std::size_t line_index = 0;
  std::size_t start = 0;
  while (start <= rendered.size()) {
    const auto end = rendered.find('\n', start);
    const auto line =
        rendered.substr(start, end == std::string::npos ? std::string::npos
                                                        : end - start);
    if (line.find(first) != std::string::npos &&
        line.find(second) != std::string::npos) {
      return line_index;
    }
    if (end == std::string::npos) {
      break;
    }
    start = end + 1;
    ++line_index;
  }

  return std::nullopt;
}

std::optional<std::string> line_at(const std::string &rendered,
                                   std::size_t target_line) {
  std::size_t line_index = 0;
  std::size_t start = 0;
  while (start <= rendered.size()) {
    const auto end = rendered.find('\n', start);
    if (line_index == target_line) {
      return rendered.substr(start, end == std::string::npos
                                        ? std::string::npos
                                        : end - start);
    }
    if (end == std::string::npos) {
      break;
    }
    start = end + 1;
    ++line_index;
  }

  return std::nullopt;
}

fl::widgets::RootComponent make_gameplay_root(fl::GrandCentral &gc) {
  return fl::widgets::RootComponent{gc.account_context(0), gc.accounts(),
                                    gc.game_log(), gc.world_clock()};
}

ftxui::Component make_all_account_surface(fl::GrandCentral &gc) {
  return ftxui::Make<fl::widgets::AllAccountBattleScreen>(gc.reg(),
                                                          gc.accounts());
}

} // namespace

TEST_CASE("Normal gameplay renders the normal key manifest",
          "[widgets][root][presentation]") {
  fl::GrandCentral gc{1, 1, 1};
  auto root = make_gameplay_root(gc);

  const auto rendered = render_to_string(root);
  REQUIRE(rendered.find("+/-") != std::string::npos);
  REQUIRE(rendered.find("overdrive") != std::string::npos);
  REQUIRE(rendered.find("help") != std::string::npos);
}

TEST_CASE("Chaos Attract hides the normal key manifest",
          "[widgets][root][presentation]") {
  fl::GrandCentral attract_gc{8, 5, 5};
  fl::widgets::ChaosAttractRoot root{make_all_account_surface(attract_gc)};

  const auto rendered = render_to_string(root, attract_gc.world_clock());
  REQUIRE(rendered.find("+/-") == std::string::npos);
  REQUIRE(rendered.find("overdrive") == std::string::npos);
  REQUIRE(rendered.find("help") == std::string::npos);
  REQUIRE(rendered.find("A1 P1") != std::string::npos);
  REQUIRE(rendered.find("*") != std::string::npos);
  REQUIRE(rendered.find("Moons") != std::string::npos);
}

TEST_CASE("Chaos Attract renders the begin prompt",
          "[widgets][root][presentation]") {
  fl::GrandCentral attract_gc{8, 5, 5};
  fl::widgets::ChaosAttractRoot root{make_all_account_surface(attract_gc)};

  const auto rendered = render_to_string(root, attract_gc.world_clock());
  REQUIRE(rendered.find("ENTER TO BEGIN") != std::string::npos);
  REQUIRE(rendered.find("A1 P1") != std::string::npos);
  REQUIRE(rendered.find("Selected battle") != std::string::npos);
  REQUIRE(rendered.find("log") != std::string::npos);
  REQUIRE(rendered.find("HP: [") != std::string::npos);
  REQUIRE(rendered.find("log") < rendered.find("Selected battle"));
}

TEST_CASE("Chaos Attract selected battle omits roster introduction lines",
          "[widgets][root][presentation]") {
  fl::GrandCentral attract_gc{8, 5, 5};
  auto party_ctx = attract_gc.account_context(0).party_context(0);
  fl::primitives::EncounterBuilder{party_ctx}.thump_it_out();

  fl::widgets::ChaosAttractRoot root{make_all_account_surface(attract_gc)};

  fl::widgets::force_battle_render_target(fl::widgets::BattleLayoutProfile::Wide);
  const auto rendered =
      render_to_string(root, attract_gc.world_clock(), 180, 50);
  fl::widgets::clear_forced_battle_render_target();

  REQUIRE(rendered.find("Party: A1 P1") == std::string::npos);
  REQUIRE(rendered.find("Alderion(L1)") == std::string::npos);
  REQUIRE(rendered.find("Bramblethorn(L1)") == std::string::npos);
  REQUIRE(rendered.find("Enemies") == std::string::npos);
  REQUIRE(rendered.find("No encounter") == std::string::npos);
}

TEST_CASE("Chaos Attract renders selector rows in two-line mode",
          "[widgets][root][presentation]") {
  fl::GrandCentral attract_gc{8, 5, 5};
  fl::widgets::ChaosAttractRoot root{make_all_account_surface(attract_gc)};

  const auto rendered = render_to_string(root, attract_gc.world_clock());
  const auto identity_line = find_line_containing(rendered, "*", "A1 P1");

  REQUIRE(identity_line.has_value());
  const auto matchup_line = line_at(rendered, *identity_line + 1);
  REQUIRE(matchup_line.has_value());
  REQUIRE(matchup_line->find(" -> ") != std::string::npos);
}

TEST_CASE("Chaos Attract keeps the selected party log above the battle area",
          "[widgets][root][presentation]") {
  fl::GrandCentral attract_gc{8, 5, 5};
  fl::widgets::ChaosAttractRoot root{make_all_account_surface(attract_gc)};

  const auto rendered = render_to_string(root, attract_gc.world_clock());
  const auto log_line = find_line_containing(rendered, " log");
  const auto battle_line = find_line_containing(rendered, "Selected battle");

  REQUIRE(log_line.has_value());
  REQUIRE(battle_line.has_value());
  REQUIRE(*log_line < *battle_line);
}

TEST_CASE("Chaos Attract overview spends wide rows on player names",
          "[widgets][root][presentation]") {
  fl::GrandCentral attract_gc{8, 5, 5};
  fl::widgets::ChaosAttractRoot root{make_all_account_surface(attract_gc)};

  fl::widgets::force_battle_render_target(fl::widgets::BattleLayoutProfile::Wide);
  const auto rendered =
      render_to_string(root, attract_gc.world_clock(), 180, 50);
  fl::widgets::clear_forced_battle_render_target();

  REQUIRE(rendered.find("Alderion(10)") != std::string::npos);
  REQUIRE(rendered.find("Bramblethorn(10)") != std::string::npos);
}

TEST_CASE("Chaos Attract prompt color changes across frames",
          "[widgets][root][presentation]") {
  fl::GrandCentral attract_gc{8, 5, 5};
  fl::widgets::ChaosAttractRoot root{make_all_account_surface(attract_gc)};

  const auto first = find_prompt_pixel(root);
  REQUIRE(first.has_value());

  for (int i = 0; i < 12; ++i) {
    (void)render_to_string(root);
  }

  const auto later = find_prompt_pixel(root);
  REQUIRE(later.has_value());
  REQUIRE(first->foreground_color != later->foreground_color);
  REQUIRE(first->character == later->character);
}

TEST_CASE("Chaos Attract selection moves through party rows",
          "[widgets][root][presentation]") {
  fl::GrandCentral attract_gc{8, 5, 5};
  auto surface = ftxui::Make<fl::widgets::AllAccountBattleScreen>(
      attract_gc.reg(), attract_gc.accounts());
  auto *all_accounts =
      dynamic_cast<fl::widgets::AllAccountBattleScreen *>(surface.get());
  fl::widgets::ChaosAttractRoot root{std::move(surface)};

  REQUIRE(render_to_string(root).find("A1 P1") != std::string::npos);
  REQUIRE(all_accounts->selected_account_index() == 0);
  REQUIRE(all_accounts->selected_party_index() == 0);
  REQUIRE(root.OnEvent(ftxui::Event::ArrowDown));
  REQUIRE(render_to_string(root).find("A1 P2") != std::string::npos);
  REQUIRE(all_accounts->selected_account_index() == 0);
  REQUIRE(all_accounts->selected_party_index() == 1);
  REQUIRE(root.OnEvent(ftxui::Event::ArrowUp));
  REQUIRE(all_accounts->selected_account_index() == 0);
  REQUIRE(all_accounts->selected_party_index() == 0);
  REQUIRE(root.OnEvent(ftxui::Event::Character("]")));
  REQUIRE(all_accounts->selected_account_index() == 0);
  REQUIRE(all_accounts->selected_party_index() == 1);
  REQUIRE(root.OnEvent(ftxui::Event::Character("[")));
  REQUIRE(all_accounts->selected_account_index() == 0);
  REQUIRE(all_accounts->selected_party_index() == 0);
}

TEST_CASE("Chaos Attract toggles keybind help",
          "[widgets][root][presentation]") {
  fl::GrandCentral attract_gc{8, 5, 5};
  fl::widgets::ChaosAttractRoot root{make_all_account_surface(attract_gc)};

  REQUIRE_FALSE(root.help_open());
  REQUIRE(render_to_string(root).find("Chaos Attract") != std::string::npos);
  REQUIRE(render_to_string(root).find("close this help") == std::string::npos);

  REQUIRE(root.OnEvent(ftxui::Event::Character("h")));
  REQUIRE(root.help_open());
  REQUIRE(render_to_string(root).find("close this help") != std::string::npos);
  REQUIRE(render_to_string(root).find("[ / ]") != std::string::npos);

  REQUIRE(root.OnEvent(ftxui::Event::Character("h")));
  REQUIRE_FALSE(root.help_open());
}

TEST_CASE("Enter requests GrandCentral switchout to normal gameplay",
          "[widgets][root][presentation]") {
  fl::widgets::ChaosAttractRoot root;

  REQUIRE(root.OnEvent(ftxui::Event::Return));
  REQUIRE(root.begin_requested());

  fl::GrandCentral gc{1, 1, 1};
  auto gameplay_root = make_gameplay_root(gc);
  const auto rendered = render_to_string(gameplay_root);
  REQUIRE(rendered.find("ENTER TO BEGIN") == std::string::npos);
  REQUIRE(rendered.find("help") != std::string::npos);
}
