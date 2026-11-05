#pragma once

#include <functional>
#include <string_view>

#include <ftxui/component/component.hpp>

namespace fl::primitives {
class ObjectiveSystem;
}
namespace fl::primitives {
struct PartyData;
}

namespace fl::widgets {

class StoryScreen : public ftxui::ComponentBase {
public:
  enum class Start { Objectives, Village };
  using EmitChoice = std::function<void(std::string_view)>;
  using Purchase = std::function<void()>;
  using SelectTower = std::function<bool(std::string_view, int)>;
  using Recruit = std::function<void(std::string_view)>;
  StoryScreen(const fl::primitives::ObjectiveSystem& objectives,
              const fl::primitives::PartyData& party, EmitChoice emit_choice, Purchase purchase,
              SelectTower select_tower, Recruit recruit, Start start);
  bool OnEvent(ftxui::Event event) override;
  ftxui::Element Render() override;

private:
  enum class Mode { Objectives, Village, Pub, Outfitter, Biomes, Floors, Recruitment, Dialogue };
  void open_dialogue(std::string_view id);
  std::size_t biome_count() const noexcept;
  std::string_view selected_biome_id() const noexcept;
  const fl::primitives::ObjectiveSystem* objectives_;
  const fl::primitives::PartyData* party_;
  EmitChoice emit_choice_;
  Purchase purchase_;
  SelectTower select_tower_;
  Recruit recruit_;
  Mode mode_;
  std::string_view dialogue_id_{};
  std::string_view node_id_{};
  std::size_t selection_{0};
  std::size_t biome_selection_{0};
};

} // namespace fl::widgets
