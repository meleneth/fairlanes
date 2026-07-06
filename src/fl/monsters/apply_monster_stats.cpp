#include "fl/monsters/apply_monster_stats.hpp"

#include <chrono>
#include <cstdint>
#include <utility>

#include "fl/ecs/components/combat_status.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/ecs/components/visual_effects.hpp"
#include "fl/generated/monster_content.hpp"
#include "fl/widgets/effects/decal.hpp"

namespace fl::monster {
namespace {

void apply_starfire_anomaly_visuals(fl::primitives::EntityBuilder &builder) {
  auto &reg = builder.ctx().reg();
  const auto entity = builder.entity();

  auto &statuses =
      reg.get_or_emplace<fl::ecs::components::CombatStatuses>(entity);
  auto haste = fl::ecs::components::CombatStatusEffect{};
  haste.id = statuses.next_id++;
  haste.kind = fl::ecs::components::CombatStatusKind::Haste;
  haste.name = "Starfire Drift";
  haste.source = entity;
  haste.value = 60;
  haste.negative = false;
  haste.removable = false;
  haste.effect.owner = entity;
  statuses.effects.push_back(std::move(haste));

  auto config = fl::widgets::effects::DecalConfig{};
  config.duration_seconds = 2.0F;
  config.seed = 0x57A2F17Eu;
  reg.emplace_or_replace<fl::ecs::components::CombatantUnderlayDecals>(
      entity,
      fl::ecs::components::CombatantUnderlayDecals{
          fl::ecs::components::DecalEffect{
              seerin::uWu{0}, fl::ecs::components::DecalEffect::Clock::now(),
              std::chrono::milliseconds{2000},
              fl::widgets::effects::DecalAnimationKind::Starfire, config}});
}

} // namespace

void apply_monster_stats(fl::primitives::EntityBuilder &builder,
                         MonsterKind kind) {
  const auto generated = generated_content::stats(kind);

  auto &stats =
      builder.ctx().reg().get<fl::ecs::components::Stats>(builder.entity());
  stats.name_ = generated.display_name;
  stats.hp_ = generated.hp;
  stats.max_hp_ = generated.hp;
  stats.mp_ = generated.mp;

  if (generated.level > 0) {
    auto &xp =
        builder.ctx().reg().get<fl::ecs::components::TrackXP>(builder.entity());
    xp.level_ = generated.level;
    xp.xp_ = xp.xp_for_level(generated.level);
    xp.next_level_at = xp.xp_for_level(generated.level + 1);
  }

  if (kind == MonsterKind::StarfireAnomaly) {
    apply_starfire_anomaly_visuals(builder);
  }
}

} // namespace fl::monster
