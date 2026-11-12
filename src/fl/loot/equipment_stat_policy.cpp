#include "fl/loot/equipment_stat_policy.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <numeric>
#include <string>

#include "fl/generated/gear_content.hpp"

namespace fl::loot {
namespace {

enum class Stat : std::size_t {
  attack,
  armor,
  max_hp,
  physical,
  magical,
  fire,
  ice,
  lightning,
  count,
};

using Weights = std::array<int, static_cast<std::size_t>(Stat::count)>;

constexpr Weights kNeutralWeights{30, 30, 35, 15, 15, 15, 15, 15};
constexpr Weights kWeaponSlotWeights{100, 5, 20, 5, 5, 5, 5, 5};
constexpr Weights kArmorSlotWeights{10, 100, 70, 45, 45, 45, 45, 45};
constexpr Weights kCapeSlotWeights{20, 60, 60, 80, 80, 80, 80, 80};
constexpr Weights kJewelrySlotWeights{60, 45, 70, 70, 70, 70, 70, 70};

bool is_weapon(EquipmentSlot slot) {
  return slot == EquipmentSlot::mainhand || slot == EquipmentSlot::offhand ||
         slot == EquipmentSlot::knife;
}

bool is_jewelry(EquipmentSlot slot) {
  return slot == EquipmentSlot::necklace || slot == EquipmentSlot::ring_1 ||
         slot == EquipmentSlot::ring_2;
}

const Weights& slot_weights(EquipmentSlot slot) {
  if (is_weapon(slot)) return kWeaponSlotWeights;
  if (is_jewelry(slot)) return kJewelrySlotWeights;
  if (slot == EquipmentSlot::cape) return kCapeSlotWeights;
  return kArmorSlotWeights;
}

Weights biome_weights(std::string_view biome_id) {
  const auto* profile = generated_content::equipment_profile(biome_id);
  if (profile == nullptr) return kNeutralWeights;
  const auto& weights = profile->weights;
  return {weights.attack,  weights.armor, weights.max_hp, weights.physical,
          weights.magical, weights.fire,  weights.ice,    weights.lightning};
}

Weights combined_weights(EquipmentSlot slot, std::string_view biome_id) {
  auto result = biome_weights(biome_id);
  const auto& allowed = slot_weights(slot);
  for (std::size_t i = 0; i < result.size(); ++i) result[i] *= allowed[i];
  return result;
}

void add_stat(EquipmentBuilder& item, Stat stat, int units = 1) {
  switch (stat) {
  case Stat::attack: item.attack += units; break;
  case Stat::armor: item.armor += units; break;
  case Stat::max_hp: item.max_hp += 3 * units; break;
  case Stat::physical: item.resistances.physical += 5 * units; break;
  case Stat::magical: item.resistances.magical += 5 * units; break;
  case Stat::fire: item.resistances.fire += 5 * units; break;
  case Stat::ice: item.resistances.ice += 5 * units; break;
  case Stat::lightning: item.resistances.lightning += 5 * units; break;
  case Stat::count: break;
  }
}

Stat roll_stat(fl::primitives::RandomStream& random, const Weights& weights) {
  const int total = std::accumulate(weights.begin(), weights.end(), 0);
  assert(total > 0);
  const int rolled = random.uniform_int(1, total);
  int cursor = 0;
  for (std::size_t i = 0; i < weights.size(); ++i) {
    cursor += weights[i];
    if (rolled <= cursor) return static_cast<Stat>(i);
  }
  return Stat::max_hp;
}

Weights stat_units(const fl::ecs::components::Equipment& item) {
  const auto& r = item.resistances();
  return {item.attack(), item.armor(), item.max_hp() / 3, r.physical / 5,
          r.magical / 5, r.fire / 5,   r.ice / 5,         r.lightning / 5};
}

void allocate_proportionally(EquipmentBuilder& result, const Weights& source, int budget) {
  const int source_total = std::accumulate(source.begin(), source.end(), 0);
  if (source_total <= 0) return;

  struct Remainder {
    std::size_t stat;
    int remainder;
  };
  std::array<Remainder, static_cast<std::size_t>(Stat::count)> remainders{};
  int allocated = 0;
  for (std::size_t i = 0; i < source.size(); ++i) {
    const int numerator = source[i] * budget;
    const int units = numerator / source_total;
    add_stat(result, static_cast<Stat>(i), units);
    allocated += units;
    remainders[i] = {i, numerator % source_total};
  }
  std::stable_sort(remainders.begin(), remainders.end(),
                   [](const auto& lhs, const auto& rhs) { return lhs.remainder > rhs.remainder; });
  for (int i = allocated; i < budget; ++i)
    add_stat(result, static_cast<Stat>(remainders[static_cast<std::size_t>(i - allocated)].stat));
}

int roll_budget(fl::primitives::RandomStream& random, Tier tier) {
  const auto range = equipment_tier_budget_range(tier);
  return random.uniform_int(range.minimum, range.maximum);
}

} // namespace

EquipmentBudgetRange equipment_tier_budget_range(Tier tier) noexcept {
  switch (tier) {
  case Tier::worn: return {1, 2};
  case Tier::plain: return {2, 4};
  case Tier::sturdy: return {4, 6};
  case Tier::fine: return {6, 9};
  case Tier::excellent: return {9, 13};
  case Tier::masterwork: return {13, 18};
  case Tier::mythic: return {18, 24};
  }
  return {1, 2};
}

int equipment_minimum_level(Tier tier) noexcept {
  switch (tier) {
  case Tier::worn:
  case Tier::plain: return 1;
  case Tier::sturdy: return 10;
  case Tier::fine: return 25;
  case Tier::excellent: return 40;
  case Tier::masterwork: return 55;
  case Tier::mythic: return 75;
  }
  return 1;
}

int equipment_stat_budget(const fl::ecs::components::Equipment& item) noexcept {
  const auto& r = item.resistances();
  return item.attack() + item.armor() + item.max_hp() / 3 + r.physical / 5 + r.magical / 5 +
         r.fire / 5 + r.ice / 5 + r.lightning / 5;
}

int equipment_combat_value(const fl::ecs::components::Equipment& item) noexcept {
  const auto& r = item.resistances();
  return item.attack() * 6 + item.armor() * 6 + item.max_hp() * 2 + r.physical + r.magical +
         r.fire + r.ice + r.lightning;
}

void apply_ordinary_equipment_stats(EquipmentBuilder& item, fl::primitives::RandomHub& rng,
                                    std::string_view biome_id, std::string_view stream_name) {
  item.biome_id = std::string{biome_id};
  const auto weights = combined_weights(item.slot, biome_id);
  auto random = rng.stream(stream_name);
  const int budget = roll_budget(random, item.tier);
  for (int unit = 0; unit < budget; ++unit) add_stat(item, roll_stat(random, weights));
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
  auto source = stat_units(first);
  const auto second_units = stat_units(second);
  const auto third_units = stat_units(third);
  for (std::size_t i = 0; i < source.size(); ++i) source[i] += second_units[i] + third_units[i];
  auto random = rng.stream(stream_name);
  const int target_budget = roll_budget(random, target_tier);
  if (std::accumulate(source.begin(), source.end(), 0) > 0) {
    allocate_proportionally(result, source, target_budget);
  } else {
    const auto weights = combined_weights(result.slot, first.biome_id());
    for (int unit = 0; unit < target_budget; ++unit) add_stat(result, roll_stat(random, weights));
  }
  return result;
}

} // namespace fl::loot
