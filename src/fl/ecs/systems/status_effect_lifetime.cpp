#include "fl/ecs/systems/status_effect_lifetime.hpp"

#include "fl/primitives/party_data.hpp"

namespace fl::ecs::systems {

fl::ecs::components::StatusEffectInstance
StatusEffectLifetime::create_instance(fl::context::PartyCtx &party_ctx,
                                      entt::entity owner) {
  return fl::ecs::components::StatusEffectInstance{
      .owner = owner,
      .effect_id = party_ctx.reg().create(),
      .owner_died_sub = {},
      .party_left_combat_sub = {},
      .party_wiped_sub = {},
  };
}

StatusEffectLifetime::StatusEffectLifetime(
    fl::context::PartyCtx &party_ctx, Scheduler &scheduler,
    fl::ecs::components::StatusEffectInstance &instance)
    : party_ctx_(party_ctx), scheduler_(scheduler), instance_(instance) {}

entt::entity StatusEffectLifetime::owner() const noexcept {
  return instance_.owner;
}

entt::entity StatusEffectLifetime::effect_id() const noexcept {
  return instance_.effect_id;
}

void StatusEffectLifetime::schedule_in_beats(int beats, std::string_view note,
                                             std::function<void()> fn) {
  scheduler_.schedule_smelly_in_beats_for(beats, instance_.effect_id, note,
                                          std::move(fn));
}

void StatusEffectLifetime::clear_scheduled() {
  scheduler_.clear_smelly_callbacks_for(instance_.effect_id);
}

void StatusEffectLifetime::destroy_instance_entity() {
  auto &reg = party_ctx_.reg();
  if (reg.valid(instance_.effect_id)) {
    reg.destroy(instance_.effect_id);
  }
  instance_.effect_id = entt::null;
}

fl::events::CombatantBus &StatusEffectLifetime::owner_bus() {
  return party_ctx_.party_data().encounter_data().combatant_bus(
      instance_.owner);
}

} // namespace fl::ecs::systems
