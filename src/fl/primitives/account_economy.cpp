#include "fl/primitives/account_economy.hpp"

#include <algorithm>

#include <entt/entity/registry.hpp>

#include "fl/ecs/components/consumable.hpp"
#include "fl/ecs/components/crafted_provenance.hpp"
#include "fl/ecs/components/equipment.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/generated/equipment_policy.hpp"
#include "fl/loot/equipment_stat_policy.hpp"
#include "fl/primitives/party_data.hpp"

namespace fl::primitives {

void AccountEconomy::add_material(fl::economy::Material material, int quantity) noexcept {
  if (quantity <= 0) return;
  materials_[material] += quantity;
  metrics_.materials_acquired += quantity;
}

bool AccountEconomy::has_material(fl::economy::Material material, int quantity) const noexcept {
  return quantity >= 0 && material_count(material) >= quantity;
}

bool AccountEconomy::consume_material(fl::economy::Material material, int quantity) noexcept {
  if (quantity < 0 || !has_material(material, quantity)) return false;
  materials_[material] -= quantity;
  metrics_.materials_consumed += quantity;
  return true;
}

void AccountEconomy::deposit_item(entt::entity item) {
  if (item == entt::null || std::ranges::find(items_, item) != items_.end()) return;
  items_.push_back(item);
  ++metrics_.products_created;
}

bool AccountEconomy::distribute_item(PartyData& party, entt::entity item) {
  const auto found = std::ranges::find(items_, item);
  if (found == items_.end()) return false;
  party.add_item(item);
  items_.erase(found);
  ++metrics_.items_distributed;
  return true;
}

void AccountEconomy::set_blocked(std::string reason) {
  if (!objective_ || objective_->completed || objective_->blocked_reason == reason) return;
  objective_->blocked_reason = std::move(reason);
  ++metrics_.blocked_operations;
}

void AccountEconomy::reevaluate_objective(const entt::registry& reg) {
  if (!objective_ || objective_->completed) return;
  auto& objective = *objective_;
  switch (objective.kind) {
  case ProductionObjectiveKind::MaterialStockpile:
    objective.completed = objective.material &&
                          material_count(*objective.material) >= objective.target_quantity;
    break;
  case ProductionObjectiveKind::ConsumableStockpile: {
    int count = 0;
    if (objective.consumable)
      for (const auto item : items_)
        if (const auto* value = reg.try_get<fl::ecs::components::Consumable>(item);
            value && value->kind == *objective.consumable)
          count += value->quantity;
    objective.completed = count >= objective.target_quantity;
    break;
  }
  case ProductionObjectiveKind::CraftedEquipment:
    if (objective.equipment_recipe && objective.equipment_recipient != entt::null &&
        reg.valid(objective.equipment_recipient)) {
      const auto* member =
          reg.try_get<fl::ecs::components::PartyMember>(objective.equipment_recipient);
      objective.completed =
          member && std::ranges::any_of(items_, [&](entt::entity item) {
            const auto* provenance = reg.try_get<fl::ecs::components::CraftedProvenance>(item);
            const auto* equipment = reg.try_get<fl::ecs::components::Equipment>(item);
            if (!provenance || !equipment || provenance->recipe != objective.equipment_recipe)
              return false;
            const auto current_id = member->closet().equipped_item(equipment->slot());
            const auto* current = current_id == entt::null
                                      ? nullptr
                                      : reg.try_get<fl::ecs::components::Equipment>(current_id);
            const int current_value = current ? fl::loot::equipment_combat_value(*current) : 0;
            return fl::loot::equipment_combat_value(*equipment) >=
                   current_value +
                       fl::loot::generated_content::kMinimumAutonomousEquipmentImprovement;
          });
    }
    break;
  case ProductionObjectiveKind::SkillStudy:
    // Completion is committed by ProductionSystem after the Grimoire learns it.
    break;
  }
  if (objective.completed) objective.blocked_reason.clear();
}

} // namespace fl::primitives
