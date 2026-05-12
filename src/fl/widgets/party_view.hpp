#pragma once

#include <cstddef>

#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "fl/context.hpp"

namespace fl::widgets {

class PartyView : public ftxui::ComponentBase {
public:
  PartyView(fl::context::AccountCtx context, std::size_t party_index);

  bool OnEvent(ftxui::Event event) override;
  ftxui::Element Render() override;

private:
  ftxui::Element render_party();
  ftxui::Element render_inventory();

  fl::context::AccountCtx ctx_;
  std::size_t party_index_{0};
  int inventory_cursor_{0};
};

} // namespace fl::widgets
