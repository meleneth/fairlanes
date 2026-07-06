#include "fl/primitives/farming_plan.hpp"

#include <algorithm>

namespace fl::primitives {
namespace {
constexpr std::array<FarmFocusDefinition, 5> kFarmFocusDefinitions{{
    {FarmFocus::Brawn, "Brawn", "Train plate discipline growth."},
    {FarmFocus::Cunning, "Cunning", "Train leather discipline growth."},
    {FarmFocus::Wisdom, "Wisdom", "Train cloth discipline growth."},
    {FarmFocus::WealthMaterials, "Wealth & Materials",
     "Gather economy and crafting materials."},
    {FarmFocus::Gear, "Gear", "Seek equipment and maintenance rewards."},
}};
} // namespace

std::span<const FarmFocusDefinition> all_farm_focus_definitions() noexcept {
  return kFarmFocusDefinitions;
}

std::string_view display_name(GrimoireDiscipline discipline) noexcept {
  switch (discipline) {
  case GrimoireDiscipline::Brawn:
    return "Brawn";
  case GrimoireDiscipline::Cunning:
    return "Cunning";
  case GrimoireDiscipline::Wisdom:
    return "Wisdom";
  }
  return "Unknown Discipline";
}

std::string_view display_name(FarmFocus focus) noexcept {
  const auto definitions = all_farm_focus_definitions();
  const auto it = std::find_if(definitions.begin(), definitions.end(),
                               [focus](const FarmFocusDefinition &definition) {
                                 return definition.focus == focus;
                               });
  return it == definitions.end() ? "Unknown Farm Focus" : it->display_name;
}

std::string_view display_name(FarmRewardClass reward_class) noexcept {
  switch (reward_class) {
  case FarmRewardClass::AlignedDiscipline:
    return "Aligned discipline";
  case FarmRewardClass::CrossDiscipline:
    return "Cross-discipline breakthrough";
  case FarmRewardClass::WealthMaterials:
    return "Wealth & materials";
  case FarmRewardClass::Gear:
    return "Gear";
  }
  return "Unknown Farm Reward";
}

std::string_view display_name(ProgressionControlMode mode) noexcept {
  switch (mode) {
  case ProgressionControlMode::Auto:
    return "Auto";
  case ProgressionControlMode::Guided:
    return "Guided";
  case ProgressionControlMode::Manual:
    return "Manual";
  }
  return "Unknown Progression Control";
}

bool is_discipline_focus(FarmFocus focus) noexcept {
  switch (focus) {
  case FarmFocus::Brawn:
  case FarmFocus::Cunning:
  case FarmFocus::Wisdom:
    return true;
  case FarmFocus::WealthMaterials:
  case FarmFocus::Gear:
    return false;
  }
  return false;
}

GrimoireDiscipline discipline_for_focus(FarmFocus focus) noexcept {
  switch (focus) {
  case FarmFocus::Brawn:
    return GrimoireDiscipline::Brawn;
  case FarmFocus::Cunning:
    return GrimoireDiscipline::Cunning;
  case FarmFocus::Wisdom:
    return GrimoireDiscipline::Wisdom;
  case FarmFocus::WealthMaterials:
  case FarmFocus::Gear:
    return GrimoireDiscipline::Brawn;
  }
  return GrimoireDiscipline::Brawn;
}

FarmRewardClass classify_farming_plan(GrimoireDiscipline discipline,
                                      FarmFocus focus) noexcept {
  switch (focus) {
  case FarmFocus::WealthMaterials:
    return FarmRewardClass::WealthMaterials;
  case FarmFocus::Gear:
    return FarmRewardClass::Gear;
  case FarmFocus::Brawn:
  case FarmFocus::Cunning:
  case FarmFocus::Wisdom:
    return discipline_for_focus(focus) == discipline
               ? FarmRewardClass::AlignedDiscipline
               : FarmRewardClass::CrossDiscipline;
  }
  return FarmRewardClass::AlignedDiscipline;
}

FarmingPlan make_farming_plan(GrimoireDiscipline discipline,
                              FarmFocus focus) noexcept {
  FarmingPlan plan;
  plan.discipline = discipline;
  plan.focus = focus;
  plan.reward_class = classify_farming_plan(discipline, focus);

  switch (plan.reward_class) {
  case FarmRewardClass::AlignedDiscipline:
    plan.grimoire_progress_weight = 100;
    break;
  case FarmRewardClass::CrossDiscipline:
    plan.grimoire_progress_weight = 45;
    plan.breakthrough_progress_weight = 55;
    break;
  case FarmRewardClass::WealthMaterials:
    plan.grimoire_progress_weight = 15;
    plan.economy_progress_weight = 85;
    break;
  case FarmRewardClass::Gear:
    plan.grimoire_progress_weight = 15;
    plan.gear_progress_weight = 85;
    break;
  }

  return plan;
}

FarmFocus recommended_farm_focus(GrimoireDiscipline discipline) noexcept {
  switch (discipline) {
  case GrimoireDiscipline::Brawn:
    return FarmFocus::Brawn;
  case GrimoireDiscipline::Cunning:
    return FarmFocus::Cunning;
  case GrimoireDiscipline::Wisdom:
    return FarmFocus::Wisdom;
  }
  return FarmFocus::Brawn;
}

FarmingChoiceAdvice
make_farming_choice_advice(GrimoireDiscipline discipline,
                           std::optional<FarmFocus> previous_focus) noexcept {
  FarmingChoiceAdvice advice;
  advice.discipline = discipline;
  advice.recommended_focus = recommended_farm_focus(discipline);
  advice.previous_focus = previous_focus;
  advice.recommendation_reason = "Autotree recommends the aligned discipline "
                                 "farm for efficient grimoire growth.";
  return advice;
}

bool plan_is_aligned(const FarmingPlan &plan) noexcept {
  return plan.reward_class == FarmRewardClass::AlignedDiscipline;
}

bool plan_is_cross_discipline(const FarmingPlan &plan) noexcept {
  return plan.reward_class == FarmRewardClass::CrossDiscipline;
}

} // namespace fl::primitives
