#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>

#include "fl/events/beat.hpp"

namespace fl::events {

/// Owns beat timing policy. Produces Beat events from a monotonic clock.
class BeatClock {
public:
  using clock = Beat::clock;

  BeatClock() = default;

  void start(clock::time_point now = clock::now()) {
    last_ = now;
    index_ = 0;
    started_ = true;
  }

  void set_paused(bool paused) { paused_ = paused; }
  bool paused() const { return paused_; }

  void set_time_scale(double scale) { time_scale_ = scale; } // 1.0 normal
  double time_scale() const { return time_scale_; }

  void set_dt_cap(std::chrono::nanoseconds cap) { dt_cap_ = cap; } // e.g. 100ms

  /// Call once per outer loop iteration. Returns a Beat you can emit.
  Beat next(clock::time_point now = clock::now()) {
    if (!started_)
      start(now);

    auto raw_dt =
        std::chrono::duration_cast<std::chrono::nanoseconds>(now - last_);
    last_ = now;

    // Cap huge dt spikes (debugger break, window drag, etc.)
    if (dt_cap_.count() > 0)
      raw_dt = std::min(raw_dt, dt_cap_);

    // Pause + scale
    auto dt = paused_ ? std::chrono::nanoseconds{0}
                      : std::chrono::nanoseconds{static_cast<std::int64_t>(
                            raw_dt.count() * time_scale_)};

    return Beat{.now = now, .dt = dt, .index = index_++};
  }

private:
  clock::time_point last_{};
  std::uint64_t index_{0};
  bool started_{false};
  bool paused_{false};
  double time_scale_{1.0};
  std::chrono::nanoseconds dt_cap_{0};
};

} // namespace fl::events
