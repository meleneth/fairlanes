#include "encounter_builder.hpp"
#include "fl/context.hpp"
#include "fl/ecs/components/encounter.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/primitives/entity_builder.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/team.hpp"

namespace fl::primitives {

void EncounterBuilder::thump_it_out() {
  auto &encounter_data = ctx_.party_data().create_encounter();

  // auto party = ctx_.party_data().party_id();

  // TODO or is this the crash
  //  ctx_.reg().emplace<fl::ecs::components::Encounter>(party, party,
  //                                                   &encounter_data);

  add_field_mouse();

  ctx_.party_data().for_each_member([&](entt::entity member) {
    encounter_data.defenders().members().push_back(member);
  });
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

  auto &encounter_data = ctx_.party_data().encounter_data();
  // First strike wen?

  encounter_data.attackers().members().push_back(entity);
  encounter_data.entities_to_cleanup().push_back(entity);
}

} // namespace fl::primitives
