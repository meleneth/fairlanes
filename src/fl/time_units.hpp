// time_units.hpp
#pragma once
#include <chrono>

namespace fl::time {

constexpr int UNITS_PER_SEC = 960;
constexpr int ANIM_FPS = 12;
constexpr int FRAME_UNITS = UNITS_PER_SEC / ANIM_FPS; // 80

inline int to_units(std::chrono::nanoseconds ns) {
  return static_cast<int>(std::chrono::duration<double>(ns).count() *
                          UNITS_PER_SEC);
}

} // namespace fl::time
