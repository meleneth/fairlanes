#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include "fl/ecs/components/monster_identity.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/events/raid_bus.hpp"
#include "fl/grand_central.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "fl/widgets/account_battle_view.hpp"
#include "fl/widgets/all_account_battle_screen.hpp"
#include "fl/widgets/battle_render_budget.hpp"
#include "fl/widgets/chaos_attract_root.hpp"
#include "fl/widgets/root_chrome.hpp"
#include "fl/widgets/root_component.hpp"
#include "fl/widgets/story_start_picker.hpp"
#include "fl/widgets/important_notice_overlay.hpp"

namespace {

constexpr int kRenderWidth = 120;
constexpr int kRenderHeight = 40;

std::string screen_text(const ftxui::Screen &screen) {
  std::string result;
  for (int y = 0; y < screen.dimy(); ++y) {
    if (y != 0) {
      result += '\n';
    }
    for (int x = 0; x < screen.dimx(); ++x) {
      result += screen.PixelAt(x, y).character;
    }
  }
  return result;
}

std::string render_to_string(fl::widgets::RootComponent &root) {
  auto element = root.Render() | ftxui::size(ftxui::WIDTH, ftxui::EQUAL,
                                             kRenderWidth) |
                 ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, kRenderHeight);
  auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(kRenderWidth),
                                      ftxui::Dimension::Fixed(kRenderHeight));
  ftxui::Render(screen, element);
  return screen_text(screen);
}

std::string render_to_string(fl::widgets::ChaosAttractRoot &root) {
  auto element = root.Render() | ftxui::size(ftxui::WIDTH, ftxui::EQUAL,
                                             kRenderWidth) |
                 ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, kRenderHeight);
  auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(kRenderWidth),
                                      ftxui::Dimension::Fixed(kRenderHeight));
  ftxui::Render(screen, element);
  return screen_text(screen);
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
  return screen_text(screen);
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
  fl::GrandCentral gc{2, 3, 1};
  auto root = make_gameplay_root(gc);

  const auto rendered = render_to_string(root);
  REQUIRE(rendered.find("=/-") != std::string::npos);
  REQUIRE(rendered.find("overdrive") != std::string::npos);
  REQUIRE(rendered.find("help") != std::string::npos);

  const auto initial_overdrive = gc.world_clock().beat_rate_multiplier();
  REQUIRE(root.OnEvent(ftxui::Event::Character("=")));
  REQUIRE(gc.world_clock().beat_rate_multiplier() > initial_overdrive);
  REQUIRE(root.OnEvent(ftxui::Event::Character("-")));
  REQUIRE(gc.world_clock().beat_rate_multiplier() == initial_overdrive);

  REQUIRE(root.OnEvent(ftxui::Event::Character("'")));
  REQUIRE(render_to_string(root).find("2/2") != std::string::npos);
  REQUIRE(root.OnEvent(ftxui::Event::Character("]")));
  REQUIRE(render_to_string(root).find("2/3") != std::string::npos);
  REQUIRE(root.OnEvent(ftxui::Event::Character(";")));
  REQUIRE(render_to_string(root).find("1/2") != std::string::npos);
}

TEST_CASE("Important notices rise from bottom right and wait for acknowledgement",
          "[widgets][root][notice]") {
  using Overlay = fl::widgets::ImportantNoticeOverlay;
  const Overlay::Clock::time_point start{};
  Overlay notices;
  notices.enqueue({"BIOME UNLOCKED", "Origin Plains Tower", 20}, start);

  auto render_notice = [&](Overlay::Clock::time_point now) {
    auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                        ftxui::Dimension::Fixed(24));
    ftxui::Render(screen, notices.render(24, now) |
                              ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 80) |
                              ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 24));
    return screen_text(screen);
  };

  const auto bottom = render_notice(start);
  const auto top = render_notice(start + std::chrono::milliseconds{900});
  REQUIRE(find_line_containing(bottom, "BIOME UNLOCKED").value() > 12);
  REQUIRE(find_line_containing(top, "BIOME UNLOCKED").value() < 5);
  REQUIRE(bottom.find("Origin Plains Tower") != std::string::npos);
  REQUIRE(top.find("Origin Plains Tower") != std::string::npos);
  notices.dismiss(start + std::chrono::milliseconds{900});
  REQUIRE_FALSE(notices.active());
}

TEST_CASE("Autoplay acknowledges each achievement-style notice after sixty seconds",
          "[widgets][root][notice]") {
  using Overlay = fl::widgets::ImportantNoticeOverlay;
  const Overlay::Clock::time_point start{};
  Overlay notices;
  notices.set_auto_acknowledge(true);
  notices.enqueue({"ACHIEVEMENT UNLOCKED", "First", 14}, start);
  notices.enqueue({"OBJECTIVE REVEALED", "Second", 27}, start);
  notices.tick(start + std::chrono::seconds{59});
  REQUIRE(notices.active());
  notices.tick(start + std::chrono::seconds{60});
  REQUIRE(notices.active());
  auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                      ftxui::Dimension::Fixed(24));
  ftxui::Render(screen, notices.render(24, start + std::chrono::seconds{60}));
  REQUIRE(screen_text(screen).find("Second") != std::string::npos);
  notices.tick(start + std::chrono::seconds{119});
  REQUIRE(notices.active());
  notices.tick(start + std::chrono::seconds{120});
  REQUIRE_FALSE(notices.active());

  Overlay manual;
  manual.enqueue({"ACHIEVEMENT UNLOCKED", "Keep reading", 14}, start);
  manual.tick(start + std::chrono::minutes{2});
  REQUIRE(manual.active());
}

TEST_CASE("Root queues milestone events and consumes the acknowledging key",
          "[widgets][root][notice]") {
  fl::GrandCentral gc{1, 1, 1};
  auto root = fl::widgets::RootComponent{
      gc.account_context(0), gc.accounts(), gc.game_log(), gc.world_clock(),
      nullptr, &gc.raid_milestones().bus()};
  const auto baseline = render_to_string(root);
  auto &bus = gc.accounts().front().parties().front().party_bus();
  bus.emit(fl::events::PartyEvent{
      fl::events::BiomeUnlocked{"origin_plains_tower"}});
  bus.emit(fl::events::PartyEvent{
      fl::events::CycleUnlocked{fl::primitives::Cycle::Resonance}});
  gc.raid_milestones().bus().emit(fl::events::AchievementUnlocked{
      fl::events::AchievementId::RaidMutualDestruction,
      gc.accounts().front().account_id(), 1});

  REQUIRE(render_to_string(root).find("BIOME UNLOCKED") != std::string::npos);
  REQUIRE(root.OnEvent(ftxui::Event::Character(" ")));
  REQUIRE(render_to_string(root).find("CYCLE UNLOCKED") != std::string::npos);
  REQUIRE(root.OnEvent(ftxui::Event::Return));
  REQUIRE(render_to_string(root).find("ACHIEVEMENT UNLOCKED") !=
          std::string::npos);
  REQUIRE(root.OnEvent(ftxui::Event::Character(" ")));
  REQUIRE(render_to_string(root) == baseline);
  REQUIRE(root.OnEvent(ftxui::Event::Character(" ")));
  REQUIRE(render_to_string(root) != baseline);
}

TEST_CASE("Story keys lead from the Visitor defeat to pub recruitment",
          "[widgets][root][story]") {
  fl::GrandCentral game{1, 1, 0, true, true, 42, true};
  auto root = fl::widgets::RootComponent{
      game.account_context(0), game.accounts(), game.game_log(),
      game.world_clock(), &game.discoveries(),
      &game.raid_milestones().bus(), game.objectives()};
  game.innervate_event_system();
  game.advance_beat();
  REQUIRE(render_to_string(root).find("OBJECTIVE REVEALED") != std::string::npos);
  REQUIRE(root.OnEvent(ftxui::Event::Return));
  REQUIRE(root.OnEvent(ftxui::Event::Character("o")));
  REQUIRE(render_to_string(root).find("Recruit a hero") != std::string::npos);
  REQUIRE(render_to_string(root).find("Press v to visit the village") !=
          std::string::npos);
  REQUIRE(root.OnEvent(ftxui::Event::Character("v")));
  REQUIRE(render_to_string(root).find("Village") != std::string::npos);
  REQUIRE(root.OnEvent(ftxui::Event::Return));
  REQUIRE(render_to_string(root).find("Village Pub") != std::string::npos);
  REQUIRE(root.OnEvent(ftxui::Event::Return));
  REQUIRE(render_to_string(root).find("Then fight beside me") != std::string::npos);
  REQUIRE(root.OnEvent(ftxui::Event::Return));
  REQUIRE(game.accounts().front().parties().front().members().size() == 1);
  REQUIRE(game.objectives()->completed("recruit_first_hero"));
}

TEST_CASE("Manual village navigation is scoped to a selected party",
          "[widgets][root][story][tower]") {
  fl::GrandCentral game{1, 1, 1, true, true, 42, true};
  auto root = fl::widgets::RootComponent{
      game.account_context(0), game.accounts(), game.game_log(),
      game.world_clock(), nullptr, nullptr, game.objectives()};

  // The account battle view is the multi-party surface and cannot choose a
  // route for whichever party happens to be selected in the command model.
  REQUIRE(root.OnEvent(ftxui::Event::Character(" ")));
  REQUIRE(root.active_screen_id() == "account_battle");
  REQUIRE(root.OnEvent(ftxui::Event::Character("v")));
  REQUIRE(root.active_screen_id() == "account_battle");

  REQUIRE(root.OnEvent(ftxui::Event::Character(" ")));
  REQUIRE(root.active_screen_id() == "party_battle");
  REQUIRE(root.OnEvent(ftxui::Event::Character("v")));
  auto rendered = render_to_string(root);
  REQUIRE(rendered.find("Visit the pub") != std::string::npos);
  REQUIRE(rendered.find("Visit the outfitter's shop") != std::string::npos);
  REQUIRE(rendered.find("Choose a biome expedition") != std::string::npos);

  REQUIRE(root.OnEvent(ftxui::Event::ArrowDown));
  REQUIRE(root.OnEvent(ftxui::Event::ArrowDown));
  REQUIRE(root.OnEvent(ftxui::Event::Return));
  rendered = render_to_string(root);
  REQUIRE(rendered.find("Choose a Biome") != std::string::npos);
  REQUIRE(rendered.find("Origin Woodland  Cleared 0  Next 1") !=
          std::string::npos);

  REQUIRE(root.OnEvent(ftxui::Event::Return));
  rendered = render_to_string(root);
  REQUIRE(rendered.find("Choose a Floor") != std::string::npos);
  REQUIRE(rendered.find("Floor 1 [next target]") != std::string::npos);
  REQUIRE(root.OnEvent(ftxui::Event::Return));
  const auto &party = game.accounts().front().parties().front();
  REQUIRE(party.tower_route().has_value());
  REQUIRE(party.tower_route()->biome_id == "origin_woodland_tower");
  REQUIRE(party.tower_route()->floor == 1);
}

TEST_CASE("Sandbox console switch warns that story cannot be recovered",
          "[widgets][root][story]") {
  fl::GrandCentral game{1, 1, 0, true, true, 42, true};
  auto root = fl::widgets::RootComponent{
      game.account_context(0), game.accounts(), game.game_log(),
      game.world_clock(), nullptr, nullptr, game.objectives()};
  REQUIRE(root.OnEvent(ftxui::Event::Character("`")));
  for (char character : std::string{"sandbox"})
    REQUIRE(root.OnEvent(ftxui::Event::Character(std::string(1, character))));
  REQUIRE(root.OnEvent(ftxui::Event::Return));
  REQUIRE(render_to_string(root).find("cannot be recovered") != std::string::npos);
  REQUIRE_FALSE(root.sandbox_requested());
  REQUIRE(root.OnEvent(ftxui::Event::Character("n")));
  REQUIRE_FALSE(root.sandbox_requested());
  // The same command can be issued again after cancelling.
  for (char character : std::string{"sandbox"})
    REQUIRE(root.OnEvent(ftxui::Event::Character(std::string(1, character))));
  REQUIRE(root.OnEvent(ftxui::Event::Return));
  REQUIRE(root.OnEvent(ftxui::Event::Return));
  REQUIRE(root.sandbox_requested());
}

TEST_CASE("Attract exit picker offers manual story and autoplay with a toggle hint",
          "[widgets][story][start]") {
  bool finished = false;
  fl::widgets::StoryStartPicker picker{[&] { finished = true; }};
  auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(100),
                                      ftxui::Dimension::Fixed(24));
  ftxui::Render(screen, picker.Render());
  const auto displayed = screen_text(screen);
  REQUIRE(displayed.find("Manual story") != std::string::npos);
  REQUIRE(displayed.find("Autoplay") != std::string::npos);
  REQUIRE(displayed.find("toggle autoplay at any time") != std::string::npos);
  REQUIRE(picker.OnEvent(ftxui::Event::ArrowDown));
  REQUIRE(picker.OnEvent(ftxui::Event::Return));
  REQUIRE_FALSE(finished);
  REQUIRE(picker.mode() == fl::widgets::StoryStartMode::Cancel);
  REQUIRE(picker.OnEvent(ftxui::Event::Escape));
  REQUIRE_FALSE(finished);
  REQUIRE(picker.OnEvent(ftxui::Event::Return));
  REQUIRE_FALSE(finished);
  REQUIRE(picker.OnEvent(ftxui::Event::Return));
  REQUIRE(finished);
  REQUIRE(picker.mode() == fl::widgets::StoryStartMode::Autoplay);
}

TEST_CASE("Story autoplay requires confirmation when enabled from the root",
          "[widgets][root][story]") {
  fl::GrandCentral game{1, 1, 0, true, true, 42, true};
  auto root = fl::widgets::RootComponent{
      game.account_context(0), game.accounts(), game.game_log(),
      game.world_clock(), nullptr, nullptr, game.objectives()};
  REQUIRE_FALSE(game.objectives()->autoplay_enabled());
  REQUIRE(root.OnEvent(ftxui::Event::Character("a")));
  REQUIRE(render_to_string(root).find("Enable story autoplay") != std::string::npos);
  REQUIRE_FALSE(game.objectives()->autoplay_enabled());
  REQUIRE(root.OnEvent(ftxui::Event::Character("n")));
  REQUIRE_FALSE(game.objectives()->autoplay_enabled());
  REQUIRE(root.OnEvent(ftxui::Event::Character("a")));
  REQUIRE(root.OnEvent(ftxui::Event::Return));
  REQUIRE(game.objectives()->autoplay_enabled());
  REQUIRE(root.OnEvent(ftxui::Event::Character("a")));
  REQUIRE_FALSE(game.objectives()->autoplay_enabled());
}

TEST_CASE("Account battle combatants keep fixed twenty-percent widths",
          "[widgets][account-battle][layout]") {
  constexpr int width = 140;
  constexpr int height = 40;
  constexpr int column_width = width / 5;
  fl::GrandCentral gc{1, 1, 5};
  auto account_ctx = gc.account_context(0);
  auto &members = account_ctx.account_data().party(0).members();
  account_ctx.reg().get<fl::ecs::components::Stats>(members.front().member_id())
      .name_ = std::string(60, 'X');
  fl::widgets::AccountBattleView view{account_ctx};

  fl::widgets::force_battle_render_target(
      fl::widgets::BattleLayoutProfile::Standard);
  auto element = view.Render() |
                 ftxui::size(ftxui::WIDTH, ftxui::EQUAL, width) |
                 ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, height);
  auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(width),
                                      ftxui::Dimension::Fixed(height));
  ftxui::Render(screen, element);
  fl::widgets::clear_forced_battle_render_target();

  int hp_row = -1;
  for (int y = 0; y < height && hp_row < 0; ++y) {
    int hp_labels = 0;
    for (int x = 0; x + 1 < width; ++x) {
      if (screen.PixelAt(x, y).character == "H" &&
          screen.PixelAt(x + 1, y).character == "P") {
        ++hp_labels;
      }
    }
    if (hp_labels == 5) {
      hp_row = y;
    }
  }

  REQUIRE(hp_row >= 0);
  for (int column = 0; column < 5; ++column) {
    REQUIRE(screen.PixelAt(column * column_width, hp_row).character == "│");
    REQUIRE(screen.PixelAt((column + 1) * column_width - 1, hp_row).character ==
            "│");
  }
}

TEST_CASE("Account battle rows stay fixed across encounter transitions",
          "[widgets][account-battle][layout]") {
  constexpr int width = 140;
  constexpr int height = 40;
  fl::GrandCentral gc{1, 2, 5};
  auto account_ctx = gc.account_context(0);
  fl::widgets::AccountBattleView view{account_ctx};
  const auto anchor = account_ctx.reg()
                          .get<fl::ecs::components::Stats>(
                              account_ctx.account_data()
                                  .party(1)
                                  .members()
                                  .front()
                                  .member_id())
                          .name_;

  auto render_view = [&] {
    auto element = view.Render() |
                   ftxui::size(ftxui::WIDTH, ftxui::EQUAL, width) |
                   ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, height);
    auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(width),
                                        ftxui::Dimension::Fixed(height));
    ftxui::Render(screen, element);
    return screen_text(screen);
  };

  fl::widgets::force_battle_render_target(
      fl::widgets::BattleLayoutProfile::Standard);
  const auto before = find_line_containing(render_view(), anchor);
  account_ctx.account_data().party(0).create_encounter();
  const auto during = find_line_containing(render_view(), anchor);
  fl::widgets::clear_forced_battle_render_target();

  REQUIRE(before.has_value());
  REQUIRE(during.has_value());
  REQUIRE(*before == *during);
}

TEST_CASE("Account battle reveals ATB when the roster has vertical space",
          "[widgets][account-battle][layout][atb]") {
  using fl::widgets::BattleLayoutProfile;
  const auto scenario = GENERATE(
      std::tuple{BattleLayoutProfile::Standard, 5, false},
      std::tuple{BattleLayoutProfile::Wide, 5, true},
      std::tuple{BattleLayoutProfile::Showcase, 5, true},
      // Three parties occupy exactly two-thirds below Compact's moon chrome.
      std::tuple{BattleLayoutProfile::Compact, 3, false},
      std::tuple{BattleLayoutProfile::Compact, 2, true});
  const auto [profile, party_count, shows_atb] = scenario;
  const auto target = fl::widgets::battle_render_target_spec(profile);
  fl::GrandCentral gc{1, static_cast<std::uint8_t>(party_count), 5};
  auto ctx = gc.account_context(0);
  int hero = 0;
  for (auto &party : ctx.account_data().parties()) {
    for (auto &member : party.members()) {
      gc.reg().get<fl::ecs::components::Stats>(member.member_id()).name_ =
          "Hero" + std::to_string(++hero) + "!";
    }
    fl::primitives::EncounterBuilder{party.party_ctx()}.thump_it_out();
  }
  fl::widgets::AccountBattleView view{ctx};
  fl::widgets::force_battle_render_target(profile);
  auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(target.width),
                                      ftxui::Dimension::Fixed(target.height));
  ftxui::Render(screen, fl::widgets::render_root_chrome(
                            ctx.account_data().calendar(), view.Render()));
  fl::widgets::clear_forced_battle_render_target();

  int atb_bars = 0;
  for (int y = 0; y < screen.dimy(); ++y) {
    for (int x = 0; x + 2 < screen.dimx(); ++x) {
      if (screen.PixelAt(x, y).character == "A" &&
          screen.PixelAt(x + 1, y).character == "T" &&
          screen.PixelAt(x + 2, y).character == "B")
        ++atb_bars;
    }
  }
  REQUIRE(atb_bars == (shows_atb ? party_count * 10 : 0));
  const auto rendered = screen_text(screen);
  for (int i = 1; i <= party_count * 5; ++i)
    REQUIRE(rendered.find("Hero" + std::to_string(i) + "!") !=
            std::string::npos);
}

TEST_CASE("Account battle log panes keep fixed fractional widths",
          "[widgets][account-battle][layout][log]") {
  constexpr int width = 140;
  constexpr int height = 40;
  constexpr std::array pane_starts{0, 24, 48, 71, 94, 117};
  constexpr std::array pane_markers{'A', 'B', 'C', 'D', 'E', 'F'};
  fl::GrandCentral gc{1, 5, 5};
  auto account_ctx = gc.account_context(0);
  auto &account = account_ctx.account_data();
  account.log().clear();
  account.log().append_plain(std::string(100, pane_markers.front()));
  for (std::size_t i = 0; i < account.parties().size(); ++i) {
    account.parties()[i].log().clear();
    account.parties()[i].log().append_plain(
        std::string(i + 1, pane_markers[i + 1]));
  }
  fl::widgets::AccountBattleView view{account_ctx};

  fl::widgets::force_battle_render_target(
      fl::widgets::BattleLayoutProfile::Standard);
  auto element = view.Render() |
                 ftxui::size(ftxui::WIDTH, ftxui::EQUAL, width) |
                 ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, height);
  auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(width),
                                      ftxui::Dimension::Fixed(height));
  ftxui::Render(screen, element);
  fl::widgets::clear_forced_battle_render_target();

  int marker_row = -1;
  for (int y = 0; y < height && marker_row < 0; ++y) {
    bool matches = true;
    for (std::size_t pane = 0; pane < pane_starts.size(); ++pane) {
      if (screen.PixelAt(pane_starts[pane], y).character !=
          std::string(1, pane_markers[pane])) {
        matches = false;
        break;
      }
    }
    if (matches) {
      marker_row = y;
    }
  }

  REQUIRE(marker_row >= 0);
}

TEST_CASE("Normal gameplay collapses and expands the metrics overlay",
          "[widgets][root][presentation]") {
  fl::GrandCentral gc{1, 1, 1};
  auto root = make_gameplay_root(gc);

  REQUIRE(render_to_string(root).find("FPS ") != std::string::npos);
  const auto collapse = GENERATE(">", ".");
  const auto expand = GENERATE("<", ",");
  REQUIRE(root.OnEvent(ftxui::Event::Character(collapse)));
  const auto collapsed = render_to_string(root);
  REQUIRE(collapsed.find("FPS ") == std::string::npos);
  REQUIRE(collapsed.find("┌") != std::string::npos);
  REQUIRE(collapsed.find(",") != std::string::npos);
  REQUIRE(collapsed.find("└") != std::string::npos);

  REQUIRE(root.OnEvent(ftxui::Event::Character(expand)));
  REQUIRE(render_to_string(root).find("FPS ") != std::string::npos);
}

TEST_CASE("Party screens dispatch return-to-town requests with r",
          "[widgets][root][party][town]") {
  fl::GrandCentral gc{1, 1, 1};
  auto root = make_gameplay_root(gc);
  auto &party = gc.accounts().front().party(0);
  int request_count = 0;
  auto request_sub =
      party.party_bus().subscribe<fl::events::PartyReturnToTownRequested>(
          [&](const auto &) { ++request_count; });

  SECTION("party battle") {}

  SECTION("player and inventory") { root.show_party(0, 0); }

  REQUIRE(root.OnEvent(ftxui::Event::Character("r")));
  REQUIRE(request_count == 1);
}

TEST_CASE("Account commands preserve the selected gameplay screen",
          "[widgets][root][commands]") {
  fl::GrandCentral gc{2, 1, 1};
  auto root = make_gameplay_root(gc);

  REQUIRE(root.OnEvent(ftxui::Event::Character("/")));
  REQUIRE(render_to_string(root).find("Inventory") != std::string::npos);
  REQUIRE(root.OnEvent(ftxui::Event::Character("`")));
  REQUIRE(root.OnEvent(ftxui::Event::Character("account 1")));
  REQUIRE(root.OnEvent(ftxui::Event::Return));
  REQUIRE(root.OnEvent(ftxui::Event::Escape));

  const auto rendered = render_to_string(root);
  REQUIRE(rendered.find("Inventory") != std::string::npos);
  REQUIRE(rendered.find("Ashen Vanguard") != std::string::npos);
}

TEST_CASE("Raid spawn console command summons a new Visitor",
          "[widgets][root][commands][raid]") {
  fl::GrandCentral gc{1, 1, 1};
  auto root = make_gameplay_root(gc);
  auto &account = gc.accounts().front();
  bool announced = false;
  auto started_sub = account.raid_bus().subscribe<fl::events::RaidStarted>(
      [&](const auto &) { announced = true; });

  REQUIRE_FALSE(account.in_raid());
  REQUIRE(root.OnEvent(ftxui::Event::Character("`")));
  REQUIRE(root.OnEvent(ftxui::Event::Character("raid spawn")));
  REQUIRE(root.OnEvent(ftxui::Event::Return));
  REQUIRE(account.in_raid());
  REQUIRE(announced);
  REQUIRE(account.raid()->encounter().attackers().members().size() == 1);
  const auto visitor = account.raid()->encounter().attackers().members().front();
  REQUIRE(gc.reg().get<fl::ecs::components::MonsterIdentity>(visitor).kind ==
          fl::monster::MonsterKind::VisitorHerald);
}

TEST_CASE("Chaos Attract hides the normal key manifest",
          "[widgets][root][presentation]") {
  fl::GrandCentral attract_gc{8, 5, 5};
  fl::widgets::ChaosAttractRoot root{make_all_account_surface(attract_gc)};

  const auto rendered = render_to_string(root, attract_gc.world_clock());
  REQUIRE(rendered.find("=/-") == std::string::npos);
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
