#include <fmt/core.h>

#include "fl/context.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "track_xp.hpp"

namespace fl::ecs::components {
/// Closed-form XP curve: sum_{i=1}^{n} i * BASE_XP_VALUE
int TrackXP::xp_for_level(int level_calc) {
  if (level_calc <= 0)
    return 0;
  if (level_calc > 100)
    return 0;
  return BASE_XP_VALUE * (level_calc * (level_calc + 1)) / 2;
}

TrackXP::TrackXP(fl::context::EntityCtx ctx, int starting_xp)
    : xp_(starting_xp), next_level_at(xp_for_level(level_ + 1)), ctx_(ctx) {}

void TrackXP::add_xp(entt::handle self, int amount) {
  // ctx_.log_.append_markup(      fmt::format("[yellow](xp-debug) e={} +{}xp =>
  // total={} level={} next={}",                  (int)self.entity(), amount,
  // xp_, level_, next_level_at));
  xp_ += amount;
  using fl::ecs::components::Stats;
  while (next_level_at && xp_ >= next_level_at) {
    ++level_;
    next_level_at = xp_for_level(level_ + 1);
    if (auto info = self.try_get<Stats>()) {
      ctx_.log_.append_markup(fmt::format(
          "[name]({}) Level up! level=[level]({}) in next=[xp]({}) XP",
          info->name_, level_, next_level_at));
    } // spdlog::debug("Level up! level={} next={}", level, next_level_at);
  }
}
} // namespace fl::ecs::components
