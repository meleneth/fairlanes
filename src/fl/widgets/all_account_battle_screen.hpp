#pragma once

#include <deque>
#include <span>

#include <entt/entt.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "fl/primitives/account_data.hpp"

namespace fl::widgets {

class AllAccountBattleScreen : public ftxui::ComponentBase {
public:
  AllAccountBattleScreen(entt::registry &registry,
                         std::deque<fl::primitives::AccountData> &accounts);

  bool OnEvent(ftxui::Event event) override;
  ftxui::Element Render() override;

  [[nodiscard]] std::size_t selected_account_index() const noexcept {
    return selected_account_index_;
  }
  [[nodiscard]] std::size_t selected_party_index() const noexcept {
    return selected_party_index_;
  }

private:
  void select_relative(int delta);
  [[nodiscard]] std::size_t total_party_count() const noexcept;
  [[nodiscard]] std::size_t selected_flat_index() const noexcept;
  void select_flat_index(std::size_t flat_index) noexcept;

  ftxui::Element render_party_row(std::size_t account_index,
                                  std::size_t party_index,
                                  fl::primitives::PartyData &party,
                                  int width, bool selected) const;
  ftxui::Element render_party_overview(int width, int height);
  ftxui::Element render_selected_party_log(int width, int height);
  ftxui::Element render_selected_party_detail(int width, int height);
  ftxui::Element render_battle_summary(fl::primitives::PartyData &party,
                                       int width) const;
  ftxui::Element render_selected_party_battle(
      fl::primitives::PartyData &party, int width, int height) const;
  ftxui::Element render_combatant_row(std::span<const entt::entity> entities,
                                      int width, int height) const;
  ftxui::Element render_roster(std::span<const entt::entity> entities,
                               int width, bool show_levels = false) const;
  ftxui::Element render_member_roster(
      const std::deque<fl::primitives::MemberData> &members, int width) const;
  std::string entity_level_chip(entt::entity entity,
                                std::size_t max_name_width) const;
  std::string entity_chip(entt::entity entity,
                          std::size_t max_name_width) const;

  entt::registry *registry_{nullptr};
  std::deque<fl::primitives::AccountData> *accounts_{nullptr};
  std::size_t selected_account_index_{0};
  std::size_t selected_party_index_{0};
};

} // namespace fl::widgets
