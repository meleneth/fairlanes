#include <catch2/catch_test_macros.hpp>

#include "fl/skills/grimoire.hpp"

TEST_CASE("Grimoire learns exact ranked skills", "[skills][grimoire]") {
  fl::skills::Grimoire grimoire;
  const fl::skills::SkillKey thump_viii{fl::skills::SkillId::Thump,
                                        fl::skills::SkillRank::require(8)};

  REQUIRE_FALSE(grimoire.knows(thump_viii));
  REQUIRE(grimoire.learn(thump_viii));
  REQUIRE(grimoire.knows(thump_viii));
}

TEST_CASE("Grimoire learning is idempotent", "[skills][grimoire]") {
  fl::skills::Grimoire grimoire;
  const fl::skills::SkillKey thump_i{fl::skills::SkillId::Thump};

  REQUIRE(grimoire.learn(thump_i));
  REQUIRE_FALSE(grimoire.learn(thump_i));
  REQUIRE(grimoire.known_skills().size() == 1);
}

TEST_CASE("Grimoire does not infer lower ranks from higher ranks",
          "[skills][grimoire]") {
  fl::skills::Grimoire grimoire;
  const fl::skills::SkillKey thump_i{fl::skills::SkillId::Thump};
  const fl::skills::SkillKey thump_viii{fl::skills::SkillId::Thump,
                                        fl::skills::SkillRank::require(8)};

  REQUIRE(grimoire.learn(thump_viii));
  REQUIRE(grimoire.knows(thump_viii));
  REQUIRE_FALSE(grimoire.knows(thump_i));
}

TEST_CASE("Grimoire unlearns exact ranked skills", "[skills][grimoire]") {
  fl::skills::Grimoire grimoire;
  const fl::skills::SkillKey thump_i{fl::skills::SkillId::Thump};
  const fl::skills::SkillKey thump_viii{fl::skills::SkillId::Thump,
                                        fl::skills::SkillRank::require(8)};

  REQUIRE(grimoire.learn(thump_i));
  REQUIRE(grimoire.learn(thump_viii));
  REQUIRE(grimoire.unlearn(thump_viii));

  REQUIRE(grimoire.knows(thump_i));
  REQUIRE_FALSE(grimoire.knows(thump_viii));
  REQUIRE_FALSE(grimoire.unlearn(thump_viii));
}
