#include <catch2/catch_test_macros.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include "fl/ecs/components/casting_skill.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/grand_central.hpp"
#include "fl/lospec500.hpp"
#include "fl/skills/skill.hpp"
#include "fl/widgets/player_details_pane.hpp"

TEST_CASE("Player details selects and drops equipped skills",
          "[widgets][player][skills]") {
  fl::GrandCentral gc{1, 1, 1};
  auto party_ctx = gc.account_context(0).party_context(0);
  auto &party = party_ctx.party_data();
  const auto member = party.members().front().member_id();
  auto &party_member =
      party_ctx.reg().get<fl::ecs::components::PartyMember>(member);
  auto &closet = party_member.closet();
  const auto skill = fl::skills::SkillKey{fl::skills::SkillId::Thump};
  closet.skill_slots[1] = skill;
  REQUIRE(party_member.member_data().grimoire().learn(skill));

  fl::widgets::PlayerDetailsPane pane{party_ctx.reg(), party};
  pane.set_focused(true);
  REQUIRE(pane.OnEvent(ftxui::Event::ArrowDown));
  REQUIRE(pane.OnEvent(ftxui::Event::Delete));

  REQUIRE(closet.skill_slots[0] == fl::skills::SkillId::Observe);
  REQUIRE_FALSE(closet.skill_slots[1].has_value());
  REQUIRE_FALSE(party_member.member_data().grimoire().knows(skill));
}

TEST_CASE("Player details highlights the skill currently being cast",
          "[widgets][player][skills][casting]") {
  fl::GrandCentral gc{1, 1, 1};
  auto party_ctx = gc.account_context(0).party_context(0);
  auto &party = party_ctx.party_data();
  const auto member = party.members().front().member_id();
  auto &closet =
      party_ctx.reg().get<fl::ecs::components::PartyMember>(member).closet();
  closet.skill_slots[1] = fl::skills::SkillId::Thump;
  party_ctx.reg().emplace<fl::ecs::components::CastingSkill>(
      member, fl::skills::SkillId::Thump);

  fl::widgets::PlayerDetailsPane pane{party_ctx.reg(), party};
  auto element = pane.Render() | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 50) |
                 ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 30);
  auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(50),
                                      ftxui::Dimension::Fixed(30));
  ftxui::Render(screen, element);

  bool found_highlight = false;
  for (int y = 0; y < screen.dimy(); ++y) {
    for (int x = 0; x < screen.dimx(); ++x) {
      const auto &pixel = screen.PixelAt(x, y);
      if (pixel.character == "T" &&
          pixel.background_color == fl::lospec500::color_at(14) &&
          pixel.foreground_color == fl::lospec500::color_at(0) && pixel.bold) {
        found_highlight = true;
      }
    }
  }
  REQUIRE(found_highlight);
}
