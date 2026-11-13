#pragma once

#include <cstddef>

#include <ftxui/component/component_base.hpp>

#include "fl/context.hpp"

namespace fl::widgets {

class ProductionView : public ftxui::ComponentBase {
public:
  ProductionView(fl::context::AccountCtx ctx, std::size_t party_index);
  bool OnEvent(ftxui::Event event) override;
  ftxui::Element Render() override;

private:
  std::size_t option_count() const;
  void apply();

  fl::context::AccountCtx ctx_;
  std::size_t party_index_{0};
  std::size_t mode_{0};
  std::size_t option_{0};
};

} // namespace fl::widgets
