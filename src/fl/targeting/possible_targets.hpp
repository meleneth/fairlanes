#pragma once

#include <algorithm>
#include <ranges>
#include <span>
#include <vector>

#include <entt/entity/registry.hpp>

#include "fl/ecs/components/stats.hpp"

namespace fl::targeting {

// Borrowed, lazy candidates. The registry and participant storage must outlive
// the range and its iterators. Structural membership changes invalidate them.
// Recreate a range after changing eligibility; never capture it in deferred
// work.
struct EligibleTarget {
  const entt::registry *registry;
  bool alive;

  bool operator()(entt::entity entity) const {
    if (!registry->valid(entity))
      return false;
    const auto *stats = registry->try_get<fl::ecs::components::Stats>(entity);
    return stats && stats->is_alive() == alive;
  }
};

using TargetRange =
    std::ranges::filter_view<std::span<const entt::entity>, EligibleTarget>;

// Side identity belongs to the encounter, independently of home-party
// ownership.
class PossibleTargets {
public:
  PossibleTargets(const entt::registry &registry,
                  std::span<const entt::entity> attackers,
                  std::span<const entt::entity> defenders)
      : registry_(registry), attackers_(attackers), defenders_(defenders) {}

  TargetRange FriendlyPossibleTargets(entt::entity actor) const {
    return candidates(actor, true, true);
  }
  TargetRange FriendlyDeadPossibleTargets(entt::entity actor) const {
    return candidates(actor, true, false);
  }
  TargetRange EnemyPossibleTargets(entt::entity actor) const {
    return candidates(actor, false, true);
  }
  TargetRange EnemyDeadPossibleTargets(entt::entity actor) const {
    return candidates(actor, false, false);
  }

private:
  TargetRange candidates(entt::entity actor, bool friendly, bool alive) const {
    auto members = std::span<const entt::entity>{};
    if (registry_.valid(actor)) {
      if (std::ranges::find(attackers_, actor) != attackers_.end())
        members = friendly ? attackers_ : defenders_;
      else if (std::ranges::find(defenders_, actor) != defenders_.end())
        members = friendly ? defenders_ : attackers_;
    }
    return TargetRange{members, EligibleTarget{&registry_, alive}};
  }

  const entt::registry &registry_;
  std::span<const entt::entity> attackers_;
  std::span<const entt::entity> defenders_;
};

// Explicit snapshot for effects that mutate membership or schedule deferred
// work. IDs still require revalidation when used; this does not extend entity
// lifetime.
template <std::ranges::input_range Range>
std::vector<entt::entity> snapshot_targets(Range &&range) {
  std::vector<entt::entity> result;
  for (const auto entity : range)
    result.push_back(entity);
  return result;
}

} // namespace fl::targeting
