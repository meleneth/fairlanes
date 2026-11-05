#pragma once

#include <memory>

#include "fl/generated/ftxer_composition_root.hpp"
#include "fl/widgets/ftxer_ui_adapter.hpp"
#include "fl/widgets/root_component.hpp"

namespace fl::test {

class ProductionUiHarness {
public:
  ProductionUiHarness(fl::context::AccountCtx ctx,
                      std::deque<fl::primitives::AccountData>& accounts,
                      fl::widgets::FancyLog& console_log, fl::primitives::WorldClock& world_clock,
                      const fl::primitives::DiscoveryJournal* discoveries = nullptr,
                      fl::events::AchievementBus* achievements = nullptr,
                      fl::primitives::ObjectiveSystem* objectives = nullptr) {
    auto presentation = std::make_unique<fl::widgets::RootComponent>(
        std::move(ctx), accounts, console_log, world_clock, discoveries, achievements, objectives);
    presentation_ = presentation.get();
    auto adapter = std::make_unique<fl::widgets::FtxerUiAdapter>(std::move(presentation));
    component_ = ftxui::Make<fl::generated::ftxer::CompositionRoot>(std::move(adapter));
    composition_ = dynamic_cast<fl::generated::ftxer::CompositionRoot*>(component_.get());
  }

  bool OnEvent(ftxui::Event event) {
    return composition_->OnEvent(std::move(event));
  }
  ftxui::Element Render() {
    return composition_->Render();
  }
  bool quit_requested() const {
    return composition_->quit_requested();
  }
  bool sandbox_requested() const {
    return composition_->sandbox_requested();
  }
  fl::generated::ftxer::ScreenId current_screen() const {
    return composition_->current_screen();
  }
  std::size_t history_size() const {
    return composition_->history_size();
  }
  bool console_open() const {
    return composition_->console_open();
  }
  fl::widgets::RootComponent& presentation() {
    return *presentation_;
  }
  ftxui::Component component() const {
    return component_;
  }

private:
  ftxui::Component component_;
  fl::generated::ftxer::CompositionRoot* composition_{nullptr};
  fl::widgets::RootComponent* presentation_{nullptr};
};

} // namespace fl::test
