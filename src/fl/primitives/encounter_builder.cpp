#include "encounter_builder.hpp"

#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <fmt/format.h>

#include "fl/context.hpp"
#include "fl/ecs/components/encounter.hpp"
#include "fl/ecs/components/monster_identity.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/generated/floor_content.hpp"
#include "fl/generated/monster_content.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/monsters/monster_registry.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/encounter_data.hpp"
#include "fl/primitives/entity_builder.hpp"
#include "fl/primitives/farming_plan.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/team.hpp"
#include "fl/primitives/tower_balance.hpp"
#include "sr/atb_events.hpp"

namespace fl::primitives {

std::vector<fl::monster::MonsterKind> EncounterBuilder::chaos_attractor_monster_pool() {
  std::vector<fl::monster::MonsterKind> pool;
  const auto& registry = fl::monster::monster_registry();
  pool.reserve(registry.size());

  for (const auto& [kind, definition] : registry) {
    (void)definition;
    const auto bosses = fl::monster::generated_content::raid_bosses();
    if (std::ranges::find(bosses, kind) == bosses.end()) pool.push_back(kind);
  }

  std::ranges::sort(pool, [](const auto lhs, const auto rhs) {
    using Value = std::underlying_type_t<fl::monster::MonsterKind>;
    return static_cast<Value>(lhs) < static_cast<Value>(rhs);
  });
  return pool;
}

std::span<const fl::monster::MonsterKind> EncounterBuilder::common_woodland() noexcept {
  return fl::monster::generated_content::common_woodland();
}

std::span<const fl::monster::MonsterKind> EncounterBuilder::rare_woodland() noexcept {
  return fl::monster::generated_content::rare_woodland();
}

EncounterData& EncounterBuilder::thump_it_out() {
  const auto* floor = tower_floor_ ? fl::primitives::generated_content::floor_lineup(
                                         tower_floor_->biome_id, tower_floor_->floor)
                                   : nullptr;
  if (tower_floor_ && floor == nullptr) {
    throw std::invalid_argument("unknown tower floor");
  }
  if (mode_ == EncounterMode::HeroesJourney) {
    const auto& plan = ctx_.party_data().farming_plan();
    ctx_.log().append_markup(fmt::format(
        "Farming [ability]({}) as [ability]({}); progression path: [xp]({}).",
        display_name(plan.focus), display_name(plan.discipline), display_name(plan.reward_class)));
  }

  auto& encounter_data = ctx_.party_data().create_encounter();

  const auto enemy_count = floor ? floor->monster_count : kEnemyPartySize;
  for (std::size_t i = 0; i < enemy_count; ++i) {
    if (floor) {
      const auto entity = add_enemy_kind(floor->monsters[i]);
      const auto& balance = ctx_.account_data().tower_balance();
      auto& stats = ctx_.reg().get<fl::ecs::components::Stats>(entity);
      auto& xp = ctx_.reg().get<fl::ecs::components::TrackXP>(entity);
      const auto progression = balance.resolve(tower_floor_->floor, tower_floor_->biome_id,
                                               xp.level_, stats.max_hp_, floor->level_percent[i]);
      stats.max_hp_ = progression.max_hp;
      stats.hp_ = stats.max_hp_;
      stats.attack_ = progression.attack;
      xp.level_ = progression.level;
      xp.xp_ = xp.xp_for_level(xp.level_);
      xp.next_level_at = xp.xp_for_level(xp.level_ + 1);
    } else {
      add_random_enemy();
    }
  }

  if (floor && floor->cover.enabled()) {
    const auto protected_index = static_cast<std::size_t>(floor->cover.protected_index);
    const auto protector_index = static_cast<std::size_t>(floor->cover.protector_index);
    if (protected_index >= encounter_data.attackers().members().size() ||
        protector_index >= encounter_data.attackers().members().size()) {
      throw std::logic_error("generated tower cover index is out of range");
    }
    encounter_data.configure_projectile_cover(encounter_data.attackers().members()[protected_index],
                                              encounter_data.attackers().members()[protector_index],
                                              floor->cover.release_skill);
  }

  ctx_.party_data().for_each_member([&](entt::entity member) {
    encounter_data.defenders().members().push_back(member);
    encounter_data.add_party_combatant_bus(member);
    encounter_data.atb_in().emit(seerin::AtbInEvent{seerin::AddCombatant{member}});
  });
  ctx_.bus().emit(fl::events::PartyEvent{fl::events::PartyCombatStarted{}});
  return encounter_data;
}

EncounterData& EncounterBuilder::one_on_one(fl::monster::MonsterKind enemy) {
  auto& encounter = ctx_.party_data().create_encounter();
  add_enemy_kind(enemy);
  ctx_.party_data().for_each_member([&](entt::entity member) {
    encounter.defenders().members().push_back(member);
    encounter.add_party_combatant_bus(member);
    encounter.atb_in().emit(seerin::AtbInEvent{seerin::AddCombatant{member}});
  });
  ctx_.bus().emit(fl::events::PartyEvent{fl::events::PartyCombatStarted{}});
  return encounter;
}

void EncounterBuilder::add_random_enemy() {
  using namespace fl::ecs::components;

  auto rs = ctx_.rng().stream("encounter/spawn.monster");
  auto pick_from = [&rs](const auto& pool) {
    return pool[static_cast<std::size_t>(rs.random_index(static_cast<int>(pool.size())))];
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
      return rs.random_index(20) == 0 ? pick_from(rare_woodland()) : pick_from(common_woodland());
    }

    return fl::monster::MonsterKind::FieldMouse;
  }();

  add_enemy_kind(kind);
}

entt::entity EncounterBuilder::add_enemy_kind(fl::monster::MonsterKind kind) {
  auto context = ctx_.build_context();
  entt::entity ent = EntityBuilder(context).monster(kind).build();
  add_to_enemy_team(ent);
  return ent;
}

void EncounterBuilder::add_to_enemy_team(entt::entity entity) {

  auto& encounter_data = ctx_.party_data().encounter_data();
  // First strike wen?

  encounter_data.attackers().members().push_back(entity);
  if (const auto* identity = ctx_.reg().try_get<fl::ecs::components::MonsterIdentity>(entity)) {
    ctx_.bus().emit(fl::events::PartyEvent{fl::events::MonsterEncountered{identity->kind}});
  }
  encounter_data.entities_to_cleanup().push_back(entity);
  encounter_data.add_enemy_combatant_bus(entity);
  encounter_data.atb_in().emit(seerin::AtbInEvent{seerin::AddCombatant{entity}});
}

} // namespace fl::primitives
