#include "fl/skills/skill_definition.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <span>

namespace fl::skills {
const SkillDefinition &observe_skill_definition() noexcept;
const SkillDefinition &flee_skill_definition() noexcept;
const SkillDefinition &thump_skill_definition() noexcept;
const SkillDefinition &eviscerate_skill_definition() noexcept;
const SkillDefinition &poison_skill_definition() noexcept;
const SkillDefinition &cold_snap_skill_definition() noexcept;
const SkillDefinition &flame_strike_skill_definition() noexcept;
const SkillDefinition &flame_wave_skill_definition() noexcept;
const SkillDefinition &joltspasm_skill_definition() noexcept;
const SkillDefinition &rocks_fall_skill_definition() noexcept;
const SkillDefinition &sour_breath_skill_definition() noexcept;
const SkillDefinition &mercyburst_skill_definition() noexcept;
const SkillDefinition &blood_bloom_skill_definition() noexcept;
const SkillDefinition &ice_splitter_skill_definition() noexcept;
const SkillDefinition &gravity_sigh_skill_definition() noexcept;
const SkillDefinition &bump_skill_definition() noexcept;
const SkillDefinition &squish_skill_definition() noexcept;
const SkillDefinition &smack_skill_definition() noexcept;

namespace {

std::array<const SkillDefinition *, 18> &definitions() {
  static std::array<const SkillDefinition *, 18> data{
      &observe_skill_definition(),      &flee_skill_definition(),
      &thump_skill_definition(),        &eviscerate_skill_definition(),
      &poison_skill_definition(),       &cold_snap_skill_definition(),
      &flame_strike_skill_definition(), &flame_wave_skill_definition(),
      &joltspasm_skill_definition(),    &rocks_fall_skill_definition(),
      &sour_breath_skill_definition(),  &mercyburst_skill_definition(),
      &blood_bloom_skill_definition(),  &ice_splitter_skill_definition(),
      &gravity_sigh_skill_definition(), &bump_skill_definition(),
      &squish_skill_definition(),       &smack_skill_definition(),
  };
  return data;
}

[[noreturn]] void abort_missing_definition(SkillKey skill) {
  (void)skill;
  std::abort();
}

} // namespace

const SkillDefinition &definition(SkillKey skill) noexcept {
  const auto &all = definitions();
  const auto it = std::find_if(all.begin(), all.end(), [skill](auto *entry) {
    return entry->key == skill;
  });
  if (it == all.end()) {
    abort_missing_definition(skill);
  }
  return **it;
}

std::span<const SkillDefinition *const> all_definitions() noexcept {
  const auto &all = definitions();
  return {all.data(), all.size()};
}

std::string_view name(SkillKey skill) noexcept {
  return definition(skill).display_name;
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
