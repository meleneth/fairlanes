#include "fl/ecs/systems/loot_drop.hpp"

#include <ftxui/dom/elements.hpp>

#include "fl/ecs/components/equipment.hpp"
#include "fl/loot/global_loot_table.hpp"
#include "fl/loot/unique_items.hpp"
#include "fl/lospec500.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/widgets/equipment_label.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::ecs::systems {

fl::events::ScopedPartyListener
LootDropSystem::bind_listener(fl::context::PartyCtx &party_ctx) {
  return fl::events::ScopedPartyListener{
      party_ctx.bus(), std::in_place_type<fl::events::LootDropRequested>,
      [&party_ctx](const fl::events::LootDropRequested &ev) {
        if (ev.party != party_ctx.self()) {
          return;
        }

        LootDropSystem::commit(party_ctx, ev.source);
      }};
}

void LootDropSystem::commit(fl::context::PartyCtx &party_ctx,
                            entt::entity source) {
  if (source == entt::null || !party_ctx.reg().valid(source)) {
    return;
  }

  auto builder = fl::loot::global_loot_table().roll(party_ctx, "loot.global");
  if (!builder) {
    return;
  }

  auto &party_data = party_ctx.party_data();
  if (fl::loot::inventory_has_unique_item(party_ctx.reg(), party_data.items(),
                                          builder->unique_id)) {
    return;
  }

  auto item = builder->create(party_ctx.reg());
  const auto &equipment =
      party_ctx.reg().get<fl::ecs::components::Equipment>(item);

  party_ctx.log().append(ftxui::hbox({
      ftxui::text("Loot found: ") | fl::lospec500::at(19),
      fl::widgets::equipment_name_label(equipment),
  }));
  party_data.add_item(item);
}

} // namespace fl::ecs::systems
