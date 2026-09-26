#pragma once

#include <functional>

#include <entt/entity/registry.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "fl/primitives/party_data.hpp"

namespace fl::widgets {

class PlayerDetailsPane : public ftxui::ComponentBase {
public:
  using InventorySelected = std::function<void(entt::entity)>;

  PlayerDetailsPane(entt::registry &reg, fl::primitives::PartyData &party,
                    InventorySelected on_inventory_selected = {});

  bool OnEvent(ftxui::Event event) override;
  ftxui::Element Render() override;

  void set_focused(bool focused) noexcept;

private:
  entt::registry &reg_;
  fl::primitives::PartyData &party_;
  InventorySelected on_inventory_selected_;
  int cursor_{0};
  int selection_cursor_{0};
  int scroll_cursor_{0};
  bool focused_{false};
};

} // namespace fl::widgets
