#include "fl/ecs/systems/manual_equipment.hpp"

#include <algorithm>
#include <vector>

#include "fl/ecs/components/equipment.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/effective_combat_stats.hpp"
#include "fl/primitives/party_data.hpp"

namespace fl::ecs::systems {

fl::events::ScopedPartyListener
ManualEquipmentSystem::bind_listener(fl::context::PartyCtx& party_ctx) {
  return fl::events::ScopedPartyListener{
      party_ctx.bus(), std::in_place_type<fl::events::EquipmentEquipRequested>,
      [&party_ctx](const fl::events::EquipmentEquipRequested& request) {
        (void)ManualEquipmentSystem::commit(party_ctx, request);
      }};
}

bool ManualEquipmentSystem::commit(fl::context::PartyCtx& party_ctx,
                                   const fl::events::EquipmentEquipRequested& request) {
  auto& party = party_ctx.party_data();
  auto& reg = party_ctx.reg();

  const auto member =
      std::find_if(party.members().begin(), party.members().end(),
                   [&](const auto& entry) { return entry.member_id() == request.member; });
  if (member == party.members().end() || !reg.valid(request.member) || !reg.valid(request.item)) {
    return false;
  }

  auto* party_member = reg.try_get<fl::ecs::components::PartyMember>(request.member);
  auto* equipment = reg.try_get<fl::ecs::components::Equipment>(request.item);
  if (party_member == nullptr || equipment == nullptr || equipment->slot() != request.slot) {
    return false;
  }

  auto inventory = std::vector<entt::entity>{party.items().begin(), party.items().end()};
  const auto item = std::find(inventory.begin(), inventory.end(), request.item);
  if (item == inventory.end()) {
    return false;
  }

  auto& equipped = party_member->closet().equipped_item(request.slot);
  const auto previous = equipped;
  inventory.erase(item);
  if (previous != entt::null && reg.valid(previous)) {
    inventory.push_back(previous);
  }
  equipped = request.item;
  party.replace_items(std::move(inventory));
  auto& stats = reg.get<fl::ecs::components::Stats>(request.member);
  stats.hp_ = std::min(stats.hp_, effective_combat_stats(reg, request.member).max_hp);

  party_ctx.bus().emit(fl::events::PartyEvent{
      fl::events::EquipmentEquipped{request.member, request.slot, request.item, previous}});
  return true;
}

} // namespace fl::ecs::systems
