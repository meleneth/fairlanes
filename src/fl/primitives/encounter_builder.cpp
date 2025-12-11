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

namespace fl::concepts {

void EncounterBuilder::thump_it_out(fl::context::PartyCtx &ctx) {
  using namespace fl::ecs::components;

  // Attach / ensure an Encounter on the party and add the enemy
  auto &enc =
      ctx.reg_.emplace<Encounter>(ctx.self_, ctx.encounter_context(ctx.self_));
  auto &is_party = ctx.reg_.get<IsParty>(ctx.self_);
  enc.attackers_ =
      std::make_unique<fl::primitives::Team>(ctx.entity_context(ctx.self_));
  enc.defenders_ =
      std::make_unique<fl::primitives::Team>(ctx.entity_context(ctx.self_));

  add_field_mouse(enc.ctx_);

  is_party.for_each_member(
      [&](entt::entity member) { enc.defenders_->members_.push_back(member); });
}

void EncounterBuilder::add_field_mouse(fl::context::PartyCtx &ctx) {
  using namespace fl::ecs::components;

  entt::entity ent =
      EntityBuilder(ctx).monster(fl::monster::MonsterKind::FieldMouse).build();
  add_to_enemy_team(ent);
}

void EncounterBuilder::add_to_enemy_team(entt::entity entity) {
  // First strike wen?
  auto &enc = ctx_.reg_.get<fl::ecs::components::Encounter>(ctx_.self_);

  enc.attackers_->members_.push_back(entity);
  enc.e_to_cleanup_.push_back(entity);
}

} // namespace fl::concepts
