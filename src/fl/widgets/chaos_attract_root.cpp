#include "chaos_attract_root.hpp"

#include <array>
#include <cstddef>
#include <utility>

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "fl/lospec500.hpp"

namespace fl::widgets {

ChaosAttractRoot::ChaosAttractRoot(ftxui::Component battle_surface)
    : battle_surface_(std::move(battle_surface)) {
  if (battle_surface_) {
    Add(battle_surface_);
  }
}

bool ChaosAttractRoot::OnEvent(ftxui::Event event) {
  if (help_open_) {
    if (event == ftxui::Event::Character("h") ||
        event == ftxui::Event::Escape) {
      help_open_ = false;
      return true;
    }

    return true;
  }

  if (event == ftxui::Event::Character("h")) {
    help_open_ = true;
    return true;
  }

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

  return ftxui::dbox({
      std::move(surface),
      ftxui::vbox({
          ftxui::filler(),
          render_begin_prompt(),
          ftxui::text(""),
      }),
      help_open_ ? render_help_panel() : ftxui::filler(),
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
      text("UP/DOWN SELECT  h HELP  ENTER TO BEGIN") | bold |
          color(prompt_color(render_frames_)) |
          bgcolor(fl::lospec500::color_at(0)),
      filler(),
  });
}

ftxui::Element ChaosAttractRoot::render_help_panel() const {
  using namespace ftxui;

  const auto chrome = fl::lospec500::on_not_black(fl::lospec500::color_at(32));
  const auto accent = fl::lospec500::on_not_black(fl::lospec500::color_at(15));
  const auto bg = bgcolor(fl::lospec500::color_at(0));

  auto lines = vbox({
      text("Chaos Attract") | bold | accent,
      separator() | chrome,
      text("h         close this help") | chrome,
      text("Up / Down select party") | chrome,
      text("k / j     select party") | chrome,
      text("[ / ]     select party") | chrome,
      text("Enter     begin normal game") | chrome,
      text("q / Esc   quit") | chrome,
  });

  auto panel = window(text("Help") | accent, lines | bg) | chrome | bg |
               clear_under | size(WIDTH, GREATER_THAN, 36);

  return vbox({
      filler(),
      hbox({
          filler(),
          panel,
          filler(),
      }),
      filler(),
  });
}

} // namespace fl::widgets
