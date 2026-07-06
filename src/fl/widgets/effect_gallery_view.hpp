#pragma once

#include <chrono>
#include <cstddef>
#include <deque>
#include <string_view>
#include <vector>

#include <entt/entt.hpp>
#include <ftxui/component/component.hpp>

#include "fl/primitives/random_hub.hpp"
#include "fl/widgets/effects/decal.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::widgets {

struct EffectGalleryLabels {
  std::string_view previous;
  std::string_view current;
  std::string_view next;
};

struct EffectGalleryBackgroundLabels {
  std::string_view previous;
  std::string_view current;
  std::string_view next;
};

struct EffectGalleryState {
  std::size_t current_effect_index{0};
  std::size_t current_background_index{0};
  int combatant_count{3};

  void next_effect(std::size_t effect_count) noexcept;
  void previous_effect(std::size_t effect_count) noexcept;
  void next_background(std::size_t background_count) noexcept;
  void previous_background(std::size_t background_count) noexcept;
  bool set_combatant_count_from_digit(char digit) noexcept;
  [[nodiscard]] EffectGalleryLabels labels() const noexcept;
  [[nodiscard]] EffectGalleryBackgroundLabels
  background_labels() const noexcept;
};

[[nodiscard]] std::string_view
current_effect_name(const EffectGalleryState &state) noexcept;

class EffectGalleryView : public ftxui::ComponentBase {
public:
  EffectGalleryView();

  bool OnEvent(ftxui::Event event) override;
  ftxui::Element Render() override;

  [[nodiscard]] const EffectGalleryState &state() const noexcept;

private:
  void ensure_sample_combatants();
  void apply_selected_effect_to_samples();
  [[nodiscard]] int visible_combatant_count() const noexcept;

  EffectGalleryState state_;
  entt::registry registry_;
  fl::primitives::RandomHub rng_{fl::primitives::RandomSeedConfig{
      0xEFFECA11E7ULL, fl::primitives::RandomSeedSource::Explicit}};
  FancyLog log_{FancyLog::Options{1, 8, true}};
  std::vector<entt::entity> samples_;
  std::chrono::steady_clock::time_point animation_started_at_{
      std::chrono::steady_clock::now()};
};

} // namespace fl::widgets
