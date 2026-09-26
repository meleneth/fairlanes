#include <optional>
#include <string>
#include <string_view>

#include <catch2/catch_test_macros.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include "fl/ecs/components/party_member.hpp"
#include "fl/grand_central.hpp"
#include "fl/loot/equipment_builder.hpp"
#include "fl/skills/skill_definition.hpp"
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

std::string render_party_view(fl::widgets::PartyView &view) {
  fl::widgets::force_battle_render_target(
      fl::widgets::BattleLayoutProfile::Wide);
  auto element = view.Render() | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 180) |
                 ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 48);
  auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(180),
                                      ftxui::Dimension::Fixed(48));
  ftxui::Render(screen, element);
  fl::widgets::clear_forced_battle_render_target();
  return screen.ToString();
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

TEST_CASE("Party player pane cycles members with left and right arrows",
          "[widgets][party][player]") {
  fl::GrandCentral gc{1, 1, 5};
  fl::widgets::PartyView view{gc.account_context(0), 0};

  REQUIRE_FALSE(view.OnEvent(ftxui::Event::ArrowRight));
  REQUIRE(render_party_view(view).find("Member 1/5") != std::string::npos);

  REQUIRE(view.OnEvent(ftxui::Event::Tab));
  REQUIRE(view.OnEvent(ftxui::Event::ArrowLeft));
  REQUIRE(render_party_view(view).find("Member 5/5") != std::string::npos);

  REQUIRE(view.OnEvent(ftxui::Event::ArrowRight));
  REQUIRE(render_party_view(view).find("Member 1/5") != std::string::npos);
  REQUIRE(view.OnEvent(ftxui::Event::ArrowRight));
  REQUIRE(render_party_view(view).find("Member 2/5") != std::string::npos);
}

TEST_CASE("Party player skill selection replaces inventory with skill details",
          "[widgets][party][player][skills]") {
  fl::GrandCentral gc{1, 1, 1};
  auto account_ctx = gc.account_context(0);
  auto &party = account_ctx.account_data().party(0);
  const auto member = party.members().front().member_id();
  auto &closet =
      account_ctx.reg().get<fl::ecs::components::PartyMember>(member).closet();
  closet.skill_slots[1] = fl::skills::SkillId::Thump;
  fl::widgets::PartyView view{account_ctx, 0};

  REQUIRE(view.OnEvent(ftxui::Event::Tab));
  REQUIRE(view.OnEvent(ftxui::Event::ArrowDown));
  const auto skill_details = render_party_view(view);
  REQUIRE(skill_details.find("Skill information") != std::string::npos);
  REQUIRE(skill_details.find("Thump") != std::string::npos);
  REQUIRE(skill_details.find("Rank 1") != std::string::npos);
  REQUIRE(skill_details.find("Deal 1-5 physical damage") != std::string::npos);

  for (int i = 0; i < fl::ecs::components::Closet::kSkillSlotCount - 1; ++i) {
    REQUIRE(view.OnEvent(ftxui::Event::ArrowDown));
  }
  const auto inventory = render_party_view(view);
  REQUIRE(inventory.find("Skill information") == std::string::npos);
  REQUIRE(inventory.find("Inventory") != std::string::npos);
}

TEST_CASE("Party player inventory equips filtered gear through party events",
          "[widgets][party][player][equipment]") {
  fl::GrandCentral gc{1, 1, 1};
  auto account_ctx = gc.account_context(0);
  auto &party = account_ctx.account_data().party(0);
  auto &reg = account_ctx.reg();
  party.replace_items({});

  const auto chest =
      fl::loot::EquipmentBuilder{
          .slot = fl::loot::EquipmentSlot::chest,
          .armor_kind = fl::loot::ArmorKind::cloth,
          .tier = fl::loot::Tier::fine,
          .name = "UI Chest",
      }
          .create(reg);
  const auto helm =
      fl::loot::EquipmentBuilder{
          .slot = fl::loot::EquipmentSlot::helm,
          .armor_kind = fl::loot::ArmorKind::cloth,
          .tier = fl::loot::Tier::fine,
          .name = "UI Helm",
      }
          .create(reg);
  party.add_item(chest);
  party.add_item(helm);

  const auto member = party.members().front().member_id();
  auto &closet = reg.get<fl::ecs::components::PartyMember>(member).closet();
  fl::widgets::PartyView view{account_ctx, 0};

  REQUIRE(view.OnEvent(ftxui::Event::Tab));
  for (int i = 0; i < fl::ecs::components::Closet::kSkillSlotCount; ++i) {
    REQUIRE(view.OnEvent(ftxui::Event::ArrowDown));
  }
  REQUIRE(view.OnEvent(ftxui::Event::Return));
  REQUIRE(render_party_view(view).find("Equipment slots") != std::string::npos);

  REQUIRE(view.OnEvent(ftxui::Event::ArrowDown));
  REQUIRE(view.OnEvent(ftxui::Event::ArrowDown));
  REQUIRE(view.OnEvent(ftxui::Event::Return));
  const auto filtered = render_party_view(view);
  REQUIRE(filtered.find("chest items") != std::string::npos);
  REQUIRE(filtered.find("UI Chest") != std::string::npos);
  REQUIRE(filtered.find("UI Helm") == std::string::npos);

  REQUIRE(view.OnEvent(ftxui::Event::Return));
  REQUIRE(closet.chest == chest);
  REQUIRE(std::find(party.items().begin(), party.items().end(), chest) ==
          party.items().end());
  REQUIRE(std::find(party.items().begin(), party.items().end(), helm) !=
          party.items().end());
}
