#include "fl/ecs/systems/loot_drop.hpp"

#include <ftxui/dom/elements.hpp>
#include <optional>

#include "fl/ecs/components/equipment.hpp"
#include "fl/generated/gear_content.hpp"
#include "fl/economy/material.hpp"
#include "fl/loot/global_loot_table.hpp"
#include "fl/loot/unique_items.hpp"
#include "fl/lospec500.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/widgets/equipment_label.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::ecs::systems {
namespace {
std::optional<fl::economy::Material> combat_material(std::string_view biome) {
  if (biome == "origin_woodland_tower") return fl::economy::Material::WoodlandHide;
  if (biome == "origin_plains_tower") return fl::economy::Material::PlainsLeather;
  if (biome == "origin_murky_swamp_tower") return fl::economy::Material::SwampReagent;
  if (biome == "origin_caves_tower") return fl::economy::Material::CaveOre;
  if (biome == "origin_desert_tower") return fl::economy::Material::DesertSalt;
  return std::nullopt;
}

bool grant_builder(fl::context::PartyCtx& ctx, const fl::loot::EquipmentBuilder& builder) {
  auto& party = ctx.party_data();
  if (fl::loot::inventory_has_unique_item(ctx.reg(), party.items(), builder.unique_id))
    return false;
  const auto item = builder.create(ctx.reg());
  const auto& equipment = ctx.reg().get<fl::ecs::components::Equipment>(item);
  ctx.log().append(ftxui::hbox({
      ftxui::text("Loot found: ") | fl::lospec500::at(19),
      fl::widgets::equipment_name_label(equipment),
  }));
  party.add_item(item);
  return true;
}
} // namespace

fl::events::ScopedPartyListener LootDropSystem::bind_listener(fl::context::PartyCtx& party_ctx) {
  return fl::events::ScopedPartyListener{party_ctx.bus(),
                                         std::in_place_type<fl::events::LootDropRequested>,
                                         [&party_ctx](const fl::events::LootDropRequested& ev) {
                                           if (ev.party != party_ctx.self()) {
                                             return;
                                           }

                                           LootDropSystem::commit(party_ctx, ev.source);
                                         }};
}

void LootDropSystem::commit(fl::context::PartyCtx& party_ctx, entt::entity source) {
  if (source == entt::null || !party_ctx.reg().valid(source)) {
    return;
  }

  if (const auto material = combat_material(party_ctx.party_data().encounter_biome())) {
    party_ctx.account_data().economy().add_material(*material, 1);
    party_ctx.bus().emit(fl::events::PartyEvent{
        fl::events::MaterialGathered{entt::null, *material, 1, 0}});
  }

  auto builder = fl::loot::global_loot_table().roll(party_ctx, "loot.global",
                                                    party_ctx.party_data().encounter_biome());
  if (!builder) {
    return;
  }

  (void)grant_builder(party_ctx, *builder);
}

bool LootDropSystem::grant_unique(fl::context::PartyCtx& ctx,
                                  fl::loot::generated_content::GearId item) {
  return grant_builder(ctx, fl::loot::generated_content::definition(item).item);
}

} // namespace fl::ecs::systems
