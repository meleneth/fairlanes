#include "chaos_attract_root.hpp"

#include <array>
#include <cstddef>
#include <utility>

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "fl/lospec500.hpp"
#include "fl/widgets/moon_calendar_view.hpp"

namespace fl::widgets {

ChaosAttractRoot::ChaosAttractRoot(
    const fl::primitives::WorldClock *world_clock,
    ftxui::Component battle_surface)
    : world_clock_(world_clock), battle_surface_(std::move(battle_surface)) {
  if (battle_surface_) {
    Add(battle_surface_);
  }
}

bool ChaosAttractRoot::OnEvent(ftxui::Event event) {
  if (event == ftxui::Event::Return) {
    begin_requested_ = true;
    return true;
  }

  if (battle_surface_ && battle_surface_->OnEvent(event)) {
    return true;
  }

  return true;
}

ftxui::Element ChaosAttractRoot::Render() {
  ++render_frames_;

  ftxui::Element surface =
      battle_surface_ ? battle_surface_->Render() : ftxui::filler();
  if (world_clock_ != nullptr) {
    surface = ftxui::vbox({
        render_moon_calendar(*world_clock_),
        ftxui::separator() |
            fl::lospec500::on_not_black(fl::lospec500::color_at(32)),
        std::move(surface) | ftxui::flex,
    });
  }

  return ftxui::dbox({
      std::move(surface),
      ftxui::vbox({
          ftxui::filler(),
          render_begin_prompt(),
          ftxui::text(""),
      }),
  });
}

ftxui::Color ChaosAttractRoot::prompt_color(std::uint64_t frame) const {
  static constexpr std::array<std::size_t, 4> pulse_colors{15, 14, 8, 14};
  constexpr std::uint64_t frames_per_step = 12;
  const auto phase =
      static_cast<std::size_t>((frame / frames_per_step) % pulse_colors.size());
  return fl::lospec500::color_at(pulse_colors[phase]);
}

ftxui::Element ChaosAttractRoot::render_begin_prompt() const {
  using namespace ftxui;

  return hbox({
      filler(),
      text("ENTER TO BEGIN") | bold | color(prompt_color(render_frames_)) |
          bgcolor(fl::lospec500::color_at(0)),
      filler(),
  });
}

} // namespace fl::widgets
