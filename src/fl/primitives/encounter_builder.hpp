#pragma once
#include <entt/entity/registry.hpp>
#include <span>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "encounter_data.hpp"
#include "fl/context.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/primitives/encounter_mode.hpp"

namespace fl::primitives {

struct EncounterBuilder {
  struct TowerFloor {
    std::string biome_id;
    int floor;
  };
  static constexpr int kEnemyPartySize = 5;
  static std::span<const fl::monster::MonsterKind> common_woodland() noexcept;
  static std::span<const fl::monster::MonsterKind> rare_woodland() noexcept;

  EncounterBuilder(fl::context::PartyCtx &ctx,
                   EncounterMode mode = EncounterMode::ChaosAttractor,
                   std::optional<TowerFloor> tower_floor = std::nullopt)
      : ctx_(ctx), mode_(mode), tower_floor_(std::move(tower_floor)) {};

  [[nodiscard]] static std::vector<fl::monster::MonsterKind>
  chaos_attractor_monster_pool();

  EncounterData &thump_it_out();

  void add_random_enemy();

  void add_to_enemy_team(entt::entity entity);

private:
  void add_enemy_kind(fl::monster::MonsterKind kind);
  fl::context::PartyCtx &ctx_;
  EncounterMode mode_{EncounterMode::ChaosAttractor};
  std::optional<TowerFloor> tower_floor_;
};

} // namespace fl::primitives
