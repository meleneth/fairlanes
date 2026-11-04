#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>

#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>

#include "fl/events/party_bus.hpp"
#include "fl/generated/ftxer_composition_root.hpp"
#include "fl/grand_central.hpp"
#include "fl/widgets/ftxer_ui_adapter.hpp"
#include "fl/widgets/root_component.hpp"

namespace {

struct IntegratedRoot {
  ftxui::Component component;
  fl::generated::ftxer::CompositionRoot *composition{nullptr};
  fl::widgets::RootComponent *existing{nullptr};
};

IntegratedRoot make_root(fl::GrandCentral &game) {
  auto existing = ftxui::Make<fl::widgets::RootComponent>(
      game.account_context(0), game.accounts(), game.game_log(),
      game.world_clock(), &game.discoveries(), &game.raid_milestones().bus(),
      game.objectives());
  auto *existing_ptr =
      dynamic_cast<fl::widgets::RootComponent *>(existing.get());
  auto adapter =
      std::make_unique<fl::widgets::FtxerUiAdapter>(std::move(existing));
  auto component = ftxui::Make<fl::generated::ftxer::CompositionRoot>(
      std::move(adapter));
  return {component,
          dynamic_cast<fl::generated::ftxer::CompositionRoot *>(component.get()),
          existing_ptr};
}

std::string render(ftxui::Component component, int width = 120,
                   int height = 40) {
  auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(width),
                                      ftxui::Dimension::Fixed(height));
  ftxui::Render(screen, component->Render() |
                            ftxui::size(ftxui::WIDTH, ftxui::EQUAL, width) |
                            ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, height));
  return screen.ToString();
}

void type(fl::generated::ftxer::CompositionRoot &root,
          const std::string &text) {
  for (char value : text)
    REQUIRE(root.OnEvent(ftxui::Event::Character(std::string(1, value))));
}

} // namespace

TEST_CASE("FTXer root renders the real battle while simulation advances",
          "[ftxer][integration][battle]") {
  fl::GrandCentral game{1, 1, 1};
  game.innervate_event_system(false);
  auto root = make_root(game);

  const auto before = game.world_clock().elapsed_beats();
  game.advance_beat();
  REQUIRE(game.world_clock().elapsed_beats() == before + 1);

  const auto output = render(root.component);
  REQUIRE(output.find("Stormbreak Company") != std::string::npos);
  REQUIRE(output.find("Account log") != std::string::npos);
  REQUIRE(output.find("Party log") != std::string::npos);
}

TEST_CASE("FTXer root owns navigation and preserves the previous battle",
          "[ftxer][integration][navigation]") {
  fl::GrandCentral game{1, 1, 1};
  auto root = make_root(game);

  REQUIRE(root.composition->OnEvent(ftxui::Event::Character("b")));
  REQUIRE(root.composition->current_screen() ==
          fl::generated::ftxer::ScreenId::Bestiary);
  REQUIRE(root.composition->history_size() == 1);

  REQUIRE(root.composition->OnEvent(ftxui::Event::Escape));
  REQUIRE(root.composition->current_screen() ==
          fl::generated::ftxer::ScreenId::PartyBattle);
  REQUIRE(render(root.component).find("Stormbreak Company") !=
          std::string::npos);
}

TEST_CASE("FTXer console stays above modals and uses the real command system",
          "[ftxer][integration][console]") {
  fl::GrandCentral game{1, 1, 1};
  auto root = make_root(game);

  REQUIRE(root.composition->OnEvent(ftxui::Event::Character("q")));
  REQUIRE(render(root.component).find("Quit Fairlanes?") != std::string::npos);

  REQUIRE(root.composition->OnEvent(ftxui::Event::Character("`")));
  REQUIRE(root.composition->console_open());
  REQUIRE(root.existing->focus_owner() == "console");
  type(*root.composition, "help");
  REQUIRE(root.composition->OnEvent(ftxui::Event::Return));
  REQUIRE(root.existing->console_history_size() == 1);
  std::string expanded_console;
  for (int frame = 0; frame < 24; ++frame)
    expanded_console = render(root.component);
  REQUIRE(expanded_console.find("commands: screen") != std::string::npos);

  REQUIRE(root.composition->OnEvent(ftxui::Event::ArrowUp));
  REQUIRE(root.existing->console_input_line() == "help");
  REQUIRE(root.composition->OnEvent(ftxui::Event::Character("`")));
  REQUIRE_FALSE(root.composition->console_open());
  REQUIRE(root.existing->focus_owner() == "quit_confirmation");
  REQUIRE(render(root.component).find("Quit Fairlanes?") != std::string::npos);
}

TEST_CASE("FTXer global bindings retain selection and overdrive behavior",
          "[ftxer][integration][bindings]") {
  fl::GrandCentral game{2, 2, 1};
  auto root = make_root(game);
  const auto initial = game.world_clock().beat_rate_multiplier();

  REQUIRE(root.composition->OnEvent(ftxui::Event::Character("+")));
  REQUIRE(game.world_clock().beat_rate_multiplier() > initial);
  REQUIRE(root.composition->OnEvent(ftxui::Event::Character("-")));
  REQUIRE(game.world_clock().beat_rate_multiplier() == initial);

  REQUIRE(root.composition->OnEvent(ftxui::Event::Character("]")));
  REQUIRE(render(root.component).find("2/2") != std::string::npos);
  REQUIRE(root.composition->OnEvent(ftxui::Event::Character("'")));
  REQUIRE(render(root.component).find("2/2") != std::string::npos);
}

TEST_CASE("FTXer root remains usable across terminal sizes and subscriptions",
          "[ftxer][integration][resize][events]") {
  fl::GrandCentral game{1, 1, 1};
  auto root = make_root(game);
  auto &bus = game.accounts().front().parties().front().party_bus();

  REQUIRE(root.composition->OnEvent(ftxui::Event::Character("b")));
  bus.emit(fl::events::PartyEvent{
      fl::events::BiomeUnlocked{"origin_plains_tower"}});
  REQUIRE(render(root.component, 80, 24).find("BIOME UNLOCKED") !=
          std::string::npos);
  REQUIRE_FALSE(render(root.component, 160, 48).empty());
  REQUIRE(root.composition->OnEvent(ftxui::Event::Escape));
  REQUIRE(render(root.component, 80, 24).find("Stormbreak Company") !=
          std::string::npos);
}
