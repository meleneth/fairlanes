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

  auto &is_account =
      ctx_.reg().get<fl::ecs::components::IsAccount>(ctx_.self());
  for (auto &party : is_account.account_data().parties()) {
    auto &is_party =
        ctx_.reg().get<fl::ecs::components::IsParty>(party.party_id());
    if (is_party.party_data().party_id() == ctx_.self()) {
      rows.push_back(ftxui::text("This is your party:"));
    } else {
      rows.push_back(ftxui::text("This is the opposition:"));
    }
  }
  for (auto &party : is_account.account_data().parties()) {
    using fl::ecs::components::Encounter;
    using fl::ecs::components::IsParty;
    auto &is_party = ctx_.reg().get<IsParty>(party.party_id());
    auto *encounter = ctx_.reg().try_get<Encounter>(party.party_id());

    if (encounter) {
      std::vector<ftxui::Element> attackers_row;
      for (auto &member : encounter->encounter_data().attackers()) {
        fl::widgets::Combatant combatant{ctx_.reg(), member};
        attackers_row.push_back(combatant.Render() | ftxui::xflex);
      }
      while (attackers_row.size() < 5) {
        attackers_row.push_back(ftxui::filler() | ftxui::xflex);
      }
      rows.push_back(ftxui::hbox(std::move(attackers_row)));

    } else {
      ftxui::Element blank = ftxui::vbox({
                                 ftxui::filler(), // line 1
                                 ftxui::filler(), // line 2
                                 ftxui::filler(), // line 3
                             }) |
                             ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 1) |
                             ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 3);
      rows.push_back(blank);
    }

    std::vector<ftxui::Element> party_row;
    is_party.for_each_member([&](entt::entity member) {
      fl::widgets::Combatant combatant{ctx_.reg(), member};
      party_row.push_back(combatant.Render() | ftxui::xflex);
    });
    rows.push_back(ftxui::hbox(std::move(party_row)));
  }
  return ftxui::vbox(std::move(rows));
}

} // namespace fl::widgets
