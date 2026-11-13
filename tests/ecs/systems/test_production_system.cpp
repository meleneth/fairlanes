#include <catch2/catch_test_macros.hpp>

#include <algorithm>

#include "fl/ecs/components/consumable.hpp"
#include "fl/ecs/components/crafted_provenance.hpp"
#include "fl/ecs/components/equipment.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/profession_expertise.hpp"
#include "fl/ecs/components/skill_study.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/consumable_system.hpp"
#include "fl/ecs/systems/effective_combat_stats.hpp"
#include "fl/ecs/systems/loot_drop.hpp"
#include "fl/economy/material.hpp"
#include "fl/economy/profession.hpp"
#include "fl/economy/recipe.hpp"
#include "fl/grand_central.hpp"
#include "fl/primitives/account_economy.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "fl/primitives/party_activity.hpp"
#include "fl/primitives/world_clock.hpp"
#include "fl/skills/skill_learning.hpp"
#include "fl/skills/skill_selection.hpp"

namespace {

void advance(fl::GrandCentral& game, int beats) {
  auto ctx = game.account_context(0);
  for (int beat = 0; beat < beats; ++beat) ctx.account_data().advance_beat(ctx, false);
}

fl::primitives::PartyActivityAssignment gather(fl::economy::Material material,
                                                fl::economy::Profession profession,
                                                std::string biome) {
  return {.activity = fl::primitives::PartyActivity::Gathering,
          .profession = profession,
          .biome_id = std::move(biome),
          .material = material};
}

fl::primitives::PartyActivityAssignment craft(fl::economy::RecipeId recipe,
                                               fl::economy::Profession profession) {
  return {.activity = fl::primitives::PartyActivity::Crafting,
          .profession = profession,
          .recipe = recipe};
}

fl::primitives::PartyActivityAssignment idle() {
  return {.activity = fl::primitives::PartyActivity::Idle};
}

} // namespace

TEST_CASE("Gathering respects biome access, simulated time, and character expertise",
          "[production][gathering]") {
  fl::GrandCentral game{1, 1, 1, false, false, 101};
  auto account = game.account_context(0);
  auto& party = account.account_data().party(0);
  const auto hero = party.members().front().member_id();

  REQUIRE(party.request_activity(gather(fl::economy::Material::PlainsBloom,
                                        fl::economy::Profession::Herbalism,
                                        "origin_plains_tower")));
  advance(game, 2);
  REQUIRE_FALSE(party.production_operation());
  REQUIRE(party.production_blocked_reason().find("locked") != std::string_view::npos);
  REQUIRE(account.account_data().economy().material_count(
              fl::economy::Material::PlainsBloom) == 0);

  REQUIRE(party.request_activity(gather(fl::economy::Material::WoodlandHerb,
                                        fl::economy::Profession::Herbalism,
                                        "origin_woodland_tower")));
  const int duration = fl::primitives::WorldClock::beats_from_seconds(20);
  advance(game, duration);
  REQUIRE(account.account_data().economy().material_count(
              fl::economy::Material::WoodlandHerb) == 0);
  advance(game, 2);
  REQUIRE(account.account_data().economy().material_count(
              fl::economy::Material::WoodlandHerb) == 2);
  const auto& expertise =
      game.reg().get<fl::ecs::components::ProfessionExpertise>(hero);
  REQUIRE(expertise.xp(fl::economy::Profession::Herbalism) == 6);
  REQUIRE_FALSE(party.in_combat());
  REQUIRE(party.highest_tower_floor_cleared("origin_woodland_tower") == 0);
}

TEST_CASE("Combat victories create deterministic biome material opportunities",
          "[production][combat-materials]") {
  fl::GrandCentral game{1, 1, 1, false, false, 100};
  auto party = game.account_context(0).party_context(0);
  REQUIRE(party.party_data().select_tower("origin_woodland_tower"));
  party.party_data().begin_tower_encounter();
  const auto source = game.reg().create();
  fl::ecs::systems::LootDropSystem::commit(party, source);
  REQUIRE(party.account_data().economy().material_count(
              fl::economy::Material::WoodlandHide) == 1);
}

TEST_CASE("Production assignment waits for an active encounter boundary",
          "[production][boundary]") {
  fl::GrandCentral game{1, 1, 1, false, false, 107};
  auto party = game.account_context(0).party_context(0);
  (void)fl::primitives::EncounterBuilder{party}.thump_it_out();
  REQUIRE(party.party_data().in_combat());
  REQUIRE(party.party_data().request_activity(
      gather(fl::economy::Material::WoodlandHerb, fl::economy::Profession::Herbalism,
             "origin_woodland_tower")));
  REQUIRE(party.party_data().activity_assignment().activity ==
          fl::primitives::PartyActivity::CombatProgression);
  REQUIRE(party.party_data().in_combat());
  party.party_data().return_to_town();
  REQUIRE_FALSE(party.party_data().in_combat());
  REQUIRE(party.party_data().activity_assignment().activity ==
          fl::primitives::PartyActivity::Gathering);
}

TEST_CASE("Crafting commits finite inputs once and interruption cannot duplicate output",
          "[production][crafting][inventory]") {
  fl::GrandCentral game{1, 1, 1, false, false, 102};
  auto account = game.account_context(0);
  auto& party = account.account_data().party(0);
  auto& economy = account.account_data().economy();
  economy.add_material(fl::economy::Material::WoodlandHerb, 4);

  REQUIRE(party.request_activity(
      craft(fl::economy::RecipeId::HealingPotion, fl::economy::Profession::Alchemy)));
  advance(game, 1);
  REQUIRE(party.production_operation());
  REQUIRE(economy.material_count(fl::economy::Material::WoodlandHerb) == 2);
  REQUIRE(party.request_activity(idle()));
  REQUIRE_FALSE(party.production_operation());
  advance(game, fl::primitives::WorldClock::beats_from_seconds(40));
  REQUIRE(economy.items().empty());
  REQUIRE(economy.material_count(fl::economy::Material::WoodlandHerb) == 2);

  REQUIRE(party.request_activity(
      craft(fl::economy::RecipeId::HealingPotion, fl::economy::Profession::Alchemy)));
  advance(game, fl::primitives::WorldClock::beats_from_seconds(30) + 2);
  REQUIRE(economy.items().size() == 1);
  const auto item = economy.items().front();
  const auto& potion = game.reg().get<fl::ecs::components::Consumable>(item);
  REQUIRE(potion.kind == fl::consumables::ConsumableKind::HealingPotion);
  REQUIRE(potion.quantity == 1);
  REQUIRE(game.reg().all_of<fl::ecs::components::CraftedProvenance>(item));
  REQUIRE(economy.material_count(fl::economy::Material::WoodlandHerb) == 0);
}

TEST_CASE("Crafted equipment keeps deterministic stats, identity, crafter, and materials",
          "[production][equipment][determinism]") {
  auto make_item = [] {
    auto game = std::make_unique<fl::GrandCentral>(1, 1, 1, false, false, 777);
    auto account = game->account_context(0);
    auto& party = account.account_data().party(0);
    account.account_data().economy().add_material(fl::economy::Material::WoodlandFiber, 3);
    party.request_activity(
        craft(fl::economy::RecipeId::ClothChest, fl::economy::Profession::Tailoring));
    advance(*game, fl::primitives::WorldClock::beats_from_seconds(40) + 2);
    return game;
  };
  auto first = make_item();
  auto second = make_item();
  const auto first_item = first->accounts().front().economy().items().front();
  const auto second_item = second->accounts().front().economy().items().front();
  REQUIRE(first_item != entt::entity{entt::null});
  const auto& a = first->reg().get<fl::ecs::components::Equipment>(first_item);
  const auto& b = second->reg().get<fl::ecs::components::Equipment>(second_item);
  REQUIRE(a.name() == "Woven Cloth Chest");
  REQUIRE(a.armor_kind() == fl::loot::ArmorKind::cloth);
  REQUIRE(a.attack() == b.attack());
  REQUIRE(a.armor() == b.armor());
  REQUIRE(a.max_hp() == b.max_hp());
  REQUIRE(a.resistances().physical == b.resistances().physical);
  REQUIRE(a.resistances().magical == b.resistances().magical);
  REQUIRE(a.resistances().fire == b.resistances().fire);
  REQUIRE(a.resistances().ice == b.resistances().ice);
  REQUIRE(a.resistances().lightning == b.resistances().lightning);
  const auto& provenance =
      first->reg().get<fl::ecs::components::CraftedProvenance>(first_item);
  REQUIRE(provenance.crafter != entt::entity{entt::null});
  REQUIRE(provenance.crafter_name ==
          first->reg().get<fl::ecs::components::Stats>(provenance.crafter).name_);
  REQUIRE(provenance.profession == fl::economy::Profession::Tailoring);
  REQUIRE(provenance.materials.size() == 1);
  REQUIRE(provenance.materials.front().material == fl::economy::Material::WoodlandFiber);
  REQUIRE(provenance.materials.front().quantity == 3);

  auto first_account = first->account_context(0);
  auto& receiving_party = first_account.account_data().party(0);
  const auto wearer = receiving_party.members().front().member_id();
  const auto before = fl::ecs::systems::effective_combat_stats(first->reg(), wearer);
  REQUIRE(first_account.account_data().economy().distribute_item(receiving_party, first_item));
  receiving_party.party_bus().emit(fl::events::PartyEvent{fl::events::EquipmentEquipRequested{
      wearer, fl::loot::EquipmentSlot::chest, first_item}});
  REQUIRE(first->reg().get<fl::ecs::components::PartyMember>(wearer).closet().chest == first_item);
  const auto after = fl::ecs::systems::effective_combat_stats(first->reg(), wearer);
  REQUIRE((after.attack != before.attack || after.armor != before.armor ||
           after.max_hp != before.max_hp ||
           after.resistances.physical != before.resistances.physical ||
           after.resistances.magical != before.resistances.magical ||
           after.resistances.fire != before.resistances.fire ||
           after.resistances.ice != before.resistances.ice ||
           after.resistances.lightning != before.resistances.lightning));
}

TEST_CASE("Account inventory distributes crafted healing across cooperating parties",
          "[production][longitudinal][cooperation]") {
  fl::GrandCentral game{1, 2, 1, false, false, 103};
  auto account = game.account_context(0);
  auto& combat_party = account.account_data().party(0);
  auto& production_party = account.account_data().party(1);
  combat_party.request_activity(idle());
  production_party.request_activity(gather(fl::economy::Material::WoodlandHerb,
                                            fl::economy::Profession::Herbalism,
                                            "origin_woodland_tower"));
  advance(game, fl::primitives::WorldClock::beats_from_seconds(20) + 2);
  REQUIRE(account.account_data().economy().material_count(
              fl::economy::Material::WoodlandHerb) == 2);

  production_party.request_activity(
      craft(fl::economy::RecipeId::HealingPotion, fl::economy::Profession::Alchemy));
  advance(game, fl::primitives::WorldClock::beats_from_seconds(30) + 2);
  auto& economy = account.account_data().economy();
  REQUIRE(economy.items().size() == 1);
  const auto potion = economy.items().front();
  REQUIRE(economy.distribute_item(combat_party, potion));
  REQUIRE(economy.items().empty());
  REQUIRE(std::ranges::find(combat_party.items(), potion) != combat_party.items().end());

  auto combat_ctx = account.party_context(combat_party);
  auto& encounter = fl::primitives::EncounterBuilder{combat_ctx}.thump_it_out();
  const auto hero = combat_party.members().front().member_id();
  auto& stats = game.reg().get<fl::ecs::components::Stats>(hero);
  stats.max_hp_ = 20;
  stats.hp_ = 3;
  REQUIRE(fl::ecs::systems::ConsumableSystem::use_potion(encounter.context(), hero, potion));
  REQUIRE(stats.hp_ == 15);
  REQUIRE_FALSE(game.reg().valid(potion));
  REQUIRE(economy.metrics().materials_acquired == 2);
  REQUIRE(economy.metrics().materials_consumed == 2);
  REQUIRE(economy.metrics().products_created == 1);
  REQUIRE(economy.metrics().items_distributed == 1);
}

TEST_CASE("Ordinary observation remains immediate while Eviscerate requires transferable study",
          "[production][study][observe]") {
  fl::GrandCentral game{1, 2, 1, false, false, 104};
  auto account = game.account_context(0);
  auto& source = account.account_data().party(0);
  auto& destination = account.account_data().party(1);
  source.request_activity(idle());
  destination.request_activity(idle());
  const auto student = source.members().front().member_id();
  const auto teacher = game.reg().create();

  REQUIRE(fl::skills::learn_observed_skill_result_with_roll(
              source.party_ctx(), student, teacher, fl::skills::SkillId::Bump, 1) ==
          fl::skills::LearnObservedSkillResult::Learned);
  REQUIRE(source.members().front().grimoire().knows(fl::skills::SkillId::Bump));
  REQUIRE(fl::skills::learn_observed_skill_result_with_roll(
              source.party_ctx(), student, teacher, fl::skills::SkillId::Eviscerate, 1) ==
          fl::skills::LearnObservedSkillResult::StudyRequired);
  REQUIRE_FALSE(source.members().front().grimoire().knows(fl::skills::SkillId::Eviscerate));
  REQUIRE(game.reg().get<fl::ecs::components::SkillStudy>(student).find(
              fl::skills::SkillId::Eviscerate) != nullptr);
  game.reg()
      .get<fl::ecs::components::ProfessionExpertise>(student)
      .grant(fl::economy::Profession::Tailoring, 25);

  REQUIRE(account.account_data().request_roster_transfer(account, student, 1));
  REQUIRE(destination.members().contains(student));
  REQUIRE(game.reg().get<fl::ecs::components::SkillStudy>(student).find(
              fl::skills::SkillId::Eviscerate) != nullptr);
  REQUIRE(game.reg()
              .get<fl::ecs::components::ProfessionExpertise>(student)
              .xp(fl::economy::Profession::Tailoring) == 25);
  destination.set_progression_control_mode(fl::primitives::ProgressionControlMode::Auto);
  account.account_data().economy().set_objective(
      {.kind = fl::primitives::ProductionObjectiveKind::SkillStudy,
       .student = student,
       .skill = fl::skills::SkillId::Eviscerate,
       .target_quantity = 1});
  advance(game, fl::primitives::WorldClock::beats_from_seconds(120) + 2);
  REQUIRE(game.reg()
              .get<fl::ecs::components::PartyMember>(student)
              .member_data()
              .grimoire()
              .knows(fl::skills::SkillId::Eviscerate));
  REQUIRE(game.reg().get<fl::ecs::components::SkillStudy>(student).find(
              fl::skills::SkillId::Eviscerate) == nullptr);
  REQUIRE(game.reg()
              .get<fl::ecs::components::PartyMember>(student)
              .closet()
              .has_equipped_skill(fl::skills::SkillId::Eviscerate));
  auto& closet = game.reg().get<fl::ecs::components::PartyMember>(student).closet();
  for (std::size_t slot = 1; slot < closet.skill_slots.size(); ++slot) closet.skill_slots[slot].reset();
  REQUIRE(closet.equip_skill(fl::skills::SkillId::Eviscerate));
  REQUIRE(fl::skills::choose_skill(game.reg(), game.rng(), student).base ==
          fl::skills::SkillId::Eviscerate);
  REQUIRE(account.account_data().economy().objective()->completed);
}

TEST_CASE("Multiple parties advance independent assignments on one account beat",
          "[production][multi-party]") {
  fl::GrandCentral game{1, 2, 1, false, false, 105};
  auto account = game.account_context(0);
  auto& first = account.account_data().party(0);
  auto& second = account.account_data().party(1);
  first.request_activity(gather(fl::economy::Material::WoodlandHerb,
                                fl::economy::Profession::Herbalism,
                                "origin_woodland_tower"));
  second.request_activity(gather(fl::economy::Material::WoodlandFiber,
                                 fl::economy::Profession::Scavenging,
                                 "origin_woodland_tower"));
  advance(game, fl::primitives::WorldClock::beats_from_seconds(20) + 2);
  REQUIRE(account.account_data().economy().material_count(
              fl::economy::Material::WoodlandHerb) == 2);
  REQUIRE(account.account_data().economy().material_count(
              fl::economy::Material::WoodlandFiber) == 2);
  REQUIRE_FALSE(first.in_combat());
  REQUIRE_FALSE(second.in_combat());
}

TEST_CASE("Autonomous production maintains a stockpile and reports stable locked inputs",
          "[production][objective][autonomous]") {
  fl::GrandCentral game{1, 1, 1, false, false, 106};
  auto account = game.account_context(0);
  auto& party = account.account_data().party(0);
  party.set_progression_control_mode(fl::primitives::ProgressionControlMode::Auto);
  party.request_activity(idle());
  account.account_data().economy().set_objective(
      {.kind = fl::primitives::ProductionObjectiveKind::ConsumableStockpile,
       .consumable = fl::consumables::ConsumableKind::HealingPotion,
       .target_quantity = 2});
  advance(game, fl::primitives::WorldClock::beats_from_seconds(110));
  const auto* objective = account.account_data().economy().objective();
  REQUIRE(objective);
  REQUIRE(objective->completed);
  int potions = 0;
  for (const auto item : account.account_data().economy().items())
    if (const auto* value = game.reg().try_get<fl::ecs::components::Consumable>(item);
        value && value->kind == fl::consumables::ConsumableKind::HealingPotion)
      potions += value->quantity;
  REQUIRE(potions == 2);
  REQUIRE(party.activity_assignment().activity == fl::primitives::PartyActivity::Idle);

  account.account_data().economy().set_objective(
      {.kind = fl::primitives::ProductionObjectiveKind::CraftedEquipment,
       .target_quantity = 1});
  advance(game, fl::primitives::WorldClock::beats_from_seconds(100));
  objective = account.account_data().economy().objective();
  REQUIRE(objective);
  REQUIRE(objective->completed);
  REQUIRE(std::ranges::any_of(account.account_data().economy().items(), [&](entt::entity item) {
    return game.reg().all_of<fl::ecs::components::Equipment,
                             fl::ecs::components::CraftedProvenance>(item);
  }));

  account.account_data().economy().set_objective(
      {.kind = fl::primitives::ProductionObjectiveKind::ConsumableStockpile,
       .consumable = fl::consumables::ConsumableKind::AttackFlask,
       .target_quantity = 1});
  advance(game, 20);
  objective = account.account_data().economy().objective();
  REQUIRE(objective);
  REQUIRE_FALSE(objective->completed);
  REQUIRE_FALSE(objective->blocked_reason.empty());
  REQUIRE(account.account_data().economy().metrics().blocked_operations == 1);
}
