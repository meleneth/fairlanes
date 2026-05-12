#include "eviscerate.hpp"

#include <algorithm>

#include <fmt/format.h>

#include "fl/ecs/components/dire_bleed.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::skills {

void Eviscerate::eviscerate(fl::context::AttackCtx &&ctx) {
  auto &reg = ctx.reg();
  auto &attacker_stats = reg.get<fl::ecs::components::Stats>(ctx.attacker());
  auto &defender_stats = reg.get<fl::ecs::components::Stats>(ctx.defender());
  const int damage_per_tick = std::max(1, defender_stats.max_hp_ / 10);

  auto &bleed =
      reg.emplace_or_replace<fl::ecs::components::DireBleed>(ctx.defender());
  bleed.source = ctx.attacker();
  bleed.damage_per_tick = damage_per_tick;

  ctx.log().append_markup(
      fmt::format("[enemy_name]({}) used [ability](Eviscerate) on "
                  "[player_name]({}); [error](Dire Bleed) takes hold.",
                  attacker_stats.name_, defender_stats.name_));
}

} // namespace fl::skills
