#pragma once

#include <cstddef>
#include <memory>

#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "fl/context.hpp"

namespace fl::widgets {

class InventoryList;
class PlayerDetailsPane;

class PartyView : public ftxui::ComponentBase {
public:
  PartyView(fl::context::AccountCtx context, std::size_t party_index);

  bool OnEvent(ftxui::Event event) override;
  ftxui::Element Render() override;

private:
  enum class FocusPane { inventory, player_details, party_log };

  ftxui::Element render_party();
  void set_focus(FocusPane focus);

  fl::context::AccountCtx ctx_;
  std::size_t party_index_{0};
  std::shared_ptr<InventoryList> inventory_list_;
  std::shared_ptr<PlayerDetailsPane> player_details_;
  FocusPane focus_{FocusPane::inventory};
};

} // namespace fl::widgets
