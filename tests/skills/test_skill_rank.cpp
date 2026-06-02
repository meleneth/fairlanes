#include <catch2/catch_test_macros.hpp>

#include "fl/skills/skill.hpp"

TEST_CASE("SkillRank accepts only ranks one through nine", "[skills][rank]") {
  REQUIRE(fl::skills::SkillRank::is_valid(1));
  REQUIRE(fl::skills::SkillRank::is_valid(9));
  REQUIRE_FALSE(fl::skills::SkillRank::is_valid(0));
  REQUIRE_FALSE(fl::skills::SkillRank::is_valid(10));

  REQUIRE(fl::skills::SkillRank::from_int(5).has_value());
  REQUIRE_FALSE(fl::skills::SkillRank::from_int(0).has_value());
  REQUIRE_FALSE(fl::skills::SkillRank::from_int(10).has_value());
}

TEST_CASE("SkillRank displays ranks as roman numerals", "[skills][rank]") {
  REQUIRE(fl::skills::roman(fl::skills::SkillRank::require(1)) == "I");
  REQUIRE(fl::skills::roman(fl::skills::SkillRank::require(5)) == "V");
  REQUIRE(fl::skills::roman(fl::skills::SkillRank::require(9)) == "IX");
}

TEST_CASE("SkillKey distinguishes different ranks of the same skill",
          "[skills][rank]") {
  const fl::skills::SkillKey thump_i{fl::skills::SkillId::Thump};
  const fl::skills::SkillKey thump_ii{fl::skills::SkillId::Thump,
                                      fl::skills::SkillRank::require(2)};
  const fl::skills::SkillKey observe_i{fl::skills::SkillId::Observe};

  REQUIRE(thump_i != thump_ii);
  REQUIRE(thump_i != observe_i);
  REQUIRE(thump_i == fl::skills::SkillKey{fl::skills::SkillId::Thump});
}

TEST_CASE("Skill display names include non-default rank numerals",
          "[skills][rank]") {
  REQUIRE(fl::skills::display_name(
              fl::skills::SkillKey{fl::skills::SkillId::Thump}) == "Thump");
  REQUIRE(fl::skills::display_name(fl::skills::SkillKey{
              fl::skills::SkillId::Thump, fl::skills::SkillRank::require(3)}) ==
          "Thump III");
  REQUIRE(fl::skills::display_name(fl::skills::SkillKey{
              fl::skills::SkillId::Observe,
              fl::skills::SkillRank::require(5)}) == "Observe V");
}
