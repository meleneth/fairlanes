#include "fl/widgets/raid_view.hpp"
#include "fl/widgets/combatant.hpp"
#include "fl/primitives/account_data.hpp"
#include <ftxui/dom/node.hpp>
#include <ftxui/dom/elements.hpp>
#include <algorithm>

namespace fl::widgets {
namespace {
// Allocate the boss its upper third; the roster always gets the lower two thirds.
class RaidLayout : public ftxui::Node {
public:
  RaidLayout(ftxui::Element boss, ftxui::Element roster)
      : Node({std::move(boss), std::move(roster)}) {}
  void ComputeRequirement() override {
    for (auto &child : children_) child->ComputeRequirement();
    requirement_.min_x = 80;
    requirement_.min_y = 18;
    requirement_.flex_grow_x = requirement_.flex_grow_y = 1;
  }
  void SetBox(ftxui::Box box) override {
    box_ = box;
    int split = box.y_min + (box.y_max - box.y_min + 1) / 3;
    auto top = box;
    top.y_max = split - 1;
    auto bottom = box;
    bottom.y_min = split;
    children_[0]->SetBox(top);
    children_[1]->SetBox(bottom);
  }
};
}

ftxui::Element RaidView::Render() {
  using namespace ftxui;
  auto &account = ctx_.account_data();
  auto *raid = account.raid();
  if (!raid) return text("No Visitor encounter.");
  Elements boss_rows{text("VISITOR RAID") | bold};
  if (raid->active()) {
    static constexpr const char *sky[]{".   *   .   /\\   .   *   .", "  .   *   <  >   *   .  ", "*   .     \\/     .   *"};
    boss_rows.push_back(text(sky[(raid->combat_beats() / 6) % 3]) | center | color(Color::MagentaLight));
    for (auto enemy : raid->encounter().attackers())
      if (ctx_.reg().valid(enemy)) boss_rows.push_back(Combatant(ctx_.reg(), enemy).Render());
    boss_rows.push_back(text("Account time paused | One shared encounter") | dim);
  } else {
    const auto result = *raid->result();
    boss_rows.push_back(text(result == fl::events::RaidResult::Victory ? "VICTORY" :
        result == fl::events::RaidResult::MutualDestruction ? "WIPE - MUTUAL DESTRUCTION" : "WIPE") | bold);
    boss_rows.push_back(text("This Visitor is spent. No retries."));
    if (result == fl::events::RaidResult::Victory)
      boss_rows.push_back(text("Two exclusive trinkets awarded to every party."));
    if (result == fl::events::RaidResult::MutualDestruction)
      boss_rows.push_back(text("Raid mutual-destruction achievement fulfilled."));
    const auto &records = account.raid_records();
    boss_rows.push_back(text("Account raids: " + std::to_string(records.attempts) +
        " | Wins: " + std::to_string(records.wins) + " | Wipes: " +
        std::to_string(records.defeats) + " | Trinkets: " +
        std::to_string(records.trinkets_awarded)));
  }
  Elements rows;
  std::size_t party_index = 0;
  const auto active = raid->encounter().atb_engine().active_combatant();
  for (const auto &party : account.parties()) {
    Elements members{text("P" + std::to_string(++party_index) + " ") | size(WIDTH, EQUAL, 3)};
    for (const auto &member : party.members()) {
      const auto id = member.member_id();
      members.push_back(Combatant(ctx_.reg(), id, false, id == active, true).Render() | flex);
    }
    rows.push_back(hbox(std::move(members)) | size(HEIGHT, EQUAL, 2));
    rows.push_back(filler());
  }
  const auto seconds = (account.beats_until_visitor() + account.calendar().effective_beats_per_wall_second() - 1) /
                        account.calendar().effective_beats_per_wall_second();
  rows.push_back(text("Next Visitor: " + std::to_string(seconds / 3600) + "h " +
      std::to_string((seconds / 60) % 60) + "m " + std::to_string(seconds % 60) + "s" +
      (raid->active() ? " (paused) | h: help" : " | Tab: party view | h: help")) | dim);
  return std::make_shared<RaidLayout>(vbox(std::move(boss_rows)), vbox(std::move(rows)));
}
} // namespace fl::widgets
