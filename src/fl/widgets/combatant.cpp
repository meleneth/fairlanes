#include "combatant.hpp"

#include "fl/ecs/components/atb_charge.hpp"
#include "fl/ecs/components/color_override.hpp"
#include "fl/ecs/components/hp_bar_color_override.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/lospec500.hpp"

namespace fl::widgets {

Combatant::Combatant(entt::registry &reg_, entt::entity entity_,
                     bool render_uwu)
    : reg(reg_), entity(entity_), render_uwu_(render_uwu) {}

ftxui::Element Combatant::Render() {
  using namespace fl::ecs::components;
  if (!reg.valid(entity)) {
    return ftxui::window(ftxui::text("[invalid]"),
                         ftxui::text("entity not valid")) |
           ftxui::xflex;
  }

  auto *the_stats = reg.try_get<Stats>(entity);
  if (!the_stats) {
    return ftxui::window(ftxui::text("[no Stats]"),
                         ftxui::text("missing Stats")) |
           ftxui::xflex;
  }

  auto &stats = reg.get<Stats>(entity);
  auto &level = reg.get<TrackXP>(entity);

  // Avoid div-by-zero and clamp [0, 1].
  float percent = 0.0f;
  if (stats.max_hp_ > 0) {
    percent = static_cast<float>(stats.hp_) / static_cast<float>(stats.max_hp_);
    if (percent < 0.0f)
      percent = 0.0f;
    if (percent > 1.0f)
      percent = 1.0f;
  }

  // Keep your fixed-width bar for stability:
  constexpr int bar_width = 20;
  int filled = static_cast<int>(bar_width * percent);
  if (filled < 0)
    filled = 0;
  if (filled > bar_width)
    filled = bar_width;
  int empty = bar_width - filled;
  if (empty < 0)
    empty = 0;

  std::string bar;
  bar.reserve(bar_width);
  bar.append(static_cast<std::size_t>(filled), '#');
  bar.append(static_cast<std::size_t>(empty), '-');

  // Build the HP line element
  auto hp_text = "HP: [" + bar + "] " + std::to_string(stats.hp_) + "/" +
                 std::to_string(stats.max_hp_);

  ftxui::Element hp_line = ftxui::hbox({
      ftxui::text(hp_text),
      ftxui::filler(),
  });
  if (auto *hp_color = reg.try_get<HPBarColorOverride>(entity);
      hp_color != nullptr && stats.is_alive()) {
    hp_line = hp_line | ftxui::color(hp_color->color);
  }

  // Build the uWu (ATB charge) bar if requested
  std::vector<ftxui::Element> content_lines;
  content_lines.push_back(std::move(hp_line));

  if (render_uwu_) {
    auto *atb = reg.try_get<fl::ecs::components::AtbCharge>(entity);
    if (atb != nullptr) {
      float uwu_pct = 0.0f;
      if (atb->max_charge > 0) {
        uwu_pct = static_cast<float>(atb->charge) /
                  static_cast<float>(atb->max_charge);
        if (uwu_pct < 0.0f)
          uwu_pct = 0.0f;
        if (uwu_pct > 1.0f)
          uwu_pct = 1.0f;
      }
      int uwu_filled = static_cast<int>(bar_width * uwu_pct);
      if (uwu_filled < 0)
        uwu_filled = 0;
      if (uwu_filled > bar_width)
        uwu_filled = bar_width;
      int uwu_empty = bar_width - uwu_filled;

      std::string uwu_bar;
      uwu_bar.reserve(bar_width);
      uwu_bar.append(static_cast<std::size_t>(uwu_filled), '#');
      uwu_bar.append(static_cast<std::size_t>(uwu_empty), '-');

      auto uwu_text = "ATB: [" + uwu_bar + "] " +
                      std::to_string(atb->charge) + "/" +
                      std::to_string(atb->max_charge);
      content_lines.push_back(ftxui::hbox({
          ftxui::text(uwu_text),
          ftxui::filler(),
      }));
    }
  }

  // Top border labels
  // clang-format off
  ftxui::Element border = ftxui::window(
    ftxui::hbox(ftxui::Elements {
      ftxui::text(stats.name_) | ftxui::color(ftxui::Color::BlueLight),
      ftxui::filler(),
      ftxui::text("[") | ftxui::bgcolor(fl::lospec500::color_at(1)) | ftxui::color(fl::lospec500::color_at(32)),
     
      ftxui::text(std::to_string(level.level_)) | ftxui::bgcolor(fl::lospec500::color_at(9)) | ftxui::color(fl::lospec500::color_at(15)),
      ftxui::text("]") | ftxui::bgcolor(fl::lospec500::color_at(1)) | ftxui::color(fl::lospec500::color_at(32))
    }),
    ftxui::vbox(std::move(content_lines))
  );
  // clang-format on
  
  // Apply dead color if HP <= 0
  if (!stats.is_alive()) {
    border = border | ftxui::color(fl::lospec500::color_at(6));
  } else if (auto *co = reg.try_get<ColorOverride>(entity)) {
    border = border | ftxui::color(co->color);
  }
  // <-- key: allow the whole Combatant box to flex horizontally
  return border | ftxui::xflex;
}

} // namespace fl::widgets
