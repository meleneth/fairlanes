#pragma once

#include <functional>
#include <string_view>

#include <ftxui/component/component.hpp>

namespace fl::primitives { class ObjectiveSystem; }

namespace fl::widgets {

class StoryScreen : public ftxui::ComponentBase {
public:
  enum class Start { Objectives, Village };
  using EmitChoice = std::function<void(std::string_view)>;
  using Purchase = std::function<void()>;
  StoryScreen(const fl::primitives::ObjectiveSystem &objectives,
              EmitChoice emit_choice, Purchase purchase, Start start);
  bool OnEvent(ftxui::Event event) override;
  ftxui::Element Render() override;

private:
  enum class Mode { Objectives, Village, Pub, Dialogue };
  void open_dialogue(std::string_view id);
  const fl::primitives::ObjectiveSystem *objectives_;
  EmitChoice emit_choice_;
  Purchase purchase_;
  Mode mode_;
  std::string_view dialogue_id_{};
  std::string_view node_id_{};
  std::size_t selection_{0};
};

} // namespace fl::widgets
