// combatant_component.hpp
#pragma once
#include <entt/entt.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>

namespace fl::widgets {

class Combatant : public ftxui::ComponentBase {
public:
  Combatant(entt::registry &reg_, entt::entity entity_);
  ftxui::Element Render() override;

private:
  entt::registry &reg;
  entt::entity entity;
};

} // namespace fl::widgets
