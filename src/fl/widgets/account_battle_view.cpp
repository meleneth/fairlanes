#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>

#include "account_battle_view.hpp"
#include "fl/context.hpp"
#include "fl/ecs/components/encounter.hpp"
#include "fl/ecs/components/is_account.hpp"
#include "fl/ecs/components/is_party.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/selected_account.hpp"
#include "fl/lospec500.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/team.hpp"
#include "fl/widgets/combatant.hpp"
#include "fl/widgets/party_status.hpp"

namespace fl::widgets {

AccountBattleView::AccountBattleView(fl::context::AccountCtx ctx)
    : ctx_(std::move(ctx)) {}

bool AccountBattleView::OnEvent(ftxui::Event event) {
  auto *check_is_account =
      ctx_.reg().try_get<fl::ecs::components::IsAccount>(ctx_.self());
  if (!check_is_account) {
    return false;
  }

  auto &account = check_is_account->account_data();
  const int log_count = static_cast<int>(account.parties().size()) + 1;
  focused_log_ = std::clamp(focused_log_, 0, std::max(0, log_count - 1));

  if (event == ftxui::Event::Tab) {
    focused_log_ = (focused_log_ + 1) % log_count;
    return true;
  }

  if (event == ftxui::Event::TabReverse) {
    focused_log_ = (focused_log_ + log_count - 1) % log_count;
    return true;
  }

  if (focused_log_ == 0) {
    return account.log().OnEvent(event);
  }

  return account.parties()[static_cast<std::size_t>(focused_log_ - 1)]
      .log()
      .OnEvent(event);
}

ftxui::Element AccountBattleView::Render() {
  std::vector<ftxui::Element> rows;
  auto *check_is_account =
      ctx_.reg().try_get<fl::ecs::components::IsAccount>(ctx_.self());
  if (!check_is_account) {
    return ftxui::window(
               ftxui::text("AccountBattleView: ctx_.self() is not an account"),
               ftxui::text("Missing IsAccount on ctx_.self()")) |
           ftxui::bgcolor(fl::lospec500::color_at(0)) |
           ftxui::color(fl::lospec500::color_at(32));
  }

  auto &is_account =
      ctx_.reg().get<fl::ecs::components::IsAccount>(ctx_.self());
  auto &parties = is_account.account_data().parties();
  focused_log_ = std::clamp(focused_log_, 0,
                            std::max(0, static_cast<int>(parties.size())));

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
      // No encounter: party status row + still render players.
      fl::widgets::PartyStatus party_status{party};
      std::vector<ftxui::Element> status_row;
      status_row.push_back(party_status.Render() | ftxui::xflex);
      while (status_row.size() < 5)
        status_row.push_back(ftxui::filler() | ftxui::xflex);
      rows.push_back(ftxui::hbox(std::move(status_row)));
      rows.push_back(render_memberdata_row_5(party.members()));
    }
  }

  if (rows.empty()) {
    rows.push_back(ftxui::text("No parties."));
  }

  std::vector<ftxui::Element> log_panes;
  log_panes.reserve(parties.size() + 1);

  is_account.account_data().log().set_focused(focused_log_ == 0);
  log_panes.push_back(is_account.account_data().log().Render() | ftxui::frame |
                      ftxui::vscroll_indicator | ftxui::flex);

  for (std::size_t i = 0; i < parties.size(); ++i) {
    auto &party = parties[i];
    party.log().set_focused(focused_log_ == static_cast<int>(i + 1));
    log_panes.push_back(party.log().Render() | ftxui::frame |
                        ftxui::vscroll_indicator | ftxui::flex);
  }

  if (!log_panes.empty()) {
    rows.push_back(ftxui::hbox(std::move(log_panes)) | ftxui::flex);
  }

  return ftxui::vbox(std::move(rows)) |
         ftxui::bgcolor(fl::lospec500::color_at(0)) |
         ftxui::color(fl::lospec500::color_at(32));
}

} // namespace fl::widgets
