#pragma once

#include <span>
#include <vector>

#include "fl/skills/skill.hpp"

namespace fl::skills {

class Grimoire {
public:
  [[nodiscard]] bool knows(SkillKey skill) const noexcept;
  [[nodiscard]] bool knows(SkillId base,
                           SkillRank rank = SkillRank{}) const noexcept {
    return knows(SkillKey{base, rank});
  }

  bool learn(SkillKey skill);
  bool learn(SkillId base, SkillRank rank = SkillRank{}) {
    return learn(SkillKey{base, rank});
  }

  bool unlearn(SkillKey skill);
  bool unlearn(SkillId base, SkillRank rank = SkillRank{}) {
    return unlearn(SkillKey{base, rank});
  }

  [[nodiscard]] std::span<const SkillKey> known_skills() const noexcept {
    return known_;
  }

private:
  std::vector<SkillKey> known_;
};

} // namespace fl::skills
