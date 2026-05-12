#include "special_festival_event.hpp"

#include <type_traits>

#include <fmt/format.h>

#include "fl/loot/global_loot_table.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::ecs::systems {

void SpecialFestivalEvent::grant_starting_drops(fl::context::PartyCtx &ctx) {
  auto loot = fl::loot::global_loot_table();
  std::size_t granted = 0;
  std::size_t rolls = 0;
  const auto party_id = static_cast<std::underlying_type_t<entt::entity>>(
      ctx.party_data().party_id());

  while (granted < kDropsPerParty) {
    const auto stream_name =
        fmt::format("festival.{}.{}", party_id, rolls++);
    if (auto builder = loot.roll(ctx, stream_name)) {
      ctx.party_data().add_item(builder->create(ctx.reg()));
      ++granted;
    }
  }

  ctx.log().append_markup(fmt::format(
      "[yellow](Special Festival Event): [ok]({}) drops arrived for {}.",
      granted, ctx.party_data().name()));
}

} // namespace fl::ecs::systems
