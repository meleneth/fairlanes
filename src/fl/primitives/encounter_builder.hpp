#pragma once
#include <array>
#include <entt/entt.hpp>
#include <vector>

#include "encounter_data.hpp"
#include "fl/context.hpp"
#include "fl/monsters/monster_kind.hpp"

namespace fl::primitives {

struct EncounterBuilder {
  static constexpr int kEnemyPartySize = 5;
  static constexpr std::array<fl::monster::MonsterKind, 6> kCommonWoodland{
      fl::monster::MonsterKind::FieldMouse,
      fl::monster::MonsterKind::BumpkinHare,
      fl::monster::MonsterKind::MireSquish,
      fl::monster::MonsterKind::BarkSmack,
      fl::monster::MonsterKind::PoisonToad,
      fl::monster::MonsterKind::Yeti,
  };

  EncounterBuilder(fl::context::PartyCtx &ctx) : ctx_(ctx) {};

  EncounterData &thump_it_out();

  void add_random_enemy();

  void add_to_enemy_team(entt::entity entity);

private:
  fl::context::PartyCtx &ctx_;
};

} // namespace fl::primitives
