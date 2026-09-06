#include "fl/ecs/components/stats.hpp"
#include "fl/grand_central.hpp"
#include "fl/widgets/raid_view.hpp"
#include "fl/widgets/root_component.hpp"
#include <catch2/catch_test_macros.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

TEST_CASE("Raid layout exposes all twenty-five members at terminal sizes",
          "[raid][widgets]") {
  fl::GrandCentral gc{1, 5, 5};
  gc.innervate_event_system();
  auto ctx = gc.account_context(0);
  int index = 0;
  for (const auto &party : ctx.account_data().parties())
    for (const auto &member : party.members())
      gc.reg().get<fl::ecs::components::Stats>(member.member_id()).name_ =
          "Hero" + std::to_string(++index) + "!";
  fl::widgets::RaidView view{ctx};
  for (auto [width, height] : {std::pair{80, 21}, std::pair{120, 37}}) {
    auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(width),
                                        ftxui::Dimension::Fixed(height));
    ftxui::Render(screen, view.Render());
    REQUIRE(screen.PixelAt(width - 1, height / 3 - 1).background_color ==
            ftxui::Color::RGB(12, 5, 25));
    REQUIRE(screen.PixelAt(width - 1, height / 3).background_color !=
            ftxui::Color::RGB(12, 5, 25));
    auto output = screen.ToString();
    REQUIRE(output.find("Visitor's Herald") != std::string::npos);
    REQUIRE(output.find("paused") != std::string::npos);
    for (int hero = 1; hero <= 25; ++hero)
      REQUIRE(output.find("Hero" + std::to_string(hero) + "!") !=
              std::string::npos);
  }
}

TEST_CASE(
    "Gameplay root automatically shows the raid and consumed-arrival countdown",
    "[raid][widgets][calendar]") {
  fl::GrandCentral gc{1, 5, 5};
  gc.innervate_event_system();
  auto ctx = gc.account_context(0);
  int index = 0;
  for (const auto &party : ctx.account_data().parties())
    for (const auto &member : party.members())
      gc.reg().get<fl::ecs::components::Stats>(member.member_id()).name_ =
          "Hero" + std::to_string(++index) + "!";
  fl::widgets::RootComponent root{ctx, gc.accounts(), gc.game_log(),
                                  gc.world_clock()};
  auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                      ftxui::Dimension::Fixed(24));
  ftxui::Render(screen, root.Render());
  auto output = screen.ToString();
  root.OnEvent(ftxui::Event::Return);
  REQUIRE(output.find("VISITOR RAID") != std::string::npos);
  REQUIRE(output.find("Next Visitor: 22h 3m 0s") != std::string::npos);
  REQUIRE(output.find("PAUSED") != std::string::npos);
  REQUIRE(output.find("Visitor today") == std::string::npos);
  for (int hero = 1; hero <= 25; ++hero)
    REQUIRE(output.find("Hero" + std::to_string(hero) + "!") !=
            std::string::npos);
  for (auto id : ctx.account_data().raid()->encounter().defenders())
    gc.reg().get<fl::ecs::components::Stats>(id).hp_ = 0;
  gc.beat_bus().emit(seerin::Beat{});
  screen.Clear();
  ftxui::Render(screen, root.Render());
  output = screen.ToString();
  REQUIRE(output.find("PAUSED") == std::string::npos);
  REQUIRE(output.find("Wipes: 1") != std::string::npos);
  auto &raid = *ctx.account_data().raid();
  REQUIRE_FALSE(raid.result_expired(*raid.resolved_at() + std::chrono::seconds(299)));
  REQUIRE(raid.result_expired(*raid.resolved_at() + std::chrono::minutes(5)));
  auto log_screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(180), ftxui::Dimension::Fixed(12));
  ftxui::Render(log_screen, gc.game_log().Render());
  REQUIRE(log_screen.ToString().find("Visitor raid: Account 1 - wipe") != std::string::npos);
  REQUIRE(output.find("Next Visitor: 22h 3m 0s") != std::string::npos);
  REQUIRE(root.OnEvent(ftxui::Event::Return));
  screen.Clear();
  ftxui::Render(screen, root.Render());
  REQUIRE(screen.ToString().find("VISITOR RAID") == std::string::npos);
}
