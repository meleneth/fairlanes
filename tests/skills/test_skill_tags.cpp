#include <array>

#include <catch2/catch_test_macros.hpp>

#include "fl/monsters/monster_kind.hpp"
#include "fl/monsters/monster_skills.hpp"
#include "fl/skills/skill_definition.hpp"
#include "fl/skills/skill.hpp"

namespace {

void require_tags(fl::skills::SkillId skill,
                  std::initializer_list<fl::skills::SkillTag> expected) {
  for (const auto tag : expected) {
    REQUIRE(fl::skills::has_tag(skill, tag));
  }
}

} // namespace

TEST_CASE("Every defined skill has at least one tag", "[skills][tags]") {
  for (const auto *definition : fl::skills::all_definitions()) {
    CAPTURE(static_cast<int>(definition->kind));
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
               {fl::skills::SkillTag::Bleed,
                fl::skills::SkillTag::Slashing});

  require_tags(fl::skills::SkillId::FlameWave,
               {fl::skills::SkillTag::Fire, fl::skills::SkillTag::Area});

  require_tags(fl::skills::SkillId::Mercyburst,
               {fl::skills::SkillTag::Healing, fl::skills::SkillTag::Holy});

    require_tags(fl::skills::SkillId::Flee,
           {fl::skills::SkillTag::Escape, fl::skills::SkillTag::Utility});
}

TEST_CASE("Monster mappings still resolve to expected skills",
          "[skills][monsters]") {
  constexpr std::array<std::pair<fl::monster::MonsterKind, fl::skills::SkillId>,
             17>
      expected{ {
          {fl::monster::MonsterKind::FieldMouse, fl::skills::SkillId::Thump},
          {fl::monster::MonsterKind::HoneyBadger,
           fl::skills::SkillId::Eviscerate},
          {fl::monster::MonsterKind::BumpkinHare, fl::skills::SkillId::Bump},
      {fl::monster::MonsterKind::ScaredyCat, fl::skills::SkillId::Flee},
          {fl::monster::MonsterKind::MireSquish, fl::skills::SkillId::Squish},
          {fl::monster::MonsterKind::BarkSmack, fl::skills::SkillId::Smack},
          {fl::monster::MonsterKind::PoisonToad, fl::skills::SkillId::Poison},
          {fl::monster::MonsterKind::Yeti, fl::skills::SkillId::ColdSnap},
          {fl::monster::MonsterKind::Salamander,
           fl::skills::SkillId::FlameStrike},
          {fl::monster::MonsterKind::FireDrake,
           fl::skills::SkillId::FlameWave},
          {fl::monster::MonsterKind::StormtickImp,
           fl::skills::SkillId::Joltspasm},
          {fl::monster::MonsterKind::CeilingGrudge,
           fl::skills::SkillId::RocksFall},
          {fl::monster::MonsterKind::MiasmaToad,
           fl::skills::SkillId::SourBreath},
          {fl::monster::MonsterKind::ChoirWisp,
           fl::skills::SkillId::Mercyburst},
          {fl::monster::MonsterKind::GorecapSprout,
           fl::skills::SkillId::BloodBloom},
          {fl::monster::MonsterKind::RimefangHare,
           fl::skills::SkillId::IceSplitter},
          {fl::monster::MonsterKind::NullMote,
           fl::skills::SkillId::GravitySigh},
      } };

  for (const auto &[kind, skill] : expected) {
    REQUIRE(fl::monster::known_skill_for(kind) == skill);
  }
}
