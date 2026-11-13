#pragma once

#include <algorithm>
#include <vector>

#include "fl/skills/skill.hpp"

namespace fl::ecs::components {

struct SkillStudyEntry {
  fl::skills::SkillKey skill{fl::skills::SkillId::Observe};
  int progress_beats{0};
};

struct SkillStudy {
  std::vector<SkillStudyEntry> eligible;

  SkillStudyEntry* find(fl::skills::SkillKey skill) noexcept {
    const auto found = std::ranges::find(eligible, skill, &SkillStudyEntry::skill);
    return found == eligible.end() ? nullptr : &*found;
  }
  const SkillStudyEntry* find(fl::skills::SkillKey skill) const noexcept {
    const auto found = std::ranges::find(eligible, skill, &SkillStudyEntry::skill);
    return found == eligible.end() ? nullptr : &*found;
  }
  bool discover(fl::skills::SkillKey skill) {
    if (find(skill)) return false;
    eligible.push_back({skill, 0});
    return true;
  }
  void complete(fl::skills::SkillKey skill) {
    std::erase_if(eligible, [skill](const auto& entry) { return entry.skill == skill; });
  }
};

} // namespace fl::ecs::components
