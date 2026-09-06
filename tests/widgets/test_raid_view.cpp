#include <catch2/catch_test_macros.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include "fl/grand_central.hpp"
#include "fl/widgets/raid_view.hpp"
#include "fl/widgets/root_component.hpp"
#include "fl/ecs/components/stats.hpp"

TEST_CASE("Raid layout exposes all twenty-five members at terminal sizes", "[raid][widgets]") {
  fl::GrandCentral gc{1, 5, 5};
  gc.innervate_event_system();
  auto ctx = gc.account_context(0);
  int index = 0;
  for (const auto &party : ctx.account_data().parties())
    for (const auto &member : party.members())
      gc.reg().get<fl::ecs::components::Stats>(member.member_id()).name_ = "Hero" + std::to_string(++index) + "!";
  fl::widgets::RaidView view{ctx};
  for (auto [width, height] : {std::pair{80, 21}, std::pair{120, 37}}) {
    auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(width), ftxui::Dimension::Fixed(height));
    ftxui::Render(screen, view.Render());
    auto output = screen.ToString();
    REQUIRE(output.find("Visitor's Herald") != std::string::npos);
    REQUIRE(output.find("paused") != std::string::npos);
    for (int hero = 1; hero <= 25; ++hero)
      REQUIRE(output.find("Hero" + std::to_string(hero) + "!") != std::string::npos);
  }
}
