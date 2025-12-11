#pragma once
#include <entt/entt.hpp>

#include "fl/context.hpp"

namespace fl::widgets {
class FancyLog;
}
namespace fl::ecs::components {

struct TrackXP {
  static constexpr int BASE_XP_VALUE = 256;

  int level_ = 1;
  int xp_ = 0;
  int next_level_at = BASE_XP_VALUE * ((level_ + 1) * (level_ + 2)) / 2;
  fl::context::EntityCtx ctx_;

  /// Closed-form XP curve: sum_{i=1}^{n} i * BASE_XP_VALUE
  int xp_for_level(int level_calc);
  explicit TrackXP(fl::context::EntityCtx ctx, int starting_xp);

  void add_xp(entt::handle self, int amount);
};

} // namespace fl::ecs::components
