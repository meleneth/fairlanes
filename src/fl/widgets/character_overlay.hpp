#pragma once

#include <cstddef>
#include <string>

#include <ftxui/component/component.hpp>

#include "fl/context.hpp"

namespace fl::widgets {

class CharacterOverlay : public ftxui::ComponentBase {
public:
  CharacterOverlay(fl::context::AccountCtx ctx, std::size_t party_index);
  bool OnEvent(ftxui::Event event) override;
  ftxui::Element Render() override;

private:
  fl::context::AccountCtx ctx_;
  std::size_t party_index_{0};
  std::size_t member_index_{0};
  std::string message_;
};

} // namespace fl::widgets
