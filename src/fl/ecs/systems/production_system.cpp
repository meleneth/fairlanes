#include "fl/ecs/systems/production_system.hpp"

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#include <fmt/format.h>

#include "fl/ecs/components/combat_affinity.hpp"
#include "fl/ecs/components/consumable.hpp"
#include "fl/ecs/components/crafted_provenance.hpp"
#include "fl/ecs/components/equipment.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/profession_expertise.hpp"
#include "fl/ecs/components/skill_study.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/ecs/systems/consumable_system.hpp"
#include "fl/generated/equipment_policy.hpp"
#include "fl/generated/production_content.hpp"
#include "fl/loot/equipment_builder.hpp"
#include "fl/loot/equipment_stat_policy.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/account_economy.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/world_clock.hpp"
#include "fl/skills/grimoire.hpp"
#include "fl/skills/skill_definition.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::ecs::systems {
namespace {

using fl::economy::generated_content::MaterialDefinition;
using fl::economy::generated_content::RecipeDefinition;

entt::entity choose_worker(fl::context::PartyCtx ctx, fl::economy::Profession profession,
                           int required_proficiency) {
  entt::entity selected{entt::null};
  int best = -1;
  for (const auto& member : ctx.party_data().members()) {
    const auto entity = member.member_id();
    const auto* stats = ctx.reg().try_get<fl::ecs::components::Stats>(entity);
    const auto* expertise = ctx.reg().try_get<fl::ecs::components::ProfessionExpertise>(entity);
    if (!stats || !stats->is_alive() || !expertise) continue;
    const int proficiency = expertise->proficiency(profession);
    if (proficiency < required_proficiency || proficiency <= best) continue;
    selected = entity;
    best = proficiency;
  }
  return selected;
}

void blocked(fl::context::PartyCtx ctx, std::string reason) {
  ctx.party_data().set_production_blocked(reason);
  ctx.account_data().economy().set_blocked(reason);
  ctx.bus().emit(fl::events::PartyEvent{fl::events::ProductionBlocked{std::move(reason)}});
}

void grant_xp(fl::context::PartyCtx ctx, entt::entity worker,
              fl::economy::Profession profession, int amount, int trivial_after) {
  auto& expertise = ctx.reg().get<fl::ecs::components::ProfessionExpertise>(worker);
  if (expertise.proficiency(profession) > trivial_after) amount = 0;
  const int granted = expertise.grant(profession, amount);
  if (granted <= 0) return;
  ctx.account_data().economy().metrics().profession_xp_gained += granted;
  ctx.bus().emit(fl::events::PartyEvent{
      fl::events::ProfessionExperienceGained{worker, profession, granted}});
}

bool commit_inputs(fl::context::PartyCtx ctx, const RecipeDefinition& recipe) {
  auto& economy = ctx.account_data().economy();
  for (const auto& input : recipe.inputs)
    if (!economy.has_material(input.material, input.quantity)) {
      blocked(ctx, fmt::format("{} needs {} {}", recipe.display_name, input.quantity,
                               fl::economy::stable_id(input.material)));
      return false;
    }
  int total = 0;
  for (const auto& input : recipe.inputs) {
    (void)economy.consume_material(input.material, input.quantity);
    total += input.quantity;
  }
  ctx.bus().emit(fl::events::PartyEvent{fl::events::MaterialsConsumed{recipe.id, total}});
  return true;
}

bool armor_compatible(entt::registry& reg, entt::entity member,
                      fl::loot::ArmorKind armor_kind) {
  if (armor_kind == fl::loot::ArmorKind::none) return true;
  const auto* affinity = reg.try_get<fl::ecs::components::ActiveCombatAffinity>(member);
  if (!affinity) return false;
  switch (affinity->value) {
  case fl::primitives::CombatAffinity::Cloth: return armor_kind == fl::loot::ArmorKind::cloth;
  case fl::primitives::CombatAffinity::Leather:
    return armor_kind == fl::loot::ArmorKind::leather;
  case fl::primitives::CombatAffinity::Plate: return armor_kind == fl::loot::ArmorKind::plate;
  case fl::primitives::CombatAffinity::Neutral: return false;
  }
  return false;
}

struct EquipmentCraftCandidate {
  const RecipeDefinition* recipe{};
  entt::entity recipient{entt::null};
  int improvement{0};
  bool inputs_ready{false};
};

bool ranks_before(const EquipmentCraftCandidate& candidate,
                  const EquipmentCraftCandidate& current) {
  if (candidate.improvement != current.improvement)
    return candidate.improvement > current.improvement;
  if (candidate.inputs_ready != current.inputs_ready) return candidate.inputs_ready;
  if (candidate.recipe->duration_seconds != current.recipe->duration_seconds)
    return candidate.recipe->duration_seconds < current.recipe->duration_seconds;
  const auto candidate_id = fl::economy::stable_id(candidate.recipe->id);
  const auto current_id = fl::economy::stable_id(current.recipe->id);
  if (candidate_id != current_id) return candidate_id < current_id;
  return entt::to_integral(candidate.recipient) < entt::to_integral(current.recipient);
}

std::optional<EquipmentCraftCandidate> best_equipment_candidate(fl::context::PartyCtx ctx,
                                                                std::string& blocked_reason) {
  std::optional<EquipmentCraftCandidate> best;
  bool saw_equipment_recipe = false;
  bool saw_crafter = false;
  bool saw_compatible = false;
  bool saw_accessible_inputs = false;
  for (const auto& recipe : fl::economy::generated_content::kRecipes) {
    if (recipe.product_kind != fl::economy::generated_content::ProductKind::Equipment) continue;
    saw_equipment_recipe = true;
    if (choose_worker(ctx, recipe.profession, recipe.required_proficiency) == entt::null) continue;
    saw_crafter = true;

    bool inputs_accessible = true;
    bool inputs_ready = true;
    for (const auto& input : recipe.inputs) {
      if (ctx.account_data().economy().has_material(input.material, input.quantity)) continue;
      inputs_ready = false;
      const auto* material = fl::economy::generated_content::definition(input.material);
      if (!material || !ctx.party_data().biome_unlocked(material->biome_id) ||
          choose_worker(ctx, material->profession, 0) == entt::null) {
        inputs_accessible = false;
        break;
      }
    }
    if (!inputs_accessible) continue;
    saw_accessible_inputs = true;

    for (const auto& member_data : ctx.party_data().members()) {
      const auto member = member_data.member_id();
      const auto* party_member = ctx.reg().try_get<fl::ecs::components::PartyMember>(member);
      const auto* stats = ctx.reg().try_get<fl::ecs::components::Stats>(member);
      if (!party_member || !stats || !stats->is_alive() ||
          !armor_compatible(ctx.reg(), member, recipe.armor_kind))
        continue;
      const auto* xp = ctx.reg().try_get<fl::ecs::components::TrackXP>(member);
      if (fl::loot::equipment_minimum_level(recipe.tier) > (xp ? xp->level_ : 1)) continue;
      saw_compatible = true;
      const auto current_id = party_member->closet().equipped_item(recipe.slot);
      const auto* current = current_id == entt::null
                                ? nullptr
                                : ctx.reg().try_get<fl::ecs::components::Equipment>(current_id);
      const int current_value = current ? fl::loot::equipment_combat_value(*current) : 0;
      const int improvement = fl::loot::minimum_generated_combat_value(
                                  recipe.slot, recipe.tier, recipe.biome_id) -
                              current_value;
      if (improvement <
          fl::loot::generated_content::kMinimumAutonomousEquipmentImprovement)
        continue;
      EquipmentCraftCandidate candidate{&recipe, member, improvement, inputs_ready};
      if (!best || ranks_before(candidate, *best)) best = candidate;
    }
  }
  if (best) return best;
  if (!saw_equipment_recipe) blocked_reason = "no declared equipment recipes";
  else if (!saw_crafter) blocked_reason = "no eligible equipment crafter";
  else if (!saw_accessible_inputs) blocked_reason = "required equipment materials are unavailable";
  else if (!saw_compatible) blocked_reason = "no compatible equipment recipient";
  else blocked_reason = "no meaningful craftable equipment improvement";
  return std::nullopt;
}

void start_gathering(fl::context::PartyCtx ctx, std::uint64_t world_beat) {
  const auto& assignment = ctx.party_data().activity_assignment();
  if (!assignment.material) return blocked(ctx, "no gathering material selected");
  const auto* definition = fl::economy::generated_content::definition(*assignment.material);
  if (!definition) return blocked(ctx, "unknown gathering material");
  if (definition->profession != assignment.profession)
    return blocked(ctx, "selected profession cannot gather that material");
  if (!ctx.party_data().biome_unlocked(definition->biome_id))
    return blocked(ctx, fmt::format("{} is locked", definition->biome_id));
  const auto worker = choose_worker(ctx, definition->profession, 0);
  if (worker == entt::null) return blocked(ctx, "no living eligible gatherer");
  const auto duration = fl::primitives::WorldClock::beats_from_seconds(definition->duration_seconds);
  ctx.party_data().production_operation() = fl::primitives::ProductionOperation{
      .activity = fl::primitives::PartyActivity::Gathering,
      .profession = definition->profession,
      .worker = worker,
      .material = definition->id,
      .started_world_beat = world_beat,
      .completes_world_beat = world_beat + static_cast<std::uint64_t>(duration)};
  ctx.party_data().clear_production_blocked();
}

void start_crafting(fl::context::PartyCtx ctx, std::uint64_t world_beat) {
  const auto& assignment = ctx.party_data().activity_assignment();
  if (!assignment.recipe) return blocked(ctx, "no recipe selected");
  const auto* recipe = fl::economy::generated_content::definition(*assignment.recipe);
  if (!recipe) return blocked(ctx, "unknown recipe");
  const auto worker = choose_worker(ctx, recipe->profession, recipe->required_proficiency);
  if (worker == entt::null)
    return blocked(ctx, fmt::format("{} requires {} proficiency {}", recipe->display_name,
                                    fl::economy::display_name(recipe->profession),
                                    recipe->required_proficiency));
  if (!commit_inputs(ctx, *recipe)) return;
  const auto duration = fl::primitives::WorldClock::beats_from_seconds(recipe->duration_seconds);
  ctx.party_data().production_operation() = fl::primitives::ProductionOperation{
      .activity = fl::primitives::PartyActivity::Crafting,
      .profession = recipe->profession,
      .worker = worker,
      .recipe = recipe->id,
      .started_world_beat = world_beat,
      .completes_world_beat = world_beat + static_cast<std::uint64_t>(duration)};
  ctx.party_data().clear_production_blocked();
}

void start_study(fl::context::PartyCtx ctx, std::uint64_t world_beat) {
  const auto& assignment = ctx.party_data().activity_assignment();
  if (assignment.student == entt::null || !ctx.party_data().members().contains(assignment.student))
    return blocked(ctx, "student is not assigned to this party");
  auto* study = ctx.reg().try_get<fl::ecs::components::SkillStudy>(assignment.student);
  auto* expertise = ctx.reg().try_get<fl::ecs::components::ProfessionExpertise>(assignment.student);
  auto* member = ctx.reg().try_get<fl::ecs::components::PartyMember>(assignment.student);
  const auto& policy = fl::skills::definition(assignment.skill).study;
  if (!policy.required || !study || !study->find(assignment.skill) || !expertise || !member)
    return blocked(ctx, "skill has not been successfully observed for study");
  if (expertise->proficiency(fl::economy::Profession::Study) < policy.required_proficiency)
    return blocked(ctx, "Study proficiency is too low");
  const int total = fl::primitives::WorldClock::beats_from_seconds(policy.duration_seconds);
  const int remaining = std::max(1, total - study->find(assignment.skill)->progress_beats);
  ctx.party_data().production_operation() = fl::primitives::ProductionOperation{
      .activity = fl::primitives::PartyActivity::SkillStudy,
      .profession = fl::economy::Profession::Study,
      .worker = assignment.student,
      .skill = assignment.skill,
      .started_world_beat = world_beat,
      .completes_world_beat = world_beat + static_cast<std::uint64_t>(remaining)};
  ctx.party_data().clear_production_blocked();
}

void complete_gathering(fl::context::PartyCtx ctx,
                        const fl::primitives::ProductionOperation& operation) {
  const auto* definition = fl::economy::generated_content::definition(*operation.material);
  if (!definition) return;
  auto& economy = ctx.account_data().economy();
  economy.add_material(definition->id, definition->quantity);
  grant_xp(ctx, operation.worker, definition->profession, definition->experience,
           definition->tier * 10 + 10);
  ctx.bus().emit(fl::events::PartyEvent{fl::events::MaterialGathered{
      operation.worker, definition->id, definition->quantity,
      fl::primitives::WorldClock::beats_from_seconds(definition->duration_seconds)}});
  ctx.log().append_plain(fmt::format("Gathered {} x{} for the account.", definition->display_name,
                                     definition->quantity));
}

entt::entity create_product(fl::context::PartyCtx ctx, const RecipeDefinition& recipe,
                            entt::entity worker, std::uint64_t operation_beat) {
  entt::entity item{entt::null};
  if (recipe.product_kind == fl::economy::generated_content::ProductKind::Consumable) {
    item = ConsumableSystem::create(ctx.reg(), recipe.consumable, recipe.output_quantity);
  } else {
    fl::loot::EquipmentBuilder builder{.slot = recipe.slot,
                                       .armor_kind = recipe.armor_kind,
                                       .tier = recipe.tier,
                                       .name = std::string(recipe.display_name),
                                       .biome_id = std::string(recipe.biome_id)};
    const auto subsequence = operation_beat ^ static_cast<std::uint64_t>(entt::to_integral(ctx.self()));
    fl::loot::apply_ordinary_equipment_stats(builder, ctx.rng(), recipe.biome_id,
                                             fmt::format("production/{}#{}", fl::economy::stable_id(recipe.id), subsequence));
    item = builder.create(ctx.reg());
  }
  const auto* stats = ctx.reg().try_get<fl::ecs::components::Stats>(worker);
  const auto proficiency = ctx.reg()
                               .get<fl::ecs::components::ProfessionExpertise>(worker)
                               .proficiency(recipe.profession);
  std::vector<fl::ecs::components::MaterialAmount> materials;
  for (const auto& input : recipe.inputs) materials.push_back({input.material, input.quantity});
  ctx.reg().emplace<fl::ecs::components::CraftedProvenance>(
      item, worker, stats ? stats->name_ : "Unknown", recipe.profession, recipe.id, proficiency,
      std::move(materials));
  return item;
}

void complete_crafting(fl::context::PartyCtx ctx,
                       const fl::primitives::ProductionOperation& operation) {
  const auto* recipe = fl::economy::generated_content::definition(*operation.recipe);
  if (!recipe) return;
  const auto item = create_product(ctx, *recipe, operation.worker, operation.started_world_beat);
  ctx.account_data().economy().deposit_item(item);
  if (auto* objective = ctx.account_data().economy().objective();
      objective && objective->kind == fl::primitives::ProductionObjectiveKind::CraftedEquipment &&
      objective->equipment_recipe == recipe->id)
    objective->equipment_attempted = true;
  grant_xp(ctx, operation.worker, recipe->profession, recipe->experience,
           recipe->required_proficiency + 10);
  ctx.bus().emit(fl::events::PartyEvent{
      fl::events::ProductCrafted{operation.worker, recipe->id, item}});
  ctx.log().append_plain(fmt::format("Crafted {} for the account inventory.", recipe->display_name));
}

void complete_study(fl::context::PartyCtx ctx,
                    const fl::primitives::ProductionOperation& operation) {
  auto& member = ctx.reg().get<fl::ecs::components::PartyMember>(operation.worker);
  auto& study = ctx.reg().get<fl::ecs::components::SkillStudy>(operation.worker);
  if (!member.member_data().grimoire().learn(operation.skill)) return;
  study.complete(operation.skill);
  if (member.closet().has_open_skill_slot()) member.equip_known_skill(operation.skill);
  grant_xp(ctx, operation.worker, fl::economy::Profession::Study, 20, 100);
  ctx.bus().emit(fl::events::PartyEvent{
      fl::events::SkillStudyCompleted{operation.worker, operation.skill}});
  if (auto* objective = ctx.account_data().economy().objective();
      objective && objective->kind == fl::primitives::ProductionObjectiveKind::SkillStudy &&
      objective->student == operation.worker && objective->skill == operation.skill) {
    objective->completed = true;
    objective->blocked_reason.clear();
  }
  ctx.log().append_markup(fmt::format("[player_name]({}) completed study of [ability]({}).",
                                      member.member_data().name(),
                                      fl::skills::display_name(operation.skill)));
}

} // namespace

void ProductionSystem::evaluate_objective(fl::context::AccountCtx ctx) {
  auto& account = ctx.account_data();
  auto& economy = account.economy();
  economy.reevaluate_objective(ctx.reg());
  auto* objective = economy.objective();
  if (!objective) return;
  if (objective->completed) {
    for (auto& party : account.parties())
      if (party.activity_objective_driven() && !party.production_operation())
        party.request_activity({.activity = fl::primitives::PartyActivity::Idle}, true);
    return;
  }

  fl::primitives::PartyData* candidate = nullptr;
  for (auto& party : account.parties()) {
    if (party.progression_control_mode() != fl::primitives::ProgressionControlMode::Auto ||
        party.in_combat())
      continue;
    if (objective->kind == fl::primitives::ProductionObjectiveKind::SkillStudy &&
        party.members().contains(objective->student)) {
      candidate = &party;
      break;
    }
    if (party.activity_objective_driven()) {
      candidate = &party;
      break;
    }
    if (!candidate && party.activity_assignment().activity == fl::primitives::PartyActivity::Idle)
      candidate = &party;
  }
  if (!candidate || candidate->production_operation()) return;

  if (objective->kind == fl::primitives::ProductionObjectiveKind::MaterialStockpile &&
      objective->material) {
    const auto* material = fl::economy::generated_content::definition(*objective->material);
    if (!material || !candidate->biome_unlocked(material->biome_id)) {
      economy.set_blocked("no autonomous party can access the requested material biome");
      return;
    }
    candidate->request_activity({.activity = fl::primitives::PartyActivity::Gathering,
                                 .profession = material->profession,
                                 .biome_id = std::string(material->biome_id),
                                 .material = material->id},
                                true);
    return;
  }

  if (objective->kind == fl::primitives::ProductionObjectiveKind::ConsumableStockpile &&
      objective->consumable) {
    const auto recipe = std::ranges::find_if(
        fl::economy::generated_content::kRecipes, [&](const auto& value) {
          return value.product_kind == fl::economy::generated_content::ProductKind::Consumable &&
                 value.consumable == *objective->consumable;
        });
    if (recipe == fl::economy::generated_content::kRecipes.end()) {
      economy.set_blocked("no declared recipe produces the requested consumable");
      return;
    }
    for (const auto& input : recipe->inputs) {
      if (economy.has_material(input.material, input.quantity)) continue;
      const auto* material = fl::economy::generated_content::definition(input.material);
      if (!material || !candidate->biome_unlocked(material->biome_id)) {
        economy.set_blocked("required material biome is locked for the autonomous party");
        return;
      }
      candidate->request_activity({.activity = fl::primitives::PartyActivity::Gathering,
                                   .profession = material->profession,
                                   .biome_id = std::string(material->biome_id),
                                   .material = material->id},
                                  true);
      return;
    }
    candidate->request_activity({.activity = fl::primitives::PartyActivity::Crafting,
                                 .profession = recipe->profession,
                                 .recipe = recipe->id},
                                true);
    return;
  }

  if (objective->kind == fl::primitives::ProductionObjectiveKind::CraftedEquipment) {
    if (objective->equipment_attempted) {
      economy.set_blocked("crafted result was not a meaningful equipment improvement");
      return;
    }
    auto party_ctx = ctx.party_context(*candidate);
    std::string reason;
    const auto selected = best_equipment_candidate(party_ctx, reason);
    if (!selected) {
      economy.set_blocked(std::move(reason));
      return;
    }
    const auto* recipe = selected->recipe;
    objective->equipment_recipe = recipe->id;
    objective->equipment_recipient = selected->recipient;
    for (const auto& input : recipe->inputs) {
      if (economy.has_material(input.material, input.quantity)) continue;
      const auto* material = fl::economy::generated_content::definition(input.material);
      if (!material || !candidate->biome_unlocked(material->biome_id)) {
        economy.set_blocked("required equipment material biome is locked");
        return;
      }
      candidate->request_activity({.activity = fl::primitives::PartyActivity::Gathering,
                                   .profession = material->profession,
                                   .biome_id = std::string(material->biome_id),
                                   .material = material->id},
                                  true);
      return;
    }
    candidate->request_activity({.activity = fl::primitives::PartyActivity::Crafting,
                                 .profession = recipe->profession,
                                 .recipe = recipe->id},
                                true);
    return;
  }

  if (objective->kind == fl::primitives::ProductionObjectiveKind::SkillStudy &&
      objective->student != entt::null && candidate->members().contains(objective->student)) {
    candidate->request_activity({.activity = fl::primitives::PartyActivity::SkillStudy,
                                 .profession = fl::economy::Profession::Study,
                                 .student = objective->student,
                                 .skill = objective->skill},
                                true);
    return;
  }

  economy.set_blocked("no eligible autonomous production action");
}

void ProductionSystem::advance(fl::context::PartyCtx ctx, std::uint64_t world_beat) {
  auto& party = ctx.party_data();
  const auto activity = party.activity_assignment().activity;
  if (activity == fl::primitives::PartyActivity::Idle) return;
  ++ctx.account_data().economy().metrics().production_beats;
  ++ctx.account_data().economy().metrics().tower_progression_beats_forgone;
  if (!party.production_operation()) {
    switch (activity) {
    case fl::primitives::PartyActivity::Gathering: start_gathering(ctx, world_beat); break;
    case fl::primitives::PartyActivity::Crafting: start_crafting(ctx, world_beat); break;
    case fl::primitives::PartyActivity::SkillStudy: start_study(ctx, world_beat); break;
    case fl::primitives::PartyActivity::Idle:
    case fl::primitives::PartyActivity::CombatProgression:
    case fl::primitives::PartyActivity::TargetedFarming: break;
    }
    return;
  }

  auto& operation = *party.production_operation();
  if (operation.activity == fl::primitives::PartyActivity::SkillStudy) {
    auto& study = ctx.reg().get<fl::ecs::components::SkillStudy>(operation.worker);
    if (auto* entry = study.find(operation.skill)) ++entry->progress_beats;
    ++ctx.account_data().economy().metrics().study_beats;
  }
  if (world_beat < operation.completes_world_beat) return;

  const auto completed = operation;
  party.production_operation().reset();
  switch (completed.activity) {
  case fl::primitives::PartyActivity::Gathering: complete_gathering(ctx, completed); break;
  case fl::primitives::PartyActivity::Crafting: complete_crafting(ctx, completed); break;
  case fl::primitives::PartyActivity::SkillStudy: complete_study(ctx, completed); break;
  default: break;
  }
  ctx.account_data().economy().reevaluate_objective(ctx.reg());
}

} // namespace fl::ecs::systems
