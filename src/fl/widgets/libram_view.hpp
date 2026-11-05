#pragma once

#include <ftxui/component/component.hpp>
#include <optional>
#include <string>
#include <vector>

#include "fl/primitives/discovery_journal.hpp"
#include "fl/primitives/party_data.hpp"

namespace fl::widgets {

class LibramView : public ftxui::ComponentBase {
public:
  explicit LibramView(const fl::primitives::DiscoveryJournal& journal,
                      std::optional<fl::skills::SkillId> selected = std::nullopt,
                      fl::primitives::PartyData* party = nullptr);
  bool OnEvent(ftxui::Event event) override;
  ftxui::Element Render() override;

private:
  void refresh();
  const fl::primitives::DiscoveryJournal& journal_;
  std::vector<fl::skills::SkillId> skills_;
  std::vector<std::string> names_;
  int selected_{0};
  ftxui::Component menu_;
  fl::primitives::PartyData* party_{nullptr};
  int member_{0};
  std::string action_status_;
};

} // namespace fl::widgets
