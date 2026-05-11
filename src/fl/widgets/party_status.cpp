#include "party_status.hpp"

#include "fl/ecs/components/stats.hpp"
#include "fl/lospec500.hpp"

namespace fl::widgets {

PartyStatus::PartyStatus(fl::primitives::PartyData &party_data)
    : party_data_(party_data) {}

ftxui::Element PartyStatus::Render() {
  // Determine party status
  std::string status_text;
  ftxui::Color status_color;
  std::string detail_text;

  if (party_data_.has_encounter()) {
    status_text = "FIGHTING";
    status_color = fl::lospec500::color_at(20); // Bright green
    detail_text = "";
  } else if (party_data_.all_members_dead()) {
    status_text = "DEAD";
    status_color = fl::lospec500::color_at(6); // Dead color (red/dark)
    detail_text = "";
  } else if (party_data_.town_penalty_active()) {
    status_text = "FIXING";
    status_color = fl::lospec500::color_at(27); // Cyan/blue
    
    // Calculate progress bar for remaining penalty time
    int remaining = party_data_.town_penalty_beats_remaining();
    int total = fl::primitives::PartyData::kTownPenaltyBeats;
    float percent = static_cast<float>(remaining) / static_cast<float>(total);
    
    constexpr int bar_width = 20;
    int filled = static_cast<int>(bar_width * percent);
    if (filled < 0) filled = 0;
    if (filled > bar_width) filled = bar_width;
    int empty = bar_width - filled;
    if (empty < 0) empty = 0;
    
    std::string bar;
    bar.reserve(bar_width * 2);
    for (int i = 0; i < filled; ++i) {
      bar += "·";
    }
    for (int i = 0; i < empty; ++i) {
      bar += "-";
    }
    
    detail_text = "[" + bar + "] " + std::to_string(remaining) + "b";
  } else {
    status_text = "IDLE";
    status_color = fl::lospec500::color_at(20); // Bright green
    detail_text = "";
  }

  // Build the display element with outline matching Combatant widget
  ftxui::Element content;
  if (detail_text.empty()) {
    content = ftxui::vbox({
        ftxui::text(status_text) | ftxui::center | ftxui::bold,
        ftxui::filler(),
    });
  } else {
    content = ftxui::vbox({
        ftxui::text(status_text) | ftxui::center | ftxui::bold,
        ftxui::text(detail_text) | ftxui::center,
        ftxui::filler(),
    });
  }

  ftxui::Element bordered =
      ftxui::window(ftxui::text(""), content) | ftxui::color(status_color);

  return bordered | ftxui::xflex;
}

} // namespace fl::widgets
