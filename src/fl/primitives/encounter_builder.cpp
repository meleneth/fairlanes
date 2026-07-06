#include "encounter_builder.hpp"

#include <algorithm>
#include <type_traits>
#include <vector>

#include <fmt/format.h>

#include "fl/context.hpp"
#include "fl/ecs/components/encounter.hpp"
#include "fl/generated/monster_content.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/monsters/monster_registry.hpp"
#include "fl/primitives/encounter_data.hpp"
#include "fl/primitives/entity_builder.hpp"
#include "fl/primitives/farming_plan.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/team.hpp"
#include "sr/atb_events.hpp"

namespace fl::primitives {

std::vector<fl::monster::MonsterKind>
EncounterBuilder::chaos_attractor_monster_pool() {
  std::vector<fl::monster::MonsterKind> pool;
  const auto &registry = fl::monster::monster_registry();
  pool.reserve(registry.size());

  for (const auto &[kind, definition] : registry) {
    (void)definition;
    pool.push_back(kind);
  }

  std::ranges::sort(pool, [](const auto lhs, const auto rhs) {
    using Value = std::underlying_type_t<fl::monster::MonsterKind>;
    return static_cast<Value>(lhs) < static_cast<Value>(rhs);
  });
  return pool;
}

std::span<const fl::monster::MonsterKind>
EncounterBuilder::common_woodland() noexcept {
  return fl::monster::generated_content::common_woodland();
}

std::span<const fl::monster::MonsterKind>
EncounterBuilder::rare_woodland() noexcept {
  return fl::monster::generated_content::rare_woodland();
}

EncounterData &EncounterBuilder::thump_it_out() {
  const auto &plan = ctx_.party_data().farming_plan();
  ctx_.log().append_markup(fmt::format(
      "Farming [ability]({}) as [ability]({}); progression path: [xp]({}).",
      display_name(plan.focus), display_name(plan.discipline),
      display_name(plan.reward_class)));

  auto &encounter_data = ctx_.party_data().create_encounter();

  for (int i = 0; i < kEnemyPartySize; ++i) {
    add_random_enemy();
  }

  ctx_.party_data().for_each_member([&](entt::entity member) {
    encounter_data.defenders().members().push_back(member);
    encounter_data.add_party_combatant_bus(member);
    encounter_data.atb_in().emit(
        seerin::AtbInEvent{seerin::AddCombatant{member}});
  });
  return encounter_data;
}

void EncounterBuilder::add_random_enemy() {
  using namespace fl::ecs::components;

  auto rs = ctx_.rng().stream("encounter/spawn.monster");
  auto pick_from = [&rs](const auto &pool) {
    return pool[static_cast<std::size_t>(
        rs.random_index(static_cast<int>(pool.size())))];
  };

  auto chaos_pool = std::vector<fl::monster::MonsterKind>{};
  const auto kind = [&]() {
    switch (mode_) {
    case EncounterMode::ChaosAttractor:
      chaos_pool = chaos_attractor_monster_pool();
      if (!chaos_pool.empty()) {
        return pick_from(chaos_pool);
      }
      return fl::monster::MonsterKind::FieldMouse;
    case EncounterMode::HeroesJourney:
      return rs.random_index(20) == 0 ? pick_from(rare_woodland())
                                      : pick_from(common_woodland());
    }

    return fl::monster::MonsterKind::FieldMouse;
  }();

  auto context = ctx_.build_context();
  entt::entity ent = EntityBuilder(context).monster(kind).build();
  add_to_enemy_team(ent);
}

void EncounterBuilder::add_to_enemy_team(entt::entity entity) {

  auto &encounter_data = ctx_.party_data().encounter_data();
  // First strike wen?

  encounter_data.attackers().members().push_back(entity);
  encounter_data.entities_to_cleanup().push_back(entity);
  encounter_data.add_enemy_combatant_bus(entity);
  encounter_data.atb_in().emit(
      seerin::AtbInEvent{seerin::AddCombatant{entity}});
}

} // namespace fl::primitives
