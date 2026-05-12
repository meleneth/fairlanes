#include "fl/primitives/moon_calendar.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace fl::primitives {

MoonCalendarSnapshot MoonCalendar::snapshot(const WorldClock &clock) noexcept {
  const auto day = clock.calendar_day();
  const auto runner_day =
      static_cast<int>(day % static_cast<std::uint64_t>(kRunnerCycleDays));
  const auto elder_day =
      static_cast<int>(day % static_cast<std::uint64_t>(kElderCycleDays));

  return MoonCalendarSnapshot{
      .day = day,
      .beat_of_day = clock.beat_of_day(),
      .runner =
          MoonStatus{
              .name = "Runner",
              .cycle_days = kRunnerCycleDays,
              .day_in_cycle = runner_day,
              .phase = phase_for_day(day, kRunnerCycleDays),
          },
      .elder =
          MoonStatus{
              .name = "Elder",
              .cycle_days = kElderCycleDays,
              .day_in_cycle = elder_day,
              .phase = phase_for_day(day, kElderCycleDays, 4),
          },
      .days_until_runner_black = days_until(day, kRunnerCycleDays),
      .days_until_elder_full = days_until(day, kElderCycleDays),
      .days_until_split_light = days_until(day, kSplitLightDays),
      .days_until_twin_dark = days_until(day, kTwinDarkDays),
      .days_until_visitor = days_until(day, kVisitorCycleDays),
  };
}

MoonPhase MoonCalendar::phase_for_day(std::uint64_t day, int cycle_days,
                                      int phase_offset_eighths) noexcept {
  const auto cycle = static_cast<std::uint64_t>(cycle_days);
  const auto day_in_cycle = day % cycle;
  const auto phase_index =
      (static_cast<int>((day_in_cycle * 8U) / cycle) + phase_offset_eighths) %
      8;

  return static_cast<MoonPhase>(phase_index);
}

std::string_view MoonCalendar::phase_name(MoonPhase phase) noexcept {
  constexpr std::array<std::string_view, 8> kNames{
      "New",       "Waxing Crescent", "First Quarter", "Waxing Gibbous",
      "Full",      "Waning Gibbous",  "Last Quarter",  "Waning Crescent",
  };

  return kNames[static_cast<std::size_t>(phase)];
}

std::uint64_t MoonCalendar::days_until(std::uint64_t day,
                                       int cycle_days) noexcept {
  const auto cycle = static_cast<std::uint64_t>(cycle_days);
  return (cycle - (day % cycle)) % cycle;
}

} // namespace fl::primitives
