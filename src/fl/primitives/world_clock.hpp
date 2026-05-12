#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>

#include "sr/uWu.hpp"

namespace fl::primitives {

class WorldClock {
public:
  static constexpr int kBeatsPerSecond = seerin::BEATS_PER_SEC;
  static constexpr int kSecondsPerCalendarDay = 60;
  static constexpr std::uint64_t kBeatsPerCalendarDay =
      static_cast<std::uint64_t>(kBeatsPerSecond * kSecondsPerCalendarDay);
  static constexpr int kMaxBeatRateMultiplier = 8;

  [[nodiscard]] static constexpr int beats_from_seconds(int seconds) noexcept {
    return seconds * kBeatsPerSecond;
  }

  void advance_beat() noexcept { ++elapsed_beats_; }

  void set_beat_rate_multiplier(int multiplier) noexcept {
    beat_rate_multiplier_.store(
        std::clamp(multiplier, 1, kMaxBeatRateMultiplier),
        std::memory_order_relaxed);
  }

  [[nodiscard]] int beat_rate_multiplier() const noexcept {
    return beat_rate_multiplier_.load(std::memory_order_relaxed);
  }

  [[nodiscard]] int effective_beats_per_wall_second() const noexcept {
    return kBeatsPerSecond * beat_rate_multiplier();
  }

  [[nodiscard]] std::uint64_t elapsed_beats() const noexcept {
    return elapsed_beats_;
  }

  [[nodiscard]] std::uint64_t calendar_day() const noexcept {
    return elapsed_beats_ / kBeatsPerCalendarDay;
  }

  [[nodiscard]] std::uint64_t beat_of_day() const noexcept {
    return elapsed_beats_ % kBeatsPerCalendarDay;
  }

private:
  std::uint64_t elapsed_beats_{0};
  std::atomic<int> beat_rate_multiplier_{1};
};

} // namespace fl::primitives
