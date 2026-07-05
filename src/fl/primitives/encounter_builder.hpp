#pragma once
#include <entt/entt.hpp>
#include <span>

#include "encounter_data.hpp"
#include "fl/context.hpp"
#include "fl/monsters/monster_kind.hpp"

namespace fl::primitives {

struct EncounterBuilder {
  static constexpr int kEnemyPartySize = 5;
  static std::span<const fl::monster::MonsterKind> common_woodland() noexcept;
  static std::span<const fl::monster::MonsterKind> rare_woodland() noexcept;

  EncounterBuilder(fl::context::PartyCtx &ctx) : ctx_(ctx) {};

  EncounterData &thump_it_out();

  void add_random_enemy();

  void add_to_enemy_team(entt::entity entity);

private:
  fl::context::PartyCtx &ctx_;
};

} // namespace fl::primitives
