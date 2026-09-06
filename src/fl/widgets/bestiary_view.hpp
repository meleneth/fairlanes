#pragma once

#include <ftxui/component/component.hpp>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "fl/primitives/discovery_journal.hpp"

namespace fl::widgets {

class BestiaryView : public ftxui::ComponentBase {
public:
  using OpenSkill =
      std::function<void(fl::monster::MonsterKind, fl::skills::SkillId)>;
  BestiaryView(const fl::primitives::DiscoveryJournal &journal, OpenSkill open,
               std::optional<fl::monster::MonsterKind> selected = std::nullopt);
  ftxui::Element Render() override;

private:
  void refresh();
  void refresh_skills();
  void open_skill();
  const fl::primitives::DiscoveryJournal &journal_;
  OpenSkill open_;
  std::vector<fl::monster::MonsterKind> monsters_;
  std::vector<std::string> names_;
  std::vector<std::optional<fl::skills::SkillId>> skills_;
  std::vector<std::string> skill_names_;
  int selected_{0};
  int selected_skill_{0};
  ftxui::Component monster_menu_;
  ftxui::Component skill_menu_;
};

} // namespace fl::widgets
