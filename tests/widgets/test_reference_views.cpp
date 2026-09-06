#include <catch2/catch_test_macros.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/screen/screen.hpp>

#include "fl/ecs/components/monster_identity.hpp"
#include "fl/grand_central.hpp"
#include "fl/widgets/bestiary_view.hpp"
#include "fl/widgets/libram_view.hpp"
#include "fl/widgets/root_component.hpp"

namespace {
std::string render(ftxui::ComponentBase &view) {
  auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(100),
                                      ftxui::Dimension::Fixed(24));
  ftxui::Render(screen, view.Render());
  return screen.ToString();
}
} // namespace

TEST_CASE("Libram hides unwitnessed names and descriptions",
          "[discovery][widgets]") {
  fl::primitives::DiscoveryJournal journal;
  fl::widgets::LibramView view{journal};
  REQUIRE(render(view).find("No skills witnessed") != std::string::npos);
  journal.witness(fl::skills::SkillId::Thump);
  const auto text = render(view);
  REQUIRE(text.find("Thump") != std::string::npos);
  REQUIRE(text.find("blunt attack") != std::string::npos);
  REQUIRE(text.find("Eviscerate") == std::string::npos);
}

TEST_CASE("Bestiary links only observed monster skills",
          "[discovery][widgets]") {
  using fl::monster::MonsterKind;
  using fl::skills::SkillId;
  fl::primitives::DiscoveryJournal journal;
  journal.encounter(MonsterKind::ScaredyCat);
  journal.witness(
      SkillId::Flee); // Known globally, still hidden for this monster.
  journal.witness(SkillId::Thump, MonsterKind::ScaredyCat);
  std::optional<SkillId> opened;
  fl::widgets::BestiaryView view{journal, [&](auto kind, auto skill) {
                                   REQUIRE(kind == MonsterKind::ScaredyCat);
                                   opened = skill;
                                 }};
  const auto text = render(view);
  REQUIRE(text.find("Scaredy Cat") != std::string::npos);
  REQUIRE(text.find("--") != std::string::npos);
  REQUIRE(text.find("Flee") == std::string::npos);
  view.OnEvent(ftxui::Event::Tab);
  view.OnEvent(ftxui::Event::Return);
  REQUIRE_FALSE(opened);
  view.OnEvent(ftxui::Event::ArrowDown);
  view.OnEvent(ftxui::Event::Return);
  REQUIRE(opened == SkillId::Thump);
}

TEST_CASE("Shell follows bestiary links and returns to the monster",
          "[discovery][widgets]") {
  fl::GrandCentral game{1, 1, 1};
  auto &bus = game.accounts()[0].party(0).party_bus();
  bus.emit(fl::events::PartyEvent{
      fl::events::MonsterEncountered{fl::monster::MonsterKind::ScaredyCat}});
  const auto cat = game.reg().create();
  game.reg().emplace<fl::ecs::components::MonsterIdentity>(
      cat, fl::monster::MonsterKind::ScaredyCat);
  bus.emit(fl::events::PartyEvent{
      fl::events::SkillWitnessed{cat, fl::skills::SkillId::Thump}});
  fl::widgets::RootComponent root{game.account_context(0), game.accounts(),
                                  game.game_log(), game.world_clock(),
                                  &game.discoveries()};
  root.OnEvent(ftxui::Event::Character("b"));
  REQUIRE(render(root).find("Bestiary") != std::string::npos);
  root.OnEvent(ftxui::Event::Tab);
  root.OnEvent(ftxui::Event::ArrowDown);
  root.OnEvent(ftxui::Event::Return);
  REQUIRE(render(root).find("Libram") != std::string::npos);
  root.OnEvent(ftxui::Event::Escape);
  REQUIRE(render(root).find("Bestiary") != std::string::npos);
  REQUIRE(render(root).find("Scaredy Cat") != std::string::npos);
}
