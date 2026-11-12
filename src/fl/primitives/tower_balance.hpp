#pragma once

#include <algorithm>
#include <limits>
#include <string_view>

namespace fl::primitives {

struct TowerBalance {
  int maximum_monster_level{100};
  int maximum_monster_hp{100};
  int maximum_monster_attack{20};

  struct MonsterProgression {
    int level;
    int max_hp;
    int attack;
  };

  [[nodiscard]] MonsterProgression resolve(int floor, std::string_view biome_id, int identity_level,
                                           int identity_hp, int difficulty_percent) const noexcept {
    const int tower_level = std::max(1, scale(maximum_monster_level, floor));
    const int identity_adjustment = std::clamp(identity_level - 4, -3, 3);
    const int biome_adjustment = biome_id.find("swamp") != std::string_view::npos ||
                                         biome_id.find("desert") != std::string_view::npos
                                     ? 2
                                 : biome_id.find("plains") != std::string_view::npos ||
                                         biome_id.find("caves") != std::string_view::npos
                                     ? 1
                                     : 0;
    const int difficulty_adjustment = std::clamp((difficulty_percent - floor) / 10, -2, 2);
    const int level =
        std::max(1, tower_level + identity_adjustment + biome_adjustment + difficulty_adjustment);
    const int hp_curve = std::max(1, identity_hp / 2 + level);
    const int hp = std::max(1, hp_curve * maximum_monster_hp / 100);
    const int attack = std::max(1, (maximum_monster_attack * level + 99) / 100);
    return {level, hp, attack};
  }

  static int scale(int maximum, int percent) noexcept {
    if (maximum <= 0 || percent <= 0) return 0;
    const auto scaled = (static_cast<long long>(maximum) * percent + 99) / 100;
    return static_cast<int>(
        std::clamp(scaled, 1LL, static_cast<long long>(std::numeric_limits<int>::max())));
  }
};

} // namespace fl::primitives
