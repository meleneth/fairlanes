#include "encounter_builder.hpp"
#include "fl/context.hpp"
#include "fl/ecs/components/encounter.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/primitives/encounter_data.hpp"
#include "fl/primitives/entity_builder.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/team.hpp"
#include "sr/encounter_events.hpp"

namespace fl::primitives {

EncounterData &EncounterBuilder::thump_it_out() {
  auto &encounter_data = ctx_.party_data().create_encounter();

  add_field_mouse();

  ctx_.party_data().for_each_member([&](entt::entity member) {
    encounter_data.defenders().members().push_back(member);
    encounter_data.atb_in().emit(
        seerin::AtbInEvent{seerin::AddCombatant{member}});
  });
  return encounter_data;
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
  encounter_data.atb_in().emit(
      seerin::AtbInEvent{seerin::AddCombatant{entity}});
}

} // namespace fl::primitives
