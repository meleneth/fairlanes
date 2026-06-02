#include <catch2/catch_test_macros.hpp>

#include "fl/skills/skill.hpp"
#include "fl/skills/skill_definition.hpp"

namespace {

void require_tags(fl::skills::SkillKey skill,
                  std::initializer_list<fl::skills::SkillTag> expected) {
  for (const auto tag : expected) {
    REQUIRE(fl::skills::has_tag(skill, tag));
  }
}

} // namespace

TEST_CASE("Every defined skill has at least one tag", "[skills][tags]") {
  for (const auto *definition : fl::skills::all_definitions()) {
    CAPTURE(static_cast<int>(definition->key.base));
    CAPTURE(definition->key.rank.value());
    REQUIRE_FALSE(definition->tags.empty());
  }
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
