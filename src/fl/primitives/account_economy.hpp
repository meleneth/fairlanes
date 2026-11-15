#pragma once

#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>

#include "fl/consumables/consumable_kind.hpp"
#include "fl/economy/material.hpp"
#include "fl/economy/recipe.hpp"
#include "fl/skills/skill.hpp"

namespace fl::primitives {
class PartyData;

enum class ProductionObjectiveKind { MaterialStockpile, ConsumableStockpile, CraftedEquipment, SkillStudy };

struct ProductionObjective {
  ProductionObjectiveKind kind{ProductionObjectiveKind::MaterialStockpile};
  std::optional<fl::economy::Material> material;
  std::optional<fl::consumables::ConsumableKind> consumable;
  std::optional<fl::economy::RecipeId> equipment_recipe;
  entt::entity equipment_recipient{entt::null};
  entt::entity student{entt::null};
  fl::skills::SkillKey skill{fl::skills::SkillId::Observe};
  int target_quantity{0};
  bool completed{false};
  bool equipment_attempted{false};
  std::string blocked_reason;
};

struct ProductionMetrics {
  int materials_acquired{0};
  int materials_consumed{0};
  int products_created{0};
  int profession_xp_gained{0};
  int study_beats{0};
  int blocked_operations{0};
  int items_distributed{0};
  int production_beats{0};
  int tower_progression_beats_forgone{0};
};

class AccountEconomy {
public:
  [[nodiscard]] int material_count(fl::economy::Material material) const noexcept {
    const auto found = materials_.find(material);
    return found == materials_.end() ? 0 : found->second;
  }
  void add_material(fl::economy::Material material, int quantity) noexcept;
  [[nodiscard]] bool has_material(fl::economy::Material material, int quantity) const noexcept;
  [[nodiscard]] bool consume_material(fl::economy::Material material, int quantity) noexcept;

  void deposit_item(entt::entity item);
  [[nodiscard]] std::span<const entt::entity> items() const noexcept { return items_; }
  bool distribute_item(PartyData& party, entt::entity item);

  void set_objective(ProductionObjective objective) { objective_ = std::move(objective); }
  void clear_objective() noexcept { objective_.reset(); }
  ProductionObjective* objective() noexcept { return objective_ ? &*objective_ : nullptr; }
  const ProductionObjective* objective() const noexcept { return objective_ ? &*objective_ : nullptr; }
  void set_blocked(std::string reason);
  void reevaluate_objective(const entt::registry& reg);

  ProductionMetrics& metrics() noexcept { return metrics_; }
  const ProductionMetrics& metrics() const noexcept { return metrics_; }

private:
  // Material IDs are stable generated values, not dense declaration ordinals.
  std::map<fl::economy::Material, int> materials_;
  std::vector<entt::entity> items_;
  std::optional<ProductionObjective> objective_;
  ProductionMetrics metrics_{};
};

} // namespace fl::primitives
