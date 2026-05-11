// party_status.hpp
#pragma once
#include <entt/entt.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>

#include "fl/primitives/party_data.hpp"

namespace fl::widgets {

class PartyStatus : public ftxui::ComponentBase {
public:
  PartyStatus(fl::primitives::PartyData &party_data);
  ftxui::Element Render() override;

private:
  fl::primitives::PartyData &party_data_;
};

} // namespace fl::widgets
