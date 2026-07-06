#include "fl/ecs/systems/combatant_status_visuals.hpp"

#include <algorithm>
#include <chrono>
#include <limits>
#include <string_view>

#include "fl/ecs/components/dire_bleed.hpp"
#include "fl/ecs/components/freeze.hpp"
#include "fl/ecs/components/poison.hpp"
#include "fl/lospec500.hpp"
#include "fl/widgets/effects/decal.hpp"

namespace fl::ecs::systems {
namespace {

using fl::ecs::components::CombatStatusEffect;
using fl::ecs::components::CombatStatuses;
using fl::ecs::components::CombatStatusKind;
using fl::ecs::components::DecalEffect;
using fl::widgets::effects::DecalAnimationKind;

constexpr int kLegacyStatusPriority = 100;
constexpr int kGenericStatusPriority = 200;
constexpr int kExistingUnderlayPriority = 300;

DecalEffect persistent_decal(DecalAnimationKind kind) {
  return DecalEffect{seerin::uWu{std::numeric_limits<int64_t>::max()},
                     DecalEffect::Clock::time_point{},
                     std::chrono::milliseconds{2000}, kind};
}

CombatantStatusVisual status_row(std::string label, ftxui::Color color,
                                 int priority, bool targeted) {
  CombatantStatusVisual visual;
  visual.layer = CombatantVisualLayer::StatusText;
  visual.region = CombatantVisualRegion::StatusRows;
  visual.priority = priority;
  visual.label = std::move(label);
  visual.color = color;
  visual.preserves_existing_targeted_behavior = targeted;
  return visual;
}

CombatantStatusVisual underlay_for(const CombatStatusEffect &effect,
                                   CombatantVisualRegion region,
                                   DecalAnimationKind kind, int priority) {
  CombatantStatusVisual visual;
  visual.layer = CombatantVisualLayer::Underlay;
  visual.region = region;
  visual.priority = priority;
  visual.label = effect.name;
  visual.status_kind = effect.kind;
  visual.decal = persistent_decal(kind);
  return visual;
}

CombatantStatusVisual label_for(const CombatStatusEffect &effect,
                                CombatantVisualRegion region,
                                ftxui::Color color, int priority) {
  CombatantStatusVisual visual;
  visual.layer = CombatantVisualLayer::StatusText;
  visual.region = region;
  visual.priority = priority;
  visual.label = effect.name;
  visual.color = color;
  visual.status_kind = effect.kind;
  return visual;
}

bool is_custom_underlay_status(const CombatStatusEffect &effect) {
  return effect.kind == CombatStatusKind::Haste &&
         effect.name == "Starfire Drift";
}

void append_shared_status_visuals(std::vector<CombatantStatusVisual> &out,
                                  const CombatStatusEffect &effect) {
  if (is_custom_underlay_status(effect)) {
    return;
  }

  switch (effect.kind) {
  case CombatStatusKind::Shield:
    out.push_back(underlay_for(effect, CombatantVisualRegion::WholeCombatant,
                               DecalAnimationKind::Aura,
                               kGenericStatusPriority));
    break;
  case CombatStatusKind::Haste:
    out.push_back(underlay_for(effect, CombatantVisualRegion::Feet,
                               DecalAnimationKind::Projectile,
                               kGenericStatusPriority));
    break;
  case CombatStatusKind::Slow:
    out.push_back(underlay_for(effect, CombatantVisualRegion::Feet,
                               DecalAnimationKind::Field,
                               kGenericStatusPriority));
    break;
  case CombatStatusKind::Silence:
    out.push_back(label_for(effect, CombatantVisualRegion::Nameplate,
                            fl::lospec500::color_at(30),
                            kGenericStatusPriority));
    break;
  case CombatStatusKind::Stun:
    out.push_back(label_for(effect, CombatantVisualRegion::Head,
                            fl::lospec500::color_at(14),
                            kGenericStatusPriority));
    break;
  case CombatStatusKind::Burn:
    out.push_back(label_for(effect, CombatantVisualRegion::Body,
                            fl::lospec500::color_at(10),
                            kGenericStatusPriority));
    break;
  case CombatStatusKind::Blind:
    out.push_back(label_for(effect, CombatantVisualRegion::StatusRows,
                            fl::lospec500::color_at(25),
                            kGenericStatusPriority));
    break;
  }
}

void append_existing_underlays(std::vector<CombatantStatusVisual> &out,
                               entt::registry &reg, entt::entity entity) {
  const auto *underlays =
      reg.try_get<fl::ecs::components::CombatantUnderlayDecals>(entity);
  if (underlays == nullptr) {
    return;
  }

  int index = 0;
  for (const auto &effect : underlays->effects) {
    CombatantStatusVisual visual;
    visual.layer = CombatantVisualLayer::Underlay;
    visual.region = CombatantVisualRegion::WholeCombatant;
    visual.priority = kExistingUnderlayPriority + index;
    visual.label =
        std::string{fl::widgets::effects::name(effect.animation_kind)};
    visual.decal = effect;
    out.push_back(std::move(visual));
    ++index;
  }
}

void sort_visuals(std::vector<CombatantStatusVisual> &visuals) {
  std::stable_sort(
      visuals.begin(), visuals.end(), [](const auto &lhs, const auto &rhs) {
        if (lhs.priority != rhs.priority) {
          return lhs.priority < rhs.priority;
        }
        if (lhs.layer != rhs.layer) {
          return static_cast<int>(lhs.layer) < static_cast<int>(rhs.layer);
        }
        if (lhs.region != rhs.region) {
          return static_cast<int>(lhs.region) < static_cast<int>(rhs.region);
        }
        return lhs.label < rhs.label;
      });
}

} // namespace

std::vector<CombatantStatusVisual>
combatant_status_visuals(entt::registry &reg, entt::entity entity) {
  std::vector<CombatantStatusVisual> visuals;
  if (!reg.valid(entity)) {
    return visuals;
  }

  if (reg.any_of<fl::ecs::components::Poison>(entity)) {
    visuals.push_back(status_row("Poison", fl::lospec500::color_at(20),
                                 kLegacyStatusPriority, true));
  }
  if (reg.any_of<fl::ecs::components::DireBleed>(entity)) {
    visuals.push_back(status_row("Dire Bleed", fl::lospec500::color_at(6),
                                 kLegacyStatusPriority + 1, true));
  }
  if (reg.any_of<fl::ecs::components::Freeze>(entity)) {
    visuals.push_back(status_row("Frozen", fl::lospec500::color_at(27),
                                 kLegacyStatusPriority + 2, true));
  }

  if (const auto *statuses = reg.try_get<CombatStatuses>(entity)) {
    for (const auto &effect : statuses->effects) {
      append_shared_status_visuals(visuals, effect);
    }
  }

  append_existing_underlays(visuals, reg, entity);
  sort_visuals(visuals);
  return visuals;
}

std::vector<CombatantStatusVisual>
combatant_status_visuals_for(entt::registry &reg, entt::entity entity,
                             CombatantVisualLayer layer,
                             CombatantVisualRegion region) {
  auto visuals = combatant_status_visuals(reg, entity);
  std::erase_if(visuals, [&](const auto &visual) {
    return visual.layer != layer || visual.region != region;
  });
  return visuals;
}

} // namespace fl::ecs::systems
