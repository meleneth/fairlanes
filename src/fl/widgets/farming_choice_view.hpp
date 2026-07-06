#pragma once

#include <cstddef>

#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "fl/primitives/farming_plan.hpp"

namespace fl::primitives {
struct PartyData;
}

namespace fl::widgets {

class FarmingChoiceState {
public:
  [[nodiscard]] std::size_t selected_index() const noexcept {
    return selected_index_;
  }
  [[nodiscard]] fl::primitives::FarmFocus selected_focus() const noexcept;

  void next() noexcept;
  void previous() noexcept;
  bool set_from_digit(char digit) noexcept;

private:
  std::size_t selected_index_{0};
};

class FarmingChoiceView : public ftxui::ComponentBase {
public:
  FarmingChoiceView(fl::primitives::PartyData &party,
                    fl::primitives::GrimoireDiscipline discipline);

  bool OnEvent(ftxui::Event event) override;
  ftxui::Element Render() override;

  [[nodiscard]] const FarmingChoiceState &state() const noexcept {
    return state_;
  }

private:
  void apply_selected_focus();

  fl::primitives::PartyData *party_{nullptr};
  fl::primitives::GrimoireDiscipline discipline_;
  FarmingChoiceState state_;
};

ftxui::Element
render_farming_plan_summary(const fl::primitives::FarmingPlan &plan, int width);

} // namespace fl::widgets
