#include "fl/primitives/moon_calendar.hpp"

#include <cstdint>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("world clock advances calendar days at the documented beat cadence") {
  fl::primitives::WorldClock clock;

  REQUIRE(fl::primitives::WorldClock::kBeatsPerSecond == 12);
  REQUIRE(fl::primitives::WorldClock::beats_from_seconds(3) == 36);
  REQUIRE(fl::primitives::WorldClock::kBeatsPerCalendarDay == 720);
  REQUIRE(clock.beat_rate_multiplier() == 1);
  REQUIRE(clock.effective_beats_per_wall_second() == 12);
  REQUIRE(clock.calendar_day() == 0);

  clock.set_beat_rate_multiplier(4);
  REQUIRE(clock.beat_rate_multiplier() == 4);
  REQUIRE(clock.effective_beats_per_wall_second() == 48);

  for (std::uint64_t i = 0;
       i < fl::primitives::WorldClock::kBeatsPerCalendarDay; ++i) {
    clock.advance_beat();
  }

  REQUIRE(clock.calendar_day() == 1);
  REQUIRE(clock.beat_of_day() == 0);
}

TEST_CASE("moon calendar follows the lore cycle periods") {
  fl::primitives::WorldClock clock;

  auto snapshot = fl::primitives::MoonCalendar::snapshot(clock);
  REQUIRE(snapshot.day == 0);
  REQUIRE(snapshot.runner.day_in_cycle == 0);
  REQUIRE(snapshot.runner.phase == fl::primitives::MoonPhase::New);
  REQUIRE(snapshot.elder.day_in_cycle == 0);
  REQUIRE(snapshot.elder.phase == fl::primitives::MoonPhase::Full);
  REQUIRE(snapshot.days_until_runner_black == 0);
  REQUIRE(snapshot.days_until_elder_full == 0);
  REQUIRE(snapshot.days_until_split_light == 0);
  REQUIRE(snapshot.days_until_twin_dark == 0);
  REQUIRE(snapshot.days_until_visitor == 0);

  for (std::uint64_t i = 0;
       i < fl::primitives::WorldClock::kBeatsPerCalendarDay; ++i) {
    clock.advance_beat();
  }

  snapshot = fl::primitives::MoonCalendar::snapshot(clock);
  REQUIRE(snapshot.day == 1);
  REQUIRE(snapshot.runner.day_in_cycle == 1);
  REQUIRE(snapshot.elder.day_in_cycle == 1);
  REQUIRE(snapshot.days_until_runner_black == 8);
  REQUIRE(snapshot.days_until_elder_full == 20);
  REQUIRE(snapshot.days_until_split_light == 62);
  REQUIRE(snapshot.days_until_twin_dark == 188);
  REQUIRE(snapshot.days_until_visitor == 1322);
}

TEST_CASE("moon calendar repeats major celestial events") {
  fl::primitives::WorldClock clock;

  for (std::uint64_t i = 0;
       i < fl::primitives::WorldClock::kBeatsPerCalendarDay * 189; ++i) {
    clock.advance_beat();
  }

  const auto snapshot = fl::primitives::MoonCalendar::snapshot(clock);
  REQUIRE(snapshot.day == 189);
  REQUIRE(snapshot.days_until_runner_black == 0);
  REQUIRE(snapshot.days_until_elder_full == 0);
  REQUIRE(snapshot.days_until_split_light == 0);
  REQUIRE(snapshot.days_until_twin_dark == 0);
}
