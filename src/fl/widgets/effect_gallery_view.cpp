#include "effect_gallery_view.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/dom/elements.hpp>

#include "combatant.hpp"
#include "fl/context.hpp"
#include "fl/ecs/components/atb_charge.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/ecs/components/visual_effects.hpp"
#include "fl/lospec500.hpp"
#include "fl/widgets/battle_render_budget.hpp"
#include "fl/widgets/textures/bog_background.hpp"
#include "fl/widgets/textures/forest_background.hpp"
#include "fl/widgets/textures/savannah_background.hpp"

namespace fl::widgets {
namespace {

constexpr int kRootChromeHeight = 2;
constexpr int kMinCombatantHeight = 6;
constexpr int kGalleryFrameMilliseconds = 1200;
constexpr std::array<std::string_view, 6> kSampleNames{
    "Ash Meridian", "Bitrune Adept", "Cinder Finch",
    "Dawn Courier", "Echo Warden",   "Flux Knight",
};

enum class GalleryBackground {
  Forest,
  Savannah,
  Bog,
};

constexpr std::array<GalleryBackground, 3> kGalleryBackgrounds{
    GalleryBackground::Forest,
    GalleryBackground::Savannah,
    GalleryBackground::Bog,
};

std::string_view background_name(GalleryBackground background) noexcept {
  switch (background) {
  case GalleryBackground::Forest:
    return "Forest";
  case GalleryBackground::Savannah:
    return "Savannah";
  case GalleryBackground::Bog:
    return "Bog";
  }

  return "Unknown";
}

std::size_t effect_count() noexcept {
  return effects::available_decal_animation_kinds().size();
}

std::size_t wrapped_index(std::size_t index) noexcept {
  const auto count = effect_count();
  return count == 0 ? 0 : index % count;
}

std::size_t background_count() noexcept { return kGalleryBackgrounds.size(); }

std::size_t wrapped_background_index(std::size_t index) noexcept {
  const auto count = background_count();
  return count == 0 ? 0 : index % count;
}

GalleryBackground background_at(std::size_t index) noexcept {
  if (kGalleryBackgrounds.empty()) {
    return GalleryBackground::Forest;
  }
  return kGalleryBackgrounds[wrapped_background_index(index)];
}

effects::DecalAnimationKind effect_kind_at(std::size_t index) noexcept {
  const auto &catalog = effects::available_decal_animation_kinds();
  if (catalog.empty()) {
    return effects::DecalAnimationKind::FlameWave;
  }
  return catalog[wrapped_index(index)];
}

std::string_view effect_name_at(std::size_t index) noexcept {
  return effects::name(effect_kind_at(index));
}

int columns_for(const BattleRenderBudget &budget, int count) noexcept {
  if (count <= 1) {
    return 1;
  }

  switch (budget.profile) {
  case BattleLayoutProfile::Tiny:
    return 1;
  case BattleLayoutProfile::Compact:
    return std::min(2, count);
  case BattleLayoutProfile::Standard:
  case BattleLayoutProfile::Wide:
  case BattleLayoutProfile::Showcase:
    return std::min(3, count);
  }

  return 1;
}

int max_visible_for(const BattleRenderBudget &budget) noexcept {
  switch (budget.profile) {
  case BattleLayoutProfile::Tiny:
    return 1;
  case BattleLayoutProfile::Compact:
    return 2;
  case BattleLayoutProfile::Standard:
    return 4;
  case BattleLayoutProfile::Wide:
  case BattleLayoutProfile::Showcase:
    return 6;
  }

  return 1;
}

std::chrono::steady_clock::time_point
looped_start_time(std::chrono::steady_clock::time_point origin,
                  std::chrono::milliseconds duration) {
  const auto now = std::chrono::steady_clock::now();
  if (duration.count() <= 0) {
    return now;
  }

  const auto elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - origin);
  const auto duration_count = duration.count();
  const auto elapsed_count = elapsed.count();
  const auto looped =
      ((elapsed_count % duration_count) + duration_count) % duration_count;
  return now - std::chrono::milliseconds{looped};
}

ftxui::Element label_row(const EffectGalleryLabels &labels,
                         const EffectGalleryBackgroundLabels &background_labels,
                         BattleLayoutProfile profile) {
  using namespace ftxui;

  const auto muted = fl::lospec500::on_not_black(fl::lospec500::color_at(24));
  const auto accent = fl::lospec500::on_not_black(fl::lospec500::color_at(15));
  const auto current = fl::lospec500::on_not_black(fl::lospec500::color_at(28));

  if (profile == BattleLayoutProfile::Tiny) {
    return hbox({text("Effect ") | muted,
                 text(std::string(labels.current)) | current | bold});
  }

  return vbox({
      hbox({
          text("Prev ") | muted,
          text(std::string(labels.previous)) | accent,
          filler(),
          text("Effect ") | muted,
          text(std::string(labels.current)) | current | bold,
          filler(),
          text("Next ") | muted,
          text(std::string(labels.next)) | accent,
      }),
      hbox({
          text("Bg Prev ") | muted,
          text(std::string(background_labels.previous)) | accent,
          filler(),
          text("Background ") | muted,
          text(std::string(background_labels.current)) | current | bold,
          filler(),
          text("Bg Next ") | muted,
          text(std::string(background_labels.next)) | accent,
      }),
  });
}

ftxui::Element background_panel(GalleryBackground background,
                                ftxui::Element foreground, int width,
                                int height, std::uint32_t seed) {
  switch (background) {
  case GalleryBackground::Savannah:
    return textures::SavannahPanel(std::move(foreground), width, height, seed);
  case GalleryBackground::Bog:
    return textures::BogPanel(std::move(foreground), width, height, seed);
  case GalleryBackground::Forest:
    return textures::ForestPanel(std::move(foreground), width, height, seed);
  }

  return textures::ForestPanel(std::move(foreground), width, height, seed);
}

} // namespace

void EffectGalleryState::next_effect(std::size_t count) noexcept {
  if (count == 0) {
    current_effect_index = 0;
    return;
  }
  current_effect_index = (current_effect_index + 1) % count;
}

void EffectGalleryState::previous_effect(std::size_t count) noexcept {
  if (count == 0) {
    current_effect_index = 0;
    return;
  }
  current_effect_index = (current_effect_index + count - 1) % count;
}

void EffectGalleryState::next_background(std::size_t count) noexcept {
  if (count == 0) {
    current_background_index = 0;
    return;
  }
  current_background_index = (current_background_index + 1) % count;
}

void EffectGalleryState::previous_background(std::size_t count) noexcept {
  if (count == 0) {
    current_background_index = 0;
    return;
  }
  current_background_index = (current_background_index + count - 1) % count;
}

bool EffectGalleryState::set_combatant_count_from_digit(char digit) noexcept {
  if (digit < '1' || digit > '6') {
    return false;
  }
  combatant_count = digit - '0';
  return true;
}

EffectGalleryLabels EffectGalleryState::labels() const noexcept {
  const auto count = effect_count();
  if (count == 0) {
    return EffectGalleryLabels{"", "", ""};
  }

  const auto current = wrapped_index(current_effect_index);
  const auto previous = (current + count - 1) % count;
  const auto next = (current + 1) % count;
  return EffectGalleryLabels{effect_name_at(previous), effect_name_at(current),
                             effect_name_at(next)};
}

EffectGalleryBackgroundLabels
EffectGalleryState::background_labels() const noexcept {
  const auto count = background_count();
  if (count == 0) {
    return EffectGalleryBackgroundLabels{"", "", ""};
  }

  const auto current = wrapped_background_index(current_background_index);
  const auto previous = (current + count - 1) % count;
  const auto next = (current + 1) % count;
  return EffectGalleryBackgroundLabels{background_name(background_at(previous)),
                                       background_name(background_at(current)),
                                       background_name(background_at(next))};
}

std::string_view current_effect_name(const EffectGalleryState &state) noexcept {
  return state.labels().current;
}

EffectGalleryView::EffectGalleryView() { ensure_sample_combatants(); }

bool EffectGalleryView::OnEvent(ftxui::Event event) {
  if (event == ftxui::Event::ArrowRight ||
      event == ftxui::Event::Character("l") ||
      event == ftxui::Event::Character("]")) {
    state_.next_effect(effect_count());
    return true;
  }

  if (event == ftxui::Event::ArrowLeft ||
      event == ftxui::Event::Character("h") ||
      event == ftxui::Event::Character("[")) {
    state_.previous_effect(effect_count());
    return true;
  }

  if (event == ftxui::Event::ArrowDown ||
      event == ftxui::Event::Character("j")) {
    state_.next_background(background_count());
    return true;
  }

  if (event == ftxui::Event::ArrowUp || event == ftxui::Event::Character("k")) {
    state_.previous_background(background_count());
    return true;
  }

  for (char digit = '1'; digit <= '6'; ++digit) {
    if (event == ftxui::Event::Character(std::string(1, digit))) {
      return state_.set_combatant_count_from_digit(digit);
    }
  }

  return false;
}

ftxui::Element EffectGalleryView::Render() {
  using namespace ftxui;

  ensure_sample_combatants();
  apply_selected_effect_to_samples();

  auto budget = current_battle_render_budget();
  budget.requested_height =
      std::max(1, budget.requested_height - kRootChromeHeight);

  const int visible = visible_combatant_count();
  const int columns = columns_for(budget, visible);
  const int rows = std::max(1, (visible + columns - 1) / columns);
  const int label_height = budget.profile == BattleLayoutProfile::Tiny ? 1 : 3;
  const int stage_height =
      std::max(kMinCombatantHeight, budget.requested_height - label_height - 1);
  const int cell_height = std::max(kMinCombatantHeight, stage_height / rows);
  const int cell_width = std::max(12, budget.requested_width / columns);

  Elements grid_rows;
  int next_sample = 0;
  for (int row = 0; row < rows; ++row) {
    Elements cells;
    for (int column = 0; column < columns; ++column) {
      if (next_sample < visible) {
        auto entity = samples_[static_cast<std::size_t>(next_sample++)];
        auto combatant = Combatant{registry_, entity, true}.Render() |
                         size(WIDTH, EQUAL, cell_width) |
                         size(HEIGHT, EQUAL, cell_height);
        cells.push_back(combatant);
      } else {
        cells.push_back(filler() | size(WIDTH, EQUAL, cell_width) |
                        size(HEIGHT, EQUAL, cell_height));
      }
    }
    grid_rows.push_back(hbox(std::move(cells)) |
                        size(HEIGHT, EQUAL, cell_height));
  }

  Element stage = vbox(std::move(grid_rows));
  if (budget.show_field_ambience) {
    stage = background_panel(background_at(state_.current_background_index),
                             std::move(stage), budget.requested_width,
                             stage_height, 0xEFFECA11u);
  }

  Elements content;
  content.push_back(
      label_row(state_.labels(), state_.background_labels(), budget.profile));
  if (budget.profile != BattleLayoutProfile::Tiny) {
    const auto chrome =
        fl::lospec500::on_not_black(fl::lospec500::color_at(24));
    content.push_back(
        text("Left/Right or h/l: effect   Up/Down or k/j: background   "
             "1-6: combatants   Esc: normal screen") |
        chrome);
  }
  content.push_back(stage | flex);

  return vbox(std::move(content)) | size(WIDTH, EQUAL, budget.requested_width) |
         size(HEIGHT, EQUAL, budget.requested_height);
}

const EffectGalleryState &EffectGalleryView::state() const noexcept {
  return state_;
}

void EffectGalleryView::ensure_sample_combatants() {
  using namespace fl::ecs::components;

  if (!samples_.empty()) {
    return;
  }

  samples_.reserve(kSampleNames.size());
  for (std::size_t i = 0; i < kSampleNames.size(); ++i) {
    auto entity = registry_.create();
    Stats stats{std::string(kSampleNames[i])};
    stats.max_hp_ = 80 + static_cast<int>(i) * 17;
    stats.hp_ = stats.max_hp_ - static_cast<int>(i) * 9;
    stats.max_mp_ = 24 + static_cast<int>(i) * 5;
    stats.mp_ = stats.max_mp_ / 2;
    registry_.emplace<Stats>(entity, std::move(stats));
    AtbCharge charge{};
    charge.charge = 1200 + static_cast<int64_t>(i) * 420;
    charge.max_charge = 4800;
    charge.charge_per_beat = 80;
    registry_.emplace<AtbCharge>(entity, charge);
    auto ctx = fl::context::EntityCtx{registry_, rng_, log_, entity};
    registry_.emplace<TrackXP>(entity, ctx, static_cast<int>(i) * 140);
    samples_.push_back(entity);
  }
}

void EffectGalleryView::apply_selected_effect_to_samples() {
  using namespace fl::ecs::components;

  const auto duration = std::chrono::milliseconds{kGalleryFrameMilliseconds};
  const auto started_at = looped_start_time(animation_started_at_, duration);
  const auto kind = effect_kind_at(state_.current_effect_index);

  for (std::size_t i = 0; i < samples_.size(); ++i) {
    const auto entity = samples_[i];
    registry_.remove<CombatantDecals>(entity);

    effects::DecalConfig config{};
    config.duration_seconds = static_cast<float>(duration.count()) / 1000.0F;
    config.seed = 0xA17CE5EDu ^ static_cast<std::uint32_t>(i * 2654435761u) ^
                  static_cast<std::uint32_t>(state_.current_effect_index);
    config.hitpoints = 42 + static_cast<int>(i) * 7;

    CombatantDecals decals;
    decals.effects.emplace_back(seerin::uWu{0}, started_at, duration, kind,
                                config, 2);
    registry_.emplace_or_replace<CombatantDecals>(entity, std::move(decals));
  }
}

int EffectGalleryView::visible_combatant_count() const noexcept {
  const auto budget = current_battle_render_budget();
  const int requested = std::clamp(state_.combatant_count, 1, 6);
  return std::clamp(requested, 1, max_visible_for(budget));
}

} // namespace fl::widgets
