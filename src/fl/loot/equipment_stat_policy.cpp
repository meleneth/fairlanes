#include "fl/loot/equipment_stat_policy.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <limits>
#include <numeric>
#include <string>

#include "fl/generated/equipment_policy.hpp"
#include "fl/generated/gear_content.hpp"
#include "fl/loot/equipment_stat.hpp"

namespace fl::loot {
namespace {

using Weights = std::array<int, kEquipmentStats.size()>;

const generated_content::EquipmentStatPolicy& stat_policy(EquipmentStat stat) {
  const auto* policy = generated_content::equipment_stat_policy(stat);
  assert(policy != nullptr);
  return *policy;
}

const generated_content::EquipmentTierPolicy& tier_policy(Tier tier) {
  const auto* policy = generated_content::equipment_tier_policy(tier);
  assert(policy != nullptr);
  return *policy;
}

Weights weights_from(const generated_content::EquipmentStatWeights& weights) {
  return {weights.attack,  weights.armor, weights.max_hp, weights.physical,
          weights.magical, weights.fire,  weights.ice,    weights.lightning};
}

Weights biome_weights(std::string_view biome_id) {
  const auto* profile = generated_content::equipment_profile(biome_id);
  if (profile == nullptr) return weights_from(generated_content::kDefaultEquipmentStatWeights);
  const auto& weights = profile->weights;
  return {weights.attack,  weights.armor, weights.max_hp, weights.physical,
          weights.magical, weights.fire,  weights.ice,    weights.lightning};
}

Weights combined_weights(EquipmentSlot slot, std::string_view biome_id) {
  auto result = biome_weights(biome_id);
  const auto* slot_policy = generated_content::equipment_slot_policy(slot);
  assert(slot_policy != nullptr);
  const auto allowed = weights_from(slot_policy->weights);
  for (std::size_t i = 0; i < result.size(); ++i) result[i] *= allowed[i];
  return result;
}

void add_stat(EquipmentBuilder& item, EquipmentStat stat, int allocations = 1) {
  const int amount = stat_policy(stat).allocation_yield * allocations;
  switch (stat) {
  case EquipmentStat::attack: item.attack += amount; break;
  case EquipmentStat::armor: item.armor += amount; break;
  case EquipmentStat::max_hp: item.max_hp += amount; break;
  case EquipmentStat::physical: item.resistances.physical += amount; break;
  case EquipmentStat::magical: item.resistances.magical += amount; break;
  case EquipmentStat::fire: item.resistances.fire += amount; break;
  case EquipmentStat::ice: item.resistances.ice += amount; break;
  case EquipmentStat::lightning: item.resistances.lightning += amount; break;
  }
}

int stat_amount(const fl::ecs::components::Equipment& item, EquipmentStat stat) {
  switch (stat) {
  case EquipmentStat::attack: return item.attack();
  case EquipmentStat::armor: return item.armor();
  case EquipmentStat::max_hp: return item.max_hp();
  case EquipmentStat::physical: return item.resistances().physical;
  case EquipmentStat::magical: return item.resistances().magical;
  case EquipmentStat::fire: return item.resistances().fire;
  case EquipmentStat::ice: return item.resistances().ice;
  case EquipmentStat::lightning: return item.resistances().lightning;
  }
  return 0;
}

EquipmentStat roll_stat(fl::primitives::RandomStream& random, const Weights& weights,
                        int remaining_budget) {
  Weights eligible = weights;
  for (std::size_t i = 0; i < eligible.size(); ++i)
    if (stat_policy(kEquipmentStats[i]).budget_cost > remaining_budget) eligible[i] = 0;
  const int total = std::accumulate(eligible.begin(), eligible.end(), 0);
  assert(total > 0);
  const int rolled = random.uniform_int(1, total);
  int cursor = 0;
  for (std::size_t i = 0; i < eligible.size(); ++i) {
    cursor += eligible[i];
    if (rolled <= cursor) return kEquipmentStats[i];
  }
  return EquipmentStat::max_hp;
}

Weights stat_budget_units(const fl::ecs::components::Equipment& item) {
  Weights result{};
  for (std::size_t i = 0; i < result.size(); ++i) {
    const auto& policy = stat_policy(kEquipmentStats[i]);
    result[i] =
        stat_amount(item, kEquipmentStats[i]) / policy.allocation_yield * policy.budget_cost;
  }
  return result;
}

bool unit_costs_are_one() {
  return std::ranges::all_of(kEquipmentStats,
                             [](auto stat) { return stat_policy(stat).budget_cost == 1; });
}

void allocate_proportionally(EquipmentBuilder& result, const Weights& source, int budget) {
  const int source_total = std::accumulate(source.begin(), source.end(), 0);
  if (source_total <= 0) return;

  if (unit_costs_are_one()) {
    struct Remainder {
      std::size_t stat;
      int remainder;
    };
    std::array<Remainder, kEquipmentStats.size()> remainders{};
    int allocated = 0;
    for (std::size_t i = 0; i < source.size(); ++i) {
      const int numerator = source[i] * budget;
      const int units = numerator / source_total;
      add_stat(result, kEquipmentStats[i], units);
      allocated += units;
      remainders[i] = {i, numerator % source_total};
    }
    std::stable_sort(remainders.begin(), remainders.end(), [](const auto& lhs, const auto& rhs) {
      return lhs.remainder > rhs.remainder;
    });
    for (int i = allocated; i < budget; ++i)
      add_stat(result, kEquipmentStats[remainders[static_cast<std::size_t>(i - allocated)].stat]);
    return;
  }

  // Spending and rounding are deterministic engine mechanics even when costs
  // are content policy. An unspendable remainder is valid.
  Weights spent{};
  int remaining = budget;
  while (remaining > 0) {
    std::size_t best = source.size();
    long long best_deficit = std::numeric_limits<long long>::min();
    for (std::size_t i = 0; i < source.size(); ++i) {
      const int cost = stat_policy(kEquipmentStats[i]).budget_cost;
      if (source[i] <= 0 || cost > remaining) continue;
      const long long deficit = static_cast<long long>(source[i]) * budget -
                                static_cast<long long>(spent[i]) * source_total;
      if (deficit > best_deficit) {
        best = i;
        best_deficit = deficit;
      }
    }
    if (best == source.size()) break;
    const int cost = stat_policy(kEquipmentStats[best]).budget_cost;
    add_stat(result, kEquipmentStats[best]);
    spent[best] += cost;
    remaining -= cost;
  }
}

int roll_budget(fl::primitives::RandomStream& random, Tier tier) {
  const auto& policy = tier_policy(tier);
  return random.uniform_int(policy.minimum_budget, policy.maximum_budget);
}

void allocate_randomly(EquipmentBuilder& item, fl::primitives::RandomStream& random,
                       const Weights& weights, int budget) {
  int remaining = budget;
  while (remaining > 0) {
    bool affordable = false;
    for (std::size_t i = 0; i < weights.size(); ++i)
      affordable = affordable ||
                   (weights[i] > 0 && stat_policy(kEquipmentStats[i]).budget_cost <= remaining);
    if (!affordable) break;
    const auto stat = roll_stat(random, weights, remaining);
    add_stat(item, stat);
    remaining -= stat_policy(stat).budget_cost;
  }
}

} // namespace

EquipmentBudgetRange equipment_tier_budget_range(Tier tier) noexcept {
  const auto& policy = tier_policy(tier);
  return {policy.minimum_budget, policy.maximum_budget};
}

int equipment_minimum_level(Tier tier) noexcept {
  return tier_policy(tier).minimum_level;
}

int equipment_stat_budget(const fl::ecs::components::Equipment& item) noexcept {
  const auto units = stat_budget_units(item);
  return std::accumulate(units.begin(), units.end(), 0);
}

int equipment_combat_value(const fl::ecs::components::Equipment& item) noexcept {
  int result = 0;
  for (const auto stat : kEquipmentStats)
    result += stat_amount(item, stat) * stat_policy(stat).combat_value_coefficient;
  return result;
}

int minimum_generated_combat_value(EquipmentSlot slot, Tier tier,
                                   std::string_view biome_id) noexcept {
  const auto weights = combined_weights(slot, biome_id);
  int minimum_per_budget = std::numeric_limits<int>::max();
  for (std::size_t i = 0; i < weights.size(); ++i) {
    if (weights[i] <= 0) continue;
    const auto& policy = stat_policy(kEquipmentStats[i]);
    const int allocation_value = policy.allocation_yield * policy.combat_value_coefficient;
    minimum_per_budget = std::min(minimum_per_budget, allocation_value / policy.budget_cost);
  }
  return minimum_per_budget == std::numeric_limits<int>::max()
             ? 0
             : minimum_per_budget * tier_policy(tier).minimum_budget;
}

void apply_ordinary_equipment_stats(EquipmentBuilder& item, fl::primitives::RandomHub& rng,
                                    std::string_view biome_id, std::string_view stream_name) {
  item.biome_id = std::string{biome_id};
  auto random = rng.stream(stream_name);
  allocate_randomly(item, random, combined_weights(item.slot, biome_id),
                    roll_budget(random, item.tier));
}

EquipmentBuilder upgraded_equipment_builder(const fl::ecs::components::Equipment& first,
                                            const fl::ecs::components::Equipment& second,
                                            const fl::ecs::components::Equipment& third,
                                            Tier target_tier, fl::primitives::RandomHub& rng,
                                            std::string_view stream_name) {
  assert(first.slot() == second.slot() && second.slot() == third.slot());
  assert(first.armor_kind() == second.armor_kind() && second.armor_kind() == third.armor_kind());
  assert(first.biome_id() == second.biome_id() && second.biome_id() == third.biome_id());
  EquipmentBuilder result{.slot = first.slot(),
                          .armor_kind = first.armor_kind(),
                          .tier = target_tier,
                          .name = {},
                          .biome_id = std::string{first.biome_id()}};
  auto source = stat_budget_units(first);
  const auto second_units = stat_budget_units(second);
  const auto third_units = stat_budget_units(third);
  for (std::size_t i = 0; i < source.size(); ++i) source[i] += second_units[i] + third_units[i];
  auto random = rng.stream(stream_name);
  const int target_budget = roll_budget(random, target_tier);
  if (std::accumulate(source.begin(), source.end(), 0) > 0) {
    allocate_proportionally(result, source, target_budget);
  } else {
    allocate_randomly(result, random, combined_weights(result.slot, first.biome_id()),
                      target_budget);
  }
  return result;
}

} // namespace fl::loot
