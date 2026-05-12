#pragma once

#include <cstdint>
#include <string_view>

#include "fl/primitives/world_clock.hpp"

namespace fl::primitives {

enum class MoonPhase {
  New,
  WaxingCrescent,
  FirstQuarter,
  WaxingGibbous,
  Full,
  WaningGibbous,
  LastQuarter,
  WaningCrescent,
};

struct MoonStatus {
  std::string_view name;
  int cycle_days{0};
  int day_in_cycle{0};
  MoonPhase phase{MoonPhase::New};
};

struct MoonCalendarSnapshot {
  std::uint64_t day{0};
  std::uint64_t beat_of_day{0};
  MoonStatus runner;
  MoonStatus elder;
  std::uint64_t days_until_runner_black{0};
  std::uint64_t days_until_elder_full{0};
  std::uint64_t days_until_split_light{0};
  std::uint64_t days_until_twin_dark{0};
  std::uint64_t days_until_visitor{0};
};

class MoonCalendar {
public:
  static constexpr int kRunnerCycleDays = 9;
  static constexpr int kElderCycleDays = 21;
  static constexpr int kVisitorCycleDays = 1323;
  // These named celestial events follow the lore cadence directly. They are
  // not recomputed from the simplified eight-phase display model.
  static constexpr int kSplitLightDays = 63;
  static constexpr int kTwinDarkDays = 189;

  [[nodiscard]] static MoonCalendarSnapshot
  snapshot(const WorldClock &clock) noexcept;

  [[nodiscard]] static MoonPhase
  phase_for_day(std::uint64_t day, int cycle_days,
                int phase_offset_eighths = 0) noexcept;
  [[nodiscard]] static std::string_view phase_name(MoonPhase phase) noexcept;

private:
  [[nodiscard]] static std::uint64_t days_until(std::uint64_t day,
                                                int cycle_days) noexcept;
};

} // namespace fl::primitives
