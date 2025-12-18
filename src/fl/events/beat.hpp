#pragma once

#include <chrono>
#include <cstdint>

namespace fl::events {

/// A single "beat" of the simulation clock.
/// Monotonic timestamps only (steady_clock).
struct Beat {
  using clock = std::chrono::steady_clock;

  clock::time_point now{};
  std::chrono::nanoseconds dt{0};
  std::uint64_t index{0};
};

} // namespace fl::events
