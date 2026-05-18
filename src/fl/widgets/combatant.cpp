#include "combatant.hpp"

#include "fl/ecs/components/atb_charge.hpp"
#include "fl/ecs/components/hp_bar_color_override.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/ecs/components/visual_effects.hpp"
#include "fl/lospec500.hpp"
#include "fl/widgets/effects/flame_wave.hpp"

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>

#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>

namespace fl::widgets {
namespace {

class FlameWaveDecalNode : public ftxui::Node {
public:
  FlameWaveDecalNode(ftxui::Element child, float progress)
      : ftxui::Node(ftxui::Elements{std::move(child)}), progress_(progress) {}

  void ComputeRequirement() override {
    children_[0]->ComputeRequirement();
    requirement_ = children_[0]->requirement();
  }

  void SetBox(ftxui::Box box) override {
    box_ = box;
    children_[0]->SetBox(box);
  }

  void Render(ftxui::Screen &screen) override {
    children_[0]->Render(screen);

    const int width = box_.x_max - box_.x_min + 1;
    const int combatant_height = box_.y_max - box_.y_min + 1;
    const int decal_height = combatant_height + kExtraHeight;
    if (width <= 0 || combatant_height <= 0) {
      return;
    }

    auto frame = fl::widgets::effects::FlameWave{}.render(progress_, width,
                                                          decal_height);
    const int decal_y_min = box_.y_max - decal_height + 1;
    for (int y = 0; y < frame.height; ++y) {
      const int screen_y = decal_y_min + y;
      if (screen_y < 0 || screen_y >= screen.dimy()) {
        continue;
      }
      for (int x = 0; x < frame.width; ++x) {
        const int screen_x = box_.x_min + x;
        if (screen_x < 0 || screen_x >= screen.dimx()) {
          continue;
        }

        const auto &cell = frame.at(x, y);
        if (!cell.active()) {
          continue;
        }

        auto &pixel = screen.PixelAt(screen_x, screen_y);
        const bool had_text = pixel.character != " ";
        if (cell.bg) {
          pixel.background_color = *cell.bg;
        }
        if (cell.fg) {
          pixel.foreground_color = *cell.fg;
        }
        if (cell.glyph != ' ') {
          pixel.character = std::string(1, cell.glyph);
        } else if (had_text && cell.bg) {
          pixel.foreground_color = ftxui::Color::White;
        }
      }
    }
  }

private:
  static constexpr int kExtraHeight = 2;
  float progress_ = 0.0F;
};

ftxui::Element flame_wave_decal(ftxui::Element child, float progress) {
  return std::make_shared<FlameWaveDecalNode>(std::move(child), progress);
}

} // namespace

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

      auto uwu_text = "ATB: [" + uwu_bar + "] " + std::to_string(atb->charge) +
                      "/" + std::to_string(atb->max_charge);
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

  if (auto *co = reg.try_get<ResolvedColorOverride>(entity)) {
    border = border | ftxui::color(co->color);
  }

  if (auto *bg = reg.try_get<ResolvedBackgroundColorOverride>(entity)) {
    border = border | ftxui::bgcolor(bg->color);
  }

  if (auto *flame = reg.try_get<FlameWaveDecal>(entity)) {
    border = flame_wave_decal(std::move(border),
                              flame->progress_at(FlameWaveDecal::Clock::now()));
  }

  // <-- key: allow the whole Combatant box to flex horizontally
  return border | ftxui::xflex;
}

} // namespace fl::widgets
