#pragma once

#include <array>
#include <optional>
#include <span>
#include <string_view>

namespace fl::primitives {

enum class GrimoireDiscipline {
  Brawn,
  Cunning,
  Wisdom,
};

enum class FarmFocus {
  Brawn,
  Cunning,
  Wisdom,
  WealthMaterials,
  Gear,
};

enum class FarmRewardClass {
  AlignedDiscipline,
  CrossDiscipline,
  WealthMaterials,
  Gear,
};

enum class ProgressionControlMode {
  Auto,
  Guided,
  Manual,
};

struct FarmFocusDefinition {
  FarmFocus focus;
  std::string_view display_name;
  std::string_view description;
};

struct FarmingPlan {
  GrimoireDiscipline discipline{GrimoireDiscipline::Brawn};
  FarmFocus focus{FarmFocus::Brawn};
  FarmRewardClass reward_class{FarmRewardClass::AlignedDiscipline};
  int grimoire_progress_weight{0};
  int breakthrough_progress_weight{0};
  int economy_progress_weight{0};
  int gear_progress_weight{0};
};

struct FarmingChoiceAdvice {
  GrimoireDiscipline discipline{GrimoireDiscipline::Brawn};
  FarmFocus recommended_focus{FarmFocus::Brawn};
  std::string_view recommendation_reason;
  std::optional<FarmFocus> previous_focus;
};

std::span<const FarmFocusDefinition> all_farm_focus_definitions() noexcept;
std::string_view display_name(GrimoireDiscipline discipline) noexcept;
std::string_view display_name(FarmFocus focus) noexcept;
std::string_view display_name(FarmRewardClass reward_class) noexcept;
std::string_view display_name(ProgressionControlMode mode) noexcept;
bool is_discipline_focus(FarmFocus focus) noexcept;
GrimoireDiscipline discipline_for_focus(FarmFocus focus) noexcept;
FarmRewardClass classify_farming_plan(GrimoireDiscipline discipline,
                                      FarmFocus focus) noexcept;
FarmingPlan make_farming_plan(GrimoireDiscipline discipline,
                              FarmFocus focus) noexcept;
FarmFocus recommended_farm_focus(GrimoireDiscipline discipline) noexcept;
FarmingChoiceAdvice make_farming_choice_advice(
    GrimoireDiscipline discipline,
    std::optional<FarmFocus> previous_focus = std::nullopt) noexcept;
bool plan_is_aligned(const FarmingPlan &plan) noexcept;
bool plan_is_cross_discipline(const FarmingPlan &plan) noexcept;

} // namespace fl::primitives
