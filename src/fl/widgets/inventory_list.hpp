#pragma once

#include <vector>

#include <entt/entity/registry.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "fl/loot/equipment_slot.hpp"
#include "fl/primitives/party_data.hpp"

namespace fl::widgets {

class InventoryList : public ftxui::ComponentBase {
public:
  InventoryList(entt::registry &reg, fl::primitives::PartyData &party);

  bool OnEvent(ftxui::Event event) override;
  ftxui::Element Render() override;

  void set_focused(bool focused) noexcept;
  void show_equipment_slots(entt::entity member) noexcept;

private:
  enum class Mode { party_inventory, equipment_slots, filtered_items };

  [[nodiscard]] std::vector<entt::entity> filtered_items() const;
  [[nodiscard]] fl::loot::EquipmentSlot selected_slot() const noexcept;
  [[nodiscard]] bool selected_member_is_valid() const;
  ftxui::Element render_party_inventory();
  ftxui::Element render_equipment_slots();
  ftxui::Element render_filtered_items();

  entt::registry &reg_;
  fl::primitives::PartyData &party_;
  Mode mode_{Mode::party_inventory};
  entt::entity selected_member_{entt::null};
  int item_cursor_{0};
  int slot_cursor_{0};
  bool focused_{false};
};

} // namespace fl::widgets
