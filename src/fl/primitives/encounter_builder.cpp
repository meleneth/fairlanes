#include "encounter_builder.hpp"
#include "fl/context.hpp"
#include "fl/ecs/components/encounter.hpp"
#include "fl/ecs/components/is_party.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/monsters/register_monsters.hpp" // brings all the monsters with it
#include "fl/primitives/entity_builder.hpp"
#include "fl/primitives/team.hpp"

namespace fl::primitives {

void EncounterBuilder::thump_it_out() {
  using namespace fl::ecs::components;

  auto &enc = ctx_.reg().emplace<fl::ecs::components::Encounter>(ctx_.self());
  auto &is_party = ctx_.reg().get<IsParty>(ctx_.self());

  enc.attackers_ = std::make_unique<fl::primitives::Team>();
  enc.defenders_ = std::make_unique<fl::primitives::Team>();

  add_field_mouse();

  is_party.for_each_member(
      [&](entt::entity member) { enc.defenders_->members_.push_back(member); });
}

void EncounterBuilder::add_field_mouse() {
  using namespace fl::ecs::components;

  auto context = ctx_.build_context();
  entt::entity ent = EntityBuilder(context)
                         .monster(fl::monster::MonsterKind::FieldMouse)
                         .build();
  add_to_enemy_team(ent);
}

void EncounterBuilder::add_to_enemy_team(entt::entity entity) {
  // First strike wen?
  auto &enc = ctx_.reg().get<fl::ecs::components::Encounter>(ctx_.self());

  enc.attackers_->members_.push_back(entity);
  enc.e_to_cleanup_.push_back(entity);
}

} // namespace fl::primitives
