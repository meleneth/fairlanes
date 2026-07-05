#include "fl/skills/skill_definition.hpp"

#include <algorithm>
#include <cstdlib>
#include <span>
#include <string>

#include "fl/generated/skill_content.hpp"

namespace fl::skills {

namespace {

[[noreturn]] void abort_missing_definition(SkillKey skill) {
  (void)skill;
  std::abort();
}

} // namespace

const SkillDefinition &definition(SkillKey skill) noexcept {
  const auto all = all_definitions();
  const auto it = std::find_if(all.begin(), all.end(), [skill](auto *entry) {
    return entry->key.base == skill.base;
  });
  if (it == all.end()) {
    abort_missing_definition(skill);
  }
  return **it;
}

std::span<const SkillDefinition *const> all_definitions() noexcept {
  return generated_content::all_definitions();
}

std::string_view name(SkillKey skill) noexcept {
  return definition(skill).display_name;
}

std::string display_name(SkillKey skill) {
  auto display = std::string{name(skill)};
  if (skill.rank.value() != SkillRank::kMin) {
    display += " ";
    display += roman(skill.rank);
  }
  return display;
}

int learn_chance_percent(SkillKey skill) noexcept {
  return definition(skill).learn_chance_percent;
}

std::span<const SkillTag> tags(SkillKey skill) noexcept {
  return definition(skill).tags;
}

bool has_tag(SkillKey skill, SkillTag tag) noexcept {
  const auto tag_list = tags(skill);
  return std::find(tag_list.begin(), tag_list.end(), tag) != tag_list.end();
}

} // namespace fl::skills
