#include "fl/ecs/systems/raid_loot.hpp"
#include "fl/ecs/components/raid_trinket.hpp"
#include "fl/primitives/party_data.hpp"

namespace fl::ecs::systems {
void RaidLootSystem::commit(
    fl::context::AccountCtx &ctx, fl::events::RaidBus &bus,
    std::uint64_t raid_id,
    std::span<fl::primitives::PartyData *const> participants) {
  std::size_t awarded = 0;
  for (auto *party : participants) {
    for (int i = 0; i < kDropsPerParty; ++i) {
      auto item = ctx.reg().create();
      ctx.reg().emplace<fl::ecs::components::RaidTrinket>(item, raid_id,
                                                          ctx.self());
      party->add_item(item);
      ++awarded;
    }
    party->log().append_plain(
        "Raid victory: two raid-exclusive trinkets awarded.");
  }
  // Count actual inventory grants, never loot requests or ordinary kill drops.
  bus.emit(fl::events::RaidLootAwarded{raid_id, ctx.self(), awarded});
}
} // namespace fl::ecs::systems
