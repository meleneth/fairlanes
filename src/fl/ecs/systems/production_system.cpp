#include "fl/ecs/systems/production_system.hpp"

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#include <fmt/format.h>

#include "fl/ecs/components/consumable.hpp"
#include "fl/ecs/components/crafted_provenance.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/profession_expertise.hpp"
#include "fl/ecs/components/skill_study.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/consumable_system.hpp"
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
      item, worker, stats ? stats->name_ : "Unknown", recipe.profession, proficiency,
      std::move(materials));
  return item;
}

void complete_crafting(fl::context::PartyCtx ctx,
                       const fl::primitives::ProductionOperation& operation) {
  const auto* recipe = fl::economy::generated_content::definition(*operation.recipe);
  if (!recipe) return;
  const auto item = create_product(ctx, *recipe, operation.worker, operation.started_world_beat);
  ctx.account_data().economy().deposit_item(item);
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
    const auto* recipe =
        fl::economy::generated_content::definition(fl::economy::RecipeId::ClothChest);
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
