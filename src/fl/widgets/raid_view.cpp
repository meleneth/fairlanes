#include "fl/widgets/raid_view.hpp"
#include "fl/widgets/combatant.hpp"
#include "fl/primitives/account_data.hpp"
#include <ftxui/dom/node.hpp>
#include <ftxui/dom/elements.hpp>
#include <algorithm>
#include <cmath>
#include <ftxui/screen/screen.hpp>

namespace fl::widgets {
namespace {
// A terminal-native celestial body scales to the actual boss allocation.
class VisitorPortrait : public ftxui::Node {
public:
  VisitorPortrait(ftxui::Element labels, std::uint64_t beat)
      : Node({std::move(labels)}), beat_(beat) {}
  void ComputeRequirement() override {
    children_[0]->ComputeRequirement();
    requirement_ = children_[0]->requirement();
    requirement_.flex_grow_x = requirement_.flex_grow_y = 1;
  }
  void SetBox(ftxui::Box box) override {
    box_ = box;
    children_[0]->SetBox(box);
  }
  void Render(ftxui::Screen &screen) override {
    const double width = std::max(1, box_.x_max - box_.x_min);
    const double height = std::max(1, box_.y_max - box_.y_min);
    const double phase = static_cast<double>(beat_) / 12.0;
    for (int y = box_.y_min; y <= box_.y_max; ++y) {
      for (int x = box_.x_min; x <= box_.x_max; ++x) {
        if (x < 0 || y < 0 || x >= screen.dimx() || y >= screen.dimy()) continue;
        const double nx = 2.0 * (x - box_.x_min) / width - 1.0;
        const double ny = 2.0 * (y - box_.y_min) / height - 1.0;
        const double ring = std::sqrt(nx * nx + ny * ny);
        auto &pixel = screen.PixelAt(x, y);
        pixel.background_color = ftxui::Color::RGB(12, 5, 25);
        if (std::abs(ring - 0.78) < 0.10 || std::abs(ny - 0.3 * std::sin(nx * 7 + phase)) < 0.08) {
          pixel.character = ring < 0.65 ? "*" : ":";
          pixel.foreground_color = ftxui::Color::MagentaLight;
        } else if ((x * 13 + y * 7 + beat_ / 12) % 37 == 0) {
          pixel.character = ".";
          pixel.foreground_color = ftxui::Color::BlueLight;
        }
      }
    }
    children_[0]->Render(screen);
  }
private:
  std::uint64_t beat_;
};

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
  Elements boss_rows{text("VISITOR RAID") | color(Color::White) | bold};
  if (raid->active()) {
    boss_rows.push_back(filler());
    for (auto enemy : raid->encounter().attackers())
      if (ctx_.reg().valid(enemy)) boss_rows.push_back(Combatant(ctx_.reg(), enemy).Render());
    boss_rows.push_back(filler());
    boss_rows.push_back(text("Account time paused | One shared encounter") | color(Color::White) | dim);
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
  rows.push_back(text(raid->active() ? "h: help" :
      "Enter: return to party combat | Wipe results close after 5 minutes | h: help") | dim);
  Element boss = vbox(std::move(boss_rows));
  if (raid->active()) boss = std::make_shared<VisitorPortrait>(std::move(boss), raid->combat_beats());
  return std::make_shared<RaidLayout>(std::move(boss), vbox(std::move(rows)));
}
} // namespace fl::widgets
