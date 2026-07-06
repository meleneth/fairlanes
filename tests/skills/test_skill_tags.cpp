#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <type_traits>
#include <vector>

#include "fl/skills/skill.hpp"
#include "fl/skills/skill_definition.hpp"

namespace {

using fl::skills::SkillDefinition;
using fl::skills::SkillExecutionKind;
using fl::skills::SkillId;
using fl::skills::SkillKey;
using fl::skills::SkillTag;

void require_tags(fl::skills::SkillKey skill,
                  std::initializer_list<fl::skills::SkillTag> expected) {
  for (const auto tag : expected) {
    REQUIRE(fl::skills::has_tag(skill, tag));
  }
}

bool has_tag(const SkillDefinition &definition, SkillTag tag) {
  return std::find(definition.tags.begin(), definition.tags.end(), tag) !=
         definition.tags.end();
}

bool has_any_tag(const SkillDefinition &definition,
                 std::initializer_list<SkillTag> tags) {
  return std::ranges::any_of(
      tags, [&definition](const auto tag) { return has_tag(definition, tag); });
}

bool has_only_tags(const SkillDefinition &definition,
                   std::initializer_list<SkillTag> tags) {
  return std::ranges::all_of(definition.tags, [tags](const auto tag) {
    return std::ranges::find(tags, tag) != tags.end();
  });
}

bool is_random_combat_skill(SkillId skill) {
  const auto random_skills = fl::skills::random_combat_skills();
  return std::ranges::any_of(random_skills, [skill](const auto candidate) {
    return candidate.base == skill;
  });
}

bool has_effect_kind_tag(const SkillDefinition &definition) {
  if (definition.execution == SkillExecutionKind::Flee) {
    return has_tag(definition, SkillTag::Escape);
  }

  return has_any_tag(definition,
                     {SkillTag::Damage, SkillTag::Heal, SkillTag::Buff,
                      SkillTag::Healing, SkillTag::Debuff, SkillTag::Cleanse,
                      SkillTag::Drain, SkillTag::Execute, SkillTag::Observe});
}

bool needs_target_tag(const SkillDefinition &definition) {
  return definition.execution != SkillExecutionKind::Observe &&
         definition.execution != SkillExecutionKind::Flee;
}

bool has_target_tag(const SkillDefinition &definition) {
  return has_any_tag(definition,
                     {SkillTag::Self, SkillTag::Enemy, SkillTag::RandomEnemy,
                      SkillTag::Group, SkillTag::AllEnemies, SkillTag::Ally,
                      SkillTag::AllAllies});
}

bool has_future_mechanics_tag(const SkillDefinition &definition) {
  return has_any_tag(definition,
                     {SkillTag::Buff, SkillTag::Debuff, SkillTag::Cleanse,
                      SkillTag::Dispel, SkillTag::PoisonStatus,
                      SkillTag::BurnStatus, SkillTag::BleedStatus,
                      SkillTag::SlowStatus, SkillTag::StunStatus,
                      SkillTag::BlindStatus, SkillTag::SilenceStatus,
                      SkillTag::FearStatus, SkillTag::RegenStatus,
                      SkillTag::ShieldStatus, SkillTag::TauntStatus,
                      SkillTag::VulnerableStatus, SkillTag::HasteStatus,
                      SkillTag::ReflectStatus});
}

} // namespace

TEST_CASE("Every defined skill has at least one tag", "[skills][tags]") {
  for (const auto *definition : fl::skills::all_definitions()) {
    CAPTURE(static_cast<int>(definition->key.base));
    CAPTURE(definition->key.rank.value());
    REQUIRE_FALSE(definition->tags.empty());
  }
}

TEST_CASE("Every generated SkillId has exactly one definition",
          "[skills][definitions]") {
  std::vector<SkillId> seen;

  for (const auto *definition : fl::skills::all_definitions()) {
    REQUIRE(definition != nullptr);
    CAPTURE(static_cast<int>(definition->key.base));
    REQUIRE(fl::skills::has_definition(definition->key));
    REQUIRE(std::ranges::find(seen, definition->key.base) == seen.end());
    seen.push_back(definition->key.base);
  }

  REQUIRE(seen.size() == fl::skills::all_definitions().size());
}

TEST_CASE("Every SkillDefinition has a non-empty display name",
          "[skills][definitions]") {
  for (const auto *definition : fl::skills::all_definitions()) {
    CAPTURE(static_cast<int>(definition->key.base));
    REQUIRE_FALSE(definition->display_name.empty());
    REQUIRE_FALSE(fl::skills::display_name(definition->key).empty());
    REQUIRE(fl::skills::display_name(definition->key).find("Unknown Skill") ==
            std::string::npos);
  }
}

TEST_CASE("Random combat skills all resolve to generated definitions",
          "[skills][definitions]") {
  for (const auto skill : fl::skills::random_combat_skills()) {
    CAPTURE(static_cast<int>(skill.base));
    REQUIRE(fl::skills::has_definition(skill));
    REQUIRE(fl::skills::definition(skill).execution !=
            SkillExecutionKind::Observe);
  }
}

TEST_CASE("Skill metadata preserves effect and targeting invariants",
          "[skills][tags][definitions]") {
  for (const auto *definition : fl::skills::all_definitions()) {
    CAPTURE(static_cast<int>(definition->key.base));
    CAPTURE(definition->display_name);
    CAPTURE(static_cast<int>(definition->execution));

    REQUIRE(has_effect_kind_tag(*definition));
    if (needs_target_tag(*definition)) {
      REQUIRE(has_target_tag(*definition));
    }
  }
}

TEST_CASE("PlaceholderEffect skills name their future mechanics",
          "[skills][tags][definitions]") {
  for (const auto *definition : fl::skills::all_definitions()) {
    if (definition->execution != SkillExecutionKind::PlaceholderEffect) {
      continue;
    }

    CAPTURE(static_cast<int>(definition->key.base));
    CAPTURE(definition->display_name);
    REQUIRE(has_future_mechanics_tag(*definition));
  }
}

TEST_CASE("Skill execution kinds match their required tags",
          "[skills][tags][definitions]") {
  for (const auto *definition : fl::skills::all_definitions()) {
    CAPTURE(static_cast<int>(definition->key.base));
    CAPTURE(definition->display_name);
    CAPTURE(static_cast<int>(definition->execution));

    switch (definition->execution) {
    case SkillExecutionKind::ThumpLike:
    case SkillExecutionKind::Eviscerate:
    case SkillExecutionKind::ColdSnap:
    case SkillExecutionKind::FlameStrike:
    case SkillExecutionKind::FlameWave:
    case SkillExecutionKind::DamageStrike:
      REQUIRE(has_any_tag(*definition, {SkillTag::Damage, SkillTag::Execute}));
      break;
    case SkillExecutionKind::Poison:
      REQUIRE(has_tag(*definition, SkillTag::Debuff));
      REQUIRE(has_tag(*definition, SkillTag::PoisonStatus));
      break;
    case SkillExecutionKind::DecalStrike:
      REQUIRE(has_any_tag(*definition, {SkillTag::Damage, SkillTag::Heal,
                                        SkillTag::Healing}));
      break;
    case SkillExecutionKind::SingleHeal:
      REQUIRE(has_tag(*definition, SkillTag::Heal));
      break;
    case SkillExecutionKind::GroupDamage:
      REQUIRE(has_any_tag(*definition, {SkillTag::Damage, SkillTag::Execute}));
      REQUIRE(has_any_tag(*definition, {SkillTag::Group, SkillTag::AllEnemies}));
      break;
    case SkillExecutionKind::GroupHeal:
      REQUIRE(has_tag(*definition, SkillTag::Heal));
      REQUIRE(has_any_tag(*definition, {SkillTag::AllAllies, SkillTag::Group}));
      break;
    case SkillExecutionKind::PlaceholderEffect:
    case SkillExecutionKind::Flee:
    case SkillExecutionKind::Observe:
      break;
    }
  }
}

TEST_CASE("Flee keeps only flee metadata and chance",
          "[skills][tags][definitions]") {
  const auto &flee = fl::skills::definition(SkillId::Flee);

  REQUIRE(flee.execution == SkillExecutionKind::Flee);
  REQUIRE(flee.flee_success_percent == 65);
  REQUIRE(flee.learn_chance_percent == 0);
  REQUIRE(has_only_tags(flee, {SkillTag::Utility, SkillTag::Escape}));
}

TEST_CASE("Observe is not randomly rolled or normally learnable",
          "[skills][tags][definitions]") {
  const auto &observe = fl::skills::definition(SkillId::Observe);

  REQUIRE(observe.execution == SkillExecutionKind::Observe);
  REQUIRE(observe.learn_chance_percent == 0);
  REQUIRE_FALSE(is_random_combat_skill(SkillId::Observe));
}

TEST_CASE("Thump and Flame Wave are distinct definitions",
          "[skills][definitions]") {
  const auto &thump = fl::skills::definition(fl::skills::SkillId::Thump);
  const auto &flame_wave =
      fl::skills::definition(fl::skills::SkillId::FlameWave);

  REQUIRE(&thump != &flame_wave);
}

TEST_CASE("Representative skills expose expected tags", "[skills][tags]") {
  require_tags(fl::skills::SkillId::Eviscerate,
               {fl::skills::SkillTag::Bleed, fl::skills::SkillTag::Slashing});

  require_tags(fl::skills::SkillId::FlameWave,
               {fl::skills::SkillTag::Fire, fl::skills::SkillTag::Area});

  require_tags(fl::skills::SkillId::Mercyburst,
               {fl::skills::SkillTag::Healing, fl::skills::SkillTag::Holy});

  require_tags(fl::skills::SkillId::Flee,
               {fl::skills::SkillTag::Escape, fl::skills::SkillTag::Utility});
}
