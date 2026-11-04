#pragma once

#include <algorithm>
#include <limits>

namespace fl::primitives {

struct TowerBalance {
  int maximum_monster_level{20};
  int maximum_monster_hp{100};
  int maximum_monster_attack{20};

  static int scale(int maximum, int percent) noexcept {
    if (maximum <= 0 || percent <= 0)
      return 0;
    const auto scaled = (static_cast<long long>(maximum) * percent + 99) / 100;
    return static_cast<int>(std::clamp(
        scaled, 1LL, static_cast<long long>(std::numeric_limits<int>::max())));
  }
};

} // namespace fl::primitives
