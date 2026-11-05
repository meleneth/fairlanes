#pragma once

#include <functional>
#include <optional>

#include <entt/entity/registry.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "fl/primitives/party_data.hpp"
#include "fl/skills/skill.hpp"

namespace fl::widgets {

class PlayerDetailsPane : public ftxui::ComponentBase {
public:
  using InventorySelected = std::function<void(entt::entity)>;
  using SkillSelected = std::function<void(std::optional<fl::skills::SkillKey>)>;

  PlayerDetailsPane(entt::registry& reg, fl::primitives::PartyData& party,
                    InventorySelected on_inventory_selected = {},
                    SkillSelected on_skill_selected = {});

  bool OnEvent(ftxui::Event event) override;
  ftxui::Element Render() override;

  void set_focused(bool focused);

private:
  entt::registry& reg_;
  fl::primitives::PartyData& party_;
  InventorySelected on_inventory_selected_;
  SkillSelected on_skill_selected_;
  int cursor_{0};
  int selection_cursor_{0};
  int scroll_cursor_{0};
  bool focused_{false};

  void notify_skill_selection();
};

} // namespace fl::widgets
