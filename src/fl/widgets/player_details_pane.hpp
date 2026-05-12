#pragma once

#include <entt/entt.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "fl/primitives/party_data.hpp"

namespace fl::widgets {

class PlayerDetailsPane : public ftxui::ComponentBase {
public:
  PlayerDetailsPane(entt::registry &reg, fl::primitives::PartyData &party);

  bool OnEvent(ftxui::Event event) override;
  ftxui::Element Render() override;

  void set_focused(bool focused) noexcept;

private:
  entt::registry &reg_;
  fl::primitives::PartyData &party_;
  int cursor_{0};
  bool focused_{false};
};

} // namespace fl::widgets
