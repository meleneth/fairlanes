#include "player_details_pane.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <type_traits>

#include "fl/ecs/components/equipment.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/widgets/equipment_label.hpp"

namespace fl::widgets {

namespace {

std::string entity_label(entt::entity entity) {
  return "#" + std::to_string(static_cast<std::underlying_type_t<entt::entity>>(
                   entity));
}

ftxui::Element equipment_label(entt::registry &reg, entt::entity equipment_id) {
  if (equipment_id == entt::null || !reg.valid(equipment_id)) {
    return ftxui::text("-");
  }

  if (auto *equipment =
          reg.try_get<fl::ecs::components::Equipment>(equipment_id)) {
    return equipment_name_label(*equipment);
  }

  return ftxui::text(entity_label(equipment_id));
}

} // namespace

PlayerDetailsPane::PlayerDetailsPane(entt::registry &reg,
                                     fl::primitives::PartyData &party)
    : reg_(reg), party_(party) {}

bool PlayerDetailsPane::OnEvent(ftxui::Event event) {
  const int member_count = static_cast<int>(party_.members().size());
  if (member_count <= 0) {
    cursor_ = 0;
    return false;
  }

  cursor_ = std::clamp(cursor_, 0, member_count - 1);

  if (event == ftxui::Event::Character("]")) {
    cursor_ = (cursor_ + 1) % member_count;
    return true;
  }

  if (event == ftxui::Event::Character("[")) {
    cursor_ = (cursor_ + member_count - 1) % member_count;
    return true;
  }

  return false;
}

ftxui::Element PlayerDetailsPane::Render() {
  using namespace ftxui;

  const auto &members = party_.members();
  const int member_count = static_cast<int>(members.size());
  auto title = focused_ ? text("Player") | inverted : text("Player");

  if (member_count <= 0) {
    cursor_ = 0;
    return window(title, text("No members."));
  }

  cursor_ = std::clamp(cursor_, 0, member_count - 1);

  const auto &member = members[static_cast<std::size_t>(cursor_)];
  const auto member_id = member.member_id();
  const auto *stats = reg_.try_get<fl::ecs::components::Stats>(member_id);

  const std::string name = stats ? stats->name_ : member.name();
  Elements lines;
  lines.push_back(text(name) | bold);
  lines.push_back(text("Member " + std::to_string(cursor_ + 1) + "/" +
                       std::to_string(member_count)));
  lines.push_back(separator());

  if (stats) {
    lines.push_back(text("HP " + std::to_string(stats->hp_) + "/" +
                         std::to_string(stats->max_hp_)));
    lines.push_back(text("MP " + std::to_string(stats->mp_) + "/" +
                         std::to_string(stats->max_mp_)));
  } else {
    lines.push_back(text("No stats."));
  }

  lines.push_back(separator());
  lines.push_back(text("Gear") | bold);

  auto *party_member = reg_.try_get<fl::ecs::components::PartyMember>(member_id);
  if (!party_member || party_member->closet_entity_id() == entt::null ||
      !reg_.all_of<fl::ecs::components::Closet>(
          party_member->closet_entity_id())) {
    lines.push_back(text("No gear data."));
    return window(title, vbox(std::move(lines)) | yframe | vscroll_indicator |
                             flex);
  }

  const auto &closet = party_member->closet();
  const std::array<std::pair<std::string_view, entt::entity>, 14> gear{{
      {"mainhand", closet.mainhand},
      {"offhand", closet.offhand},
      {"chest", closet.chest},
      {"helm", closet.helm},
      {"pants", closet.pants},
      {"boots", closet.boots},
      {"gloves", closet.gloves},
      {"sleeves", closet.sleeves},
      {"belt", closet.belt},
      {"cape", closet.cape},
      {"necklace", closet.necklace},
      {"ring_1", closet.ring_1},
      {"ring_2", closet.ring_2},
      {"knife", closet.knife},
  }};

  for (const auto &[slot, equipment_id] : gear) {
    lines.push_back(hbox({
        text(std::string(slot) + ": ") | dim,
        equipment_label(reg_, equipment_id) | flex,
    }));
  }

  return window(title, vbox(std::move(lines)) | yframe | vscroll_indicator |
                           flex);
}

void PlayerDetailsPane::set_focused(bool focused) noexcept {
  focused_ = focused;
}

} // namespace fl::widgets
