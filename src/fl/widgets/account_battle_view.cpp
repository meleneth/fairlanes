#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "account_battle_view.hpp"
#include "fl/context.hpp"
#include "fl/ecs/components/encounter.hpp"
#include "fl/ecs/components/is_account.hpp"
#include "fl/ecs/components/is_party.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/selected_account.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/team.hpp"
#include "fl/widgets/combatant.hpp"

namespace fl::widgets {

AccountBattleView::AccountBattleView(fl::context::EntityCtx ctx)
    : ctx_(std::move(ctx)) {}

ftxui::Element AccountBattleView::Render() {
  std::vector<ftxui::Element> rows;
  auto *check_is_account =
      ctx_.reg().try_get<fl::ecs::components::IsAccount>(ctx_.self());
  if (!check_is_account) {
    return ftxui::window(
        ftxui::text("AccountBattleView: ctx_.self() is not an account"),
        ftxui::text("Missing IsAccount on ctx_.self()"));
  }

  auto &is_account =
      ctx_.reg().get<fl::ecs::components::IsAccount>(ctx_.self());
  auto &parties = is_account.account_data().parties();

  auto blank_row_5 = [&] {
    std::vector<ftxui::Element> row;
    row.reserve(5);
    for (int i = 0; i < 5; ++i)
      row.push_back(ftxui::filler() | ftxui::xflex);
    return ftxui::hbox(std::move(row));
  };

  auto render_entity_row_5 = [&](const std::vector<entt::entity> &ents) {
    std::vector<ftxui::Element> row;
    row.reserve(5);

    for (std::size_t i = 0; i < ents.size() && row.size() < 5; ++i) {
      fl::widgets::Combatant combatant{ctx_.reg(), ents[i]};
      row.push_back(combatant.Render() | ftxui::xflex);
    }
    while (row.size() < 5)
      row.push_back(ftxui::filler() | ftxui::xflex);

    return ftxui::hbox(std::move(row));
  };

  auto render_memberdata_row_5 =
      [&](const std::deque<fl::primitives::MemberData> &members) {
        std::vector<ftxui::Element> row;
        row.reserve(5);

        std::size_t n = 0;
        for (const auto &m : members) {
          if (n++ >= 5)
            break;
          fl::widgets::Combatant combatant{ctx_.reg(), m.member_id()};
          row.push_back(combatant.Render() | ftxui::xflex);
        }
        while (row.size() < 5)
          row.push_back(ftxui::filler() | ftxui::xflex);

        return ftxui::hbox(std::move(row));
      };

  for (std::size_t i = 0; i < parties.size(); ++i) {
    auto &party = parties[i];

    if (party.has_encounter()) {
      auto &enc = party.encounter_data();

      // Render BOTH sides from EncounterData (2 rows).
      rows.push_back(render_entity_row_5(enc.attackers().members()));
      rows.push_back(render_entity_row_5(enc.defenders().members()));
    } else {
      // No encounter: placeholder "enemy row" + still render players.
      rows.push_back(blank_row_5());
      rows.push_back(render_memberdata_row_5(party.members()));
    }
  }

  if (rows.empty()) {
    rows.push_back(ftxui::text("No parties."));
  }

  return ftxui::vbox(std::move(rows));
}

} // namespace fl::widgets
