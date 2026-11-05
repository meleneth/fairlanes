#include "fl/skills/grimoire.hpp"

#include <algorithm>

namespace fl::skills {

bool Grimoire::knows(SkillKey skill) const noexcept {
  return std::find(known_.begin(), known_.end(), skill) != known_.end();
}

bool Grimoire::learn(SkillKey skill) {
  if (knows(skill)) {
    return false;
  }

  known_.push_back(skill);
  if (!previously_learned(skill)) learned_history_.push_back(skill);
  return true;
}

bool Grimoire::previously_learned(SkillKey skill) const noexcept {
  return std::find(learned_history_.begin(), learned_history_.end(), skill) !=
         learned_history_.end();
}

bool Grimoire::unlearn(SkillKey skill) {
  const auto it = std::find(known_.begin(), known_.end(), skill);
  if (it == known_.end()) {
    return false;
  }

  known_.erase(it);
  return true;
}

} // namespace fl::skills
