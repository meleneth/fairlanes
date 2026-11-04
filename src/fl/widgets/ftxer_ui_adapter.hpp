#pragma once

#include <memory>

#include <ftxui/component/component.hpp>

#include "fl/generated/ftxer_composition_root.hpp"

namespace fl::widgets {

class RootComponent;

class FtxerUiAdapter final : public fl::generated::ftxer::ExternalUiAdapter {
public:
  explicit FtxerUiAdapter(ftxui::Component root);

  ftxui::Component component() override;
  ftxui::Element render() override;
  bool on_event(const ftxui::Event &event) override;
  fl::generated::ftxer::ScreenId current_screen() const override;
  bool show_screen(fl::generated::ftxer::ScreenId screen) override;
  bool invoke(fl::generated::ftxer::ExternalAction action) override;
  bool console_open() const override;
  void toggle_console() override;
  void focus_console() override;
  void restore_focus() override;
  bool quit_requested() const override;
  bool sandbox_requested() const override;

private:
  RootComponent &root();
  const RootComponent &root() const;

  ftxui::Component root_;
};

} // namespace fl::widgets
