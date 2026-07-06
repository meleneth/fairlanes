#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <entt/entity/fwd.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>

#include "fl/context.hpp"

namespace fl::primitives {
struct EncounterData;
}

namespace fl::widgets {
using fl::context::AccountCtx;

class FarmingChoiceView;

class PartyBattleScreen : public ftxui::ComponentBase {
public:
  PartyBattleScreen(AccountCtx context, std::size_t party_index);
  bool OnEvent(ftxui::Event event) override;
  ftxui::Element Render() override;
  ~PartyBattleScreen() override = default;

private:
  ftxui::Element render_combatant_cell(entt::entity entity, int width,
                                       int height);
  ftxui::Element
  render_combatant_grid(const std::vector<entt::entity> &entities,
                        int cell_width, int cell_height, int grid_width,
                        int grid_height);
  ftxui::Element render_bottom_panel(int screen_width, int height,
                                     bool show_auxiliary_battle_panel);
  ftxui::Element render_account_log_panel(int width, int height);
  ftxui::Element render_party_log_panel(int width, int height);
  ftxui::Element render_question_panel(int width, int height) const;
  void update_stage_background(const fl::primitives::EncounterData *encounter);
  ftxui::Element render_stage_background(ftxui::Element foreground, int width,
                                         int height, std::uint32_t seed) const;

  AccountCtx ctx_;
  std::size_t party_index_{0};
  int focused_log_{0};
  std::shared_ptr<FarmingChoiceView> farming_choice_;
  std::size_t farming_choice_party_index_{static_cast<std::size_t>(-1)};
  const fl::primitives::EncounterData *last_encounter_{nullptr};
  std::size_t next_background_index_{0};
  std::size_t stage_background_index_{0};
};

} // namespace fl::widgets
