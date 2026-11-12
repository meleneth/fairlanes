#include "fl/regression/session.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include <entt/entity/entity.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include "fl/build_identity.hpp"
#include "fl/ecs/components/atb_charge.hpp"
#include "fl/ecs/components/closet.hpp"
#include "fl/ecs/components/combat_status.hpp"
#include "fl/ecs/components/dire_bleed.hpp"
#include "fl/ecs/components/equipment.hpp"
#include "fl/ecs/components/freeze.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/poison.hpp"
#include "fl/ecs/components/skill_slots.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/ecs/systems/effective_combat_stats.hpp"
#include "fl/ecs/systems/manual_equipment.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/generated/floor_content.hpp"
#include "fl/generated/monster_content.hpp"
#include "fl/generated/regression_content.hpp"
#include "fl/grand_central.hpp"
#include "fl/loot/equipment_stat_policy.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "fl/primitives/encounter_data.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/world_clock.hpp"
#include "fl/skills/skill.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::regression {
namespace {

using Json = nlohmann::json;
using fl::ecs::components::Stats;

std::string skill_id(fl::skills::SkillKey skill) {
  return std::string(fl::skills::stable_id(skill.base)) + "/" + std::to_string(skill.rank.value());
}

Json stats_json(entt::registry& reg, entt::entity actor) {
  const auto& stats = reg.get<Stats>(actor);
  const auto effective = fl::ecs::systems::effective_combat_stats(reg, actor);
  const auto& r = effective.resistances;
  return {{"hp", stats.hp_},
          {"max_hp", effective.max_hp},
          {"attack", effective.attack},
          {"armor", effective.armor},
          {"resistances",
           {{"physical", r.physical},
            {"magical", r.magical},
            {"fire", r.fire},
            {"ice", r.ice},
            {"lightning", r.lightning}}}};
}

Json atb_json(const fl::ecs::components::AtbCharge& atb) {
  return {{"charge", atb.charge},
          {"max_charge", atb.max_charge},
          {"charge_per_beat", atb.charge_per_beat}};
}

Json statuses_for(entt::registry& reg, entt::entity actor) {
  std::vector<std::string> statuses;
  if (const auto* effects = reg.try_get<fl::ecs::components::CombatStatuses>(actor)) {
    for (const auto& effect : effects->effects) {
      statuses.push_back(effect.name);
    }
  }
  if (reg.all_of<fl::ecs::components::Poison>(actor)) statuses.emplace_back("Poison");
  if (reg.all_of<fl::ecs::components::Freeze>(actor)) statuses.emplace_back("Freeze");
  if (reg.all_of<fl::ecs::components::DireBleed>(actor)) statuses.emplace_back("Dire Bleed");
  std::ranges::sort(statuses);
  return statuses;
}

int party_size_for(std::string_view fixture) {
  const auto* scenario = generated_content::find(fixture);
  return scenario ? static_cast<int>(scenario->party.size()) : 1;
}

} // namespace

struct Session::Impl {
  struct Actor {
    std::string id;
    entt::entity entity;
    Json prior_state;
  };

  Options options;
  GrandCentral game;
  fl::context::AccountCtx account_ctx;
  fl::context::PartyCtx party_ctx;
  fl::primitives::EncounterData* encounter{};
  std::vector<Actor> actors;
  std::size_t hero_count{1};
  std::vector<fl::events::ScopedCombatantListener> listeners;
  std::vector<fl::events::ScopedPartyListener> party_listeners;
  const generated_content::Scenario* scenario{};
  const generated_content::Variant* variant{};
  bool extended_recording{false};
  std::size_t encounter_index{0};
  std::uint64_t encounter_start_beat{0};
  struct Metrics {
    int brace_applications{0};
    int brace_consumptions{0};
    int brace_mitigated_hits{0};
    int brace_mitigated_damage{0};
    int incoming_damage{0};
    int outgoing_damage{0};
    int eviscerate_casts{0};
    int bleed_applications{0};
    int bleed_ticks{0};
    int bleed_tick_damage{0};
    int bleed_deaths{0};
    int bleed_clears{0};
    std::int64_t longest_bleed_uwu{0};
  } metrics;
  std::unordered_map<entt::entity, std::int64_t> active_bleeds;
  Json recording;
  Json compact_summary;
  Json initial_party_snapshot;
  Json encounter_party_states = Json::array();
  Json compact_encounter_results = Json::array();
  std::uint64_t beats{0};
  std::string outcome;

  explicit Impl(Options selected)
      : options(std::move(selected)), game(1, 1, party_size_for(options.fixture), false, false,
                                           options.seed, false, false, options.tower_balance),
        account_ctx(game.account_context(0)), party_ctx(account_ctx.party_context(0)) {
    if (const auto* scenario = generated_content::find(options.fixture)) {
      setup_generated(*scenario);
      return;
    }
    const bool armor_fixture = options.fixture == kArmorFixture;
    if (options.fixture != kSmokeFixture && !armor_fixture) {
      throw std::invalid_argument("unknown regression fixture: " + options.fixture);
    }
    if (options.hero_armor < 0 || (!armor_fixture && options.hero_armor != 0)) {
      throw std::invalid_argument("invalid Armor for regression fixture");
    }
    if (options.beat_cap == 0) {
      throw std::invalid_argument("beat cap must be positive");
    }

    auto& hero_member = party_ctx.party_data().members().front();
    const auto hero = hero_member.member_id();
    party_ctx.party_data().preserve_terminal_encounter(true);
    auto& hero_stats = game.reg().get<Stats>(hero);
    hero_stats.name_ = "Regression Hero";
    hero_stats.hp_ = hero_stats.max_hp_ = armor_fixture ? 8 : 20;
    hero_stats.attack_ = armor_fixture ? 0 : 2;
    hero_stats.armor_ = options.hero_armor;
    hero_stats.resistances_ = {};
    auto& party_member = game.reg().get<fl::ecs::components::PartyMember>(hero);
    party_member.closet().unequip_skill(fl::skills::SkillId::Observe);
    hero_member.grimoire().learn(fl::skills::SkillId::Thump);
    if (!party_member.equip_known_skill(fl::skills::SkillId::Thump)) {
      throw std::runtime_error("could not equip fixture Thump");
    }

    auto& built = fl::primitives::EncounterBuilder{party_ctx}.one_on_one(
        armor_fixture ? fl::monster::MonsterKind::BumpkinHare
                      : fl::monster::MonsterKind::FieldMouse);
    encounter = &built;
    const auto mouse = built.attackers().members().front();
    auto& mouse_stats = game.reg().get<Stats>(mouse);
    mouse_stats.hp_ = mouse_stats.max_hp_ = armor_fixture ? 14 : 8;
    mouse_stats.attack_ = 0;
    mouse_stats.armor_ = 0;
    mouse_stats.resistances_ = {};
    const auto enemy_skill = armor_fixture ? fl::skills::SkillId::Bump : fl::skills::SkillId::Thump;
    game.reg().replace<fl::ecs::components::SkillSlots>(
        mouse, fl::ecs::components::SkillSlots::with_known(fl::skills::SkillKey{enemy_skill}));

    actors = {{"hero", hero, {}}, {armor_fixture ? "hare" : "mouse", mouse, {}}};
    for (auto& actor : actors) actor.prior_state = state(actor.entity);

    recording = {
        {"format", "fairlanes-combat-recording"},
        {"version", 1},
        {"fixture", options.fixture},
        {"build",
         {{"version", fl::build_identity::version},
          {"revision", fl::build_identity::revision},
          {"source_dirty", fl::build_identity::source_dirty}}},
        {"content_sha256", fl::build_identity::content_sha256},
        {"gameplay_seed", options.seed},
        {"gameplay_sequence", game.rng().base_sequence()},
        {"beat_cap", options.beat_cap},
        {"fixture_parameters",
         {{"hero_armor", options.hero_armor},
          {"tower_max_level", options.tower_balance.maximum_monster_level},
          {"tower_max_hp", options.tower_balance.maximum_monster_hp},
          {"tower_max_attack", options.tower_balance.maximum_monster_attack}}},
        {"starting_world_beat", game.world_clock().elapsed_beats()},
        {"starting_uwu", encounter->visual_time().v},
        {"expected_outcome", armor_fixture ? Json(nullptr) : Json("hero_victory")},
        {"external_inputs", Json::array()},
        {"initial_state",
         Json::array({{{"id", "hero"},
                       {"kind", "player"},
                       {"name", hero_stats.name_},
                       {"stats", stats_json(game.reg(), hero)},
                       {"atb", atb_json(game.reg().get<fl::ecs::components::AtbCharge>(hero))},
                       {"statuses", statuses_for(game.reg(), hero)},
                       {"loadout", Json::array({"thump/1"})},
                       {"known_skills", Json::array({"observe/1", "thump/1"})},
                       {"action_policy", "single_equipped_skill"},
                       {"gear", Json::array()}},
                      {{"id", armor_fixture ? "hare" : "mouse"},
                       {"kind", armor_fixture ? "bumpkin_hare" : "field_mouse"},
                       {"name", mouse_stats.name_},
                       {"stats", stats_json(game.reg(), mouse)},
                       {"atb", atb_json(game.reg().get<fl::ecs::components::AtbCharge>(mouse))},
                       {"statuses", statuses_for(game.reg(), mouse)},
                       {"loadout", Json::array({armor_fixture ? "bump/1" : "thump/1"})},
                       {"known_skills", Json::array({armor_fixture ? "bump/1" : "thump/1"})},
                       {"action_policy", "single_known_skill"},
                       {"gear", Json::array()}}})},
        {"events", Json::array()},
        {"result", nullptr}};

    for (const auto& actor : actors) bind_events(actor.entity);
    initial_party_snapshot = party_state();
    encounter_party_states.push_back(initial_party_snapshot);
    game.innervate_event_system(false);
  }

  void setup_generated(const generated_content::Scenario& scenario) {
    this->scenario = &scenario;
    variant = generated_content::find_variant(scenario, options.variant);
    if (!variant) throw std::invalid_argument("unknown regression variant: " + options.variant);
    if (options.hero_armor != 0)
      throw std::invalid_argument("generated regression does not accept --armor");
    if (options.beat_cap != scenario.beat_cap)
      throw std::invalid_argument("generated regression beat cap differs from declaration");

    auto& party = party_ctx.party_data();
    party.preserve_terminal_encounter(true);
    std::size_t index = 0;
    for (auto& member : party.members()) {
      const auto setup = scenario.party[index];
      if (setup.policy != generated_content::ActionPolicy::SingleEquippedSkill)
        throw std::invalid_argument("unsupported regression action policy");
      const auto entity = member.member_id();
      auto& stats = game.reg().get<Stats>(entity);
      stats.name_ = "Regression Hero " + std::to_string(index + 1);
      stats.hp_ = stats.max_hp_ = setup.hp;
      stats.attack_ = setup.attack;
      stats.armor_ = setup.armor;
      stats.resistances_ = setup.resistances;
      auto& party_member = game.reg().get<fl::ecs::components::PartyMember>(entity);
      party_member.closet().skill_slots.fill(std::nullopt);
      const auto configured_skill = variant->skill_overrides[index].value_or(setup.skill);
      member.grimoire().learn(configured_skill);
      if (variant->observe_equipped[index] &&
          !party_member.equip_known_skill(fl::skills::SkillId::Observe))
        throw std::runtime_error("could not equip generated regression Observe");
      if (!party_member.equip_known_skill(configured_skill))
        throw std::runtime_error("could not equip generated regression skill");
      actors.push_back({"hero" + std::to_string(index + 1), entity, {}});
      ++index;
    }
    hero_count = actors.size();

    std::optional<std::string> equipped_item_id;
    if (variant->equipped_item) {
      // Variant gear travels through inventory and the ordinary equip system.
      const auto item =
          fl::loot::generated_content::definition(*variant->equipped_item).item.create(game.reg());
      party.add_item(item);
      const auto& gear = game.reg().get<fl::ecs::components::Equipment>(item);
      const auto request =
          fl::events::EquipmentEquipRequested{actors.front().entity, gear.slot(), item};
      if (!fl::ecs::systems::ManualEquipmentSystem::commit(party_ctx, request))
        throw std::runtime_error("could not equip regression variant item");
      equipped_item_id = std::string(gear.unique_id());
    }
    if (variant->generated_equipment) {
      const auto& generated = *variant->generated_equipment;
      for (std::size_t hero_index = 0; hero_index < actors.size(); ++hero_index) {
        fl::loot::EquipmentBuilder builder{.slot = generated.slot,
                                           .armor_kind = generated.armor_kind,
                                           .tier = generated.tier,
                                           .name = "Regression " + std::string(generated.biome_id) +
                                                   " equipment"};
        fl::loot::apply_ordinary_equipment_stats(
            builder, game.rng(), generated.biome_id,
            fmt::format("regression.equipment.{}.{}", variant->id, hero_index));
        const auto item = builder.create(game.reg());
        party.add_item(item);
        auto& xp = game.reg().get<fl::ecs::components::TrackXP>(actors[hero_index].entity);
        xp.level_ = std::max(xp.level_, fl::loot::equipment_minimum_level(generated.tier));
        const auto request =
            fl::events::EquipmentEquipRequested{actors[hero_index].entity, generated.slot, item};
        if (!fl::ecs::systems::ManualEquipmentSystem::commit(party_ctx, request))
          throw std::runtime_error("could not equip generated regression item");
      }
      equipped_item_id =
          fmt::format("generated:{}:{}", generated.biome_id, static_cast<int>(generated.tier));
    }
    party.revitalize_members();

    const auto first_encounter = scenario.encounters.front();
    auto& built =
        fl::primitives::EncounterBuilder{
            party_ctx, fl::primitives::EncounterMode::HeroesJourney,
            fl::primitives::EncounterBuilder::TowerFloor{std::string(first_encounter.biome_id),
                                                         first_encounter.floor}}
            .thump_it_out();
    encounter = &built;
    const auto* floor = fl::primitives::generated_content::floor_lineup(first_encounter.biome_id,
                                                                        first_encounter.floor);
    for (std::size_t i = 0; i < built.attackers().members().size(); ++i)
      actors.push_back({"enemy" + std::to_string(i + 1), built.attackers().members()[i], {}});
    for (auto& actor : actors) actor.prior_state = state(actor.entity);

    Json initial = Json::array();
    for (std::size_t i = 0; i < actors.size(); ++i) {
      const auto& actor = actors[i];
      const auto& stats = game.reg().get<Stats>(actor.entity);
      const bool hero = i < hero_count;
      Json known = Json::array();
      Json loadout = Json::array();
      if (hero) {
        const auto& member = party.members()[i];
        for (const auto skill : member.grimoire().known_skills()) known.push_back(skill_id(skill));
        const auto& closet =
            game.reg().get<fl::ecs::components::PartyMember>(actor.entity).closet();
        for (const auto skill : closet.skill_slots)
          if (skill) loadout.push_back(skill_id(*skill));
      } else {
        for (const auto skill :
             fl::monster::generated_content::known_skills(floor->monsters[i - hero_count]))
          known.push_back(skill_id(fl::skills::SkillKey{skill}));
        loadout = known;
      }
      initial.push_back(
          {{"id", actor.id},
           {"kind", hero ? "player" : "monster"},
           {"name", stats.name_},
           {"stats", stats_json(game.reg(), actor.entity)},
           {"atb", atb_json(game.reg().get<fl::ecs::components::AtbCharge>(actor.entity))},
           {"statuses", statuses_for(game.reg(), actor.entity)},
           {"loadout", loadout},
           {"known_skills", known},
           {"action_policy", hero ? "single_equipped_skill" : "monster_decision_rules"},
           {"gear", hero && equipped_item_id ? Json::array({*equipped_item_id}) : Json::array()}});
    }
    extended_recording = scenario.recording_version == 2;
    recording = {{"format", "fairlanes-combat-recording"},
                 {"version", extended_recording ? 2 : 1},
                 {"fixture", options.fixture},
                 {"build",
                  {{"version", fl::build_identity::version},
                   {"revision", fl::build_identity::revision},
                   {"source_dirty", fl::build_identity::source_dirty}}},
                 {"content_sha256", fl::build_identity::content_sha256},
                 {"gameplay_seed", options.seed},
                 {"gameplay_sequence", game.rng().base_sequence()},
                 {"beat_cap", options.beat_cap},
                 {"fixture_parameters",
                  {{"variant", options.variant},
                   {"equipped_item", equipped_item_id ? Json(*equipped_item_id) : Json(nullptr)},
                   {"tower_max_level", options.tower_balance.maximum_monster_level},
                   {"tower_max_hp", options.tower_balance.maximum_monster_hp},
                   {"tower_max_attack", options.tower_balance.maximum_monster_attack}}},
                 {"starting_world_beat", game.world_clock().elapsed_beats()},
                 {"starting_uwu", encounter->visual_time().v},
                 {"expected_outcome", nullptr},
                 {"external_inputs", Json::array()},
                 {"initial_state", initial},
                 {"events", Json::array()},
                 {"result", nullptr}};
    if (extended_recording) {
      recording["encounters"] = Json::array();
      for (const auto setup : scenario.encounters)
        recording["encounters"].push_back({{"biome", setup.biome_id}, {"floor", setup.floor}});
      recording["encounter_results"] = Json::array();
      recording["initial_party_state"] = party_state();
    }
    for (const auto& actor : actors) bind_events(actor.entity);
    party_listeners.emplace_back(party.party_bus(), std::in_place_type<fl::events::SkillWitnessed>,
                                 [this](const auto& event) {
                                   if (extended_recording)
                                     append({{"type", "skill_witnessed"},
                                             {"user", actor_id(event.user)},
                                             {"skill", skill_id(event.skill)}});
                                 });
    initial_party_snapshot = party_state();
    encounter_party_states.push_back(initial_party_snapshot);
    game.innervate_event_system(false);
  }

  std::string_view actor_id(entt::entity entity) const {
    for (const auto& actor : actors) {
      if (actor.entity == entity) return actor.id;
    }
    throw std::runtime_error("combat event referenced an unknown actor");
  }

  Json state(entt::entity entity) {
    return {{"hp", game.reg().get<Stats>(entity).hp_},
            {"statuses", statuses_for(game.reg(), entity)}};
  }

  Json party_state() {
    Json result = Json::array();
    const auto& party = party_ctx.party_data();
    for (std::size_t index = 0; index < party.members().size(); ++index) {
      const auto entity = party.members()[index].member_id();
      Json known = Json::array();
      for (const auto skill : party.members()[index].grimoire().known_skills())
        known.push_back(skill_id(skill));
      Json loadout = Json::array();
      const auto& closet = game.reg().get<fl::ecs::components::PartyMember>(entity).closet();
      for (const auto skill : closet.skill_slots)
        if (skill) loadout.push_back(skill_id(*skill));
      result.push_back({{"id", "hero" + std::to_string(index + 1)},
                        {"hp", game.reg().get<Stats>(entity).hp_},
                        {"stats", stats_json(game.reg(), entity)},
                        {"known_skills", std::move(known)},
                        {"loadout", std::move(loadout)}});
    }
    return result;
  }

  void append(Json event) {
    auto& events = recording["events"];
    event["seq"] = events.size();
    if (recording.value("version", 1) == 1) {
      event["uwu"] = encounter->visual_time().v;
    } else {
      event["encounter_index"] = encounter_index;
      event["world_beat"] = game.world_clock().elapsed_beats();
      event["uwu"] = static_cast<std::int64_t>(beats) * seerin::UWU_PER_BEAT.v;
      event["encounter_uwu"] = encounter->visual_time().v;
    }
    events.push_back(std::move(event));
  }

  std::int64_t current_uwu() const {
    return recording.value("version", 1) == 1
               ? encounter->visual_time().v
               : static_cast<std::int64_t>(beats) * seerin::UWU_PER_BEAT.v;
  }

  void bind_events(entt::entity actor) {
    auto& bus = encounter->combatant_bus(actor);
    listeners.emplace_back(bus, std::in_place_type<fl::events::SkillActionSelected>,
                           [this](const auto& event) {
                             append({{"type", "action"},
                                     {"actor", actor_id(event.actor)},
                                     {"target", actor_id(event.target)},
                                     {"skill", skill_id(event.skill)}});
                           });
    listeners.emplace_back(bus, std::in_place_type<fl::events::SkillCastStarted>,
                           [this](const auto& event) {
                             if (event.skill.base == fl::skills::SkillId::Eviscerate)
                               ++metrics.eviscerate_casts;
                             append({{"type", "cast_start"},
                                     {"actor", actor_id(event.caster)},
                                     {"skill", skill_id(event.skill)}});
                           });
    listeners.emplace_back(bus, std::in_place_type<fl::events::SkillHitLanded>,
                           [this](const auto& event) {
                             const auto target = actor_id(event.target);
                             const auto source = actor_id(event.source);
                             if (target.starts_with("hero"))
                               metrics.incoming_damage += event.damage;
                             if (source.starts_with("hero"))
                               metrics.outgoing_damage += event.damage;
                             append({{"type", "hit"},
                                     {"actor", actor_id(event.source)},
                                     {"target", actor_id(event.target)},
                                     {"skill", skill_id(event.skill)},
                                     {"damage", event.damage},
                                     {"hp_after", game.reg().get<Stats>(event.target).hp_}});
                           });
    listeners.emplace_back(bus, std::in_place_type<fl::events::DireBleedApplied>,
                           [this](const auto& event) {
                             ++metrics.bleed_applications;
                             active_bleeds[event.target] = current_uwu();
                             append({{"type", "dire_bleed_applied"},
                                     {"actor", actor_id(event.source)},
                                     {"target", actor_id(event.target)},
                                     {"damage_per_tick", event.damage_per_tick}});
                           });
    listeners.emplace_back(bus, std::in_place_type<fl::events::DireBleedTicked>,
                           [this](const auto& event) {
                             ++metrics.bleed_ticks;
                             metrics.bleed_tick_damage += event.damage;
                             if (actor_id(event.target).starts_with("hero"))
                               metrics.incoming_damage += event.damage;
                             if (event.killed) ++metrics.bleed_deaths;
                             append({{"type", "dire_bleed_tick"},
                                     {"actor", actor_id(event.source)},
                                     {"target", actor_id(event.target)},
                                     {"damage", event.damage},
                                     {"hp_after", event.hp_after},
                                     {"killed", event.killed}});
                           });
    listeners.emplace_back(
        bus, std::in_place_type<fl::events::DireBleedCleared>, [this](const auto& event) {
          ++metrics.bleed_clears;
          if (const auto found = active_bleeds.find(event.target); found != active_bleeds.end()) {
            metrics.longest_bleed_uwu =
                std::max(metrics.longest_bleed_uwu, current_uwu() - found->second);
            active_bleeds.erase(found);
          }
          append({{"type", "dire_bleed_cleared"}, {"target", actor_id(event.target)}});
        });
    listeners.emplace_back(bus, std::in_place_type<fl::events::BraceMitigated>,
                           [this](const auto& event) {
                             const auto target = actor_id(event.target);
                             if (target.starts_with("hero")) {
                               ++metrics.brace_mitigated_hits;
                               metrics.brace_mitigated_damage += event.damage;
                             }
                             if (extended_recording && target.starts_with("hero"))
                               append({{"type", "brace_mitigated"},
                                       {"actor", actor_id(event.source)},
                                       {"target", actor_id(event.target)},
                                       {"damage", event.damage}});
                           });
    listeners.emplace_back(bus, std::in_place_type<fl::events::SkillCastFinished>,
                           [this](const auto& event) {
                             append({{"type", "cast_finish"}, {"actor", actor_id(event.caster)}});
                           });
  }

  Json encounter_result(std::string_view resolved) {
    Json remaining_hp = Json::object();
    int survivors = 0;
    for (std::size_t index = 0; index < actors.size(); ++index) {
      const auto hp = game.reg().get<Stats>(actors[index].entity).hp_;
      remaining_hp[actors[index].id] = hp;
      if (index < hero_count && hp > 0) ++survivors;
    }
    const auto segment_beats = beats - encounter_start_beat;
    auto longest_bleed_uwu = metrics.longest_bleed_uwu;
    for (const auto& [target, started] : active_bleeds) {
      (void)target;
      longest_bleed_uwu = std::max(longest_bleed_uwu, current_uwu() - started);
    }
    return {{"encounter_index", encounter_index},
            {"outcome", resolved},
            {"beats", segment_beats},
            {"simulated_seconds",
             static_cast<double>(segment_beats) / fl::primitives::WorldClock::kBeatsPerSecond},
            {"remaining_hp", std::move(remaining_hp)},
            {"survivors", survivors},
            {"brace_applications", metrics.brace_applications},
            {"brace_consumptions", metrics.brace_consumptions},
            {"brace_mitigated_hits", metrics.brace_mitigated_hits},
            {"brace_mitigated_damage", metrics.brace_mitigated_damage},
            {"incoming_damage", metrics.incoming_damage},
            {"outgoing_damage", metrics.outgoing_damage},
            {"eviscerate_casts", metrics.eviscerate_casts},
            {"bleed_applications", metrics.bleed_applications},
            {"bleed_ticks", metrics.bleed_ticks},
            {"bleed_tick_damage", metrics.bleed_tick_damage},
            {"bleed_deaths", metrics.bleed_deaths},
            {"bleed_clears", metrics.bleed_clears},
            {"bleed_active_at_end", active_bleeds.size()},
            {"longest_bleed_uwu", longest_bleed_uwu},
            {"longest_bleed_seconds",
             static_cast<double>(longest_bleed_uwu) /
                 static_cast<double>(seerin::UWU_PER_BEAT.v *
                                     fl::primitives::WorldClock::kBeatsPerSecond)}};
  }

  void start_next_encounter() {
    auto& party = party_ctx.party_data();
    listeners.clear();
    actors.resize(hero_count);
    party.leave_combat();
    party.revitalize_members();

    if (!variant->next_skill)
      throw std::runtime_error("progression regression has no next skill selection");
    auto& member = game.reg().get<fl::ecs::components::PartyMember>(actors.front().entity);
    std::vector<fl::skills::SkillKey> equipped;
    for (const auto skill : member.closet().skill_slots)
      if (skill) equipped.push_back(*skill);
    for (const auto skill : equipped) member.closet().unequip_skill(skill);
    if (!member.equip_known_skill(*variant->next_skill))
      throw std::runtime_error("progression regression could not equip retained next skill");

    ++encounter_index;
    encounter_start_beat = beats;
    metrics = {};
    active_bleeds.clear();
    const auto setup = scenario->encounters[encounter_index];
    auto& built =
        fl::primitives::EncounterBuilder{
            party_ctx, fl::primitives::EncounterMode::HeroesJourney,
            fl::primitives::EncounterBuilder::TowerFloor{std::string(setup.biome_id), setup.floor}}
            .thump_it_out();
    encounter = &built;
    for (std::size_t index = 0; index < built.attackers().members().size(); ++index)
      actors.push_back(
          {"enemy" + std::to_string(index + 1), built.attackers().members()[index], {}});
    for (auto& actor : actors) actor.prior_state = state(actor.entity);
    for (const auto& actor : actors) bind_events(actor.entity);

    const auto selection = skill_id(fl::skills::SkillKey{*variant->next_skill});
    recording["external_inputs"].push_back({{"type", "skill_selection"},
                                            {"encounter_index", encounter_index},
                                            {"member", "hero1"},
                                            {"skill", selection},
                                            {"world_beat", game.world_clock().elapsed_beats()}});
    append({{"type", "encounter_start"},
            {"biome", setup.biome_id},
            {"floor", setup.floor},
            {"party_state", party_state()}});
    encounter_party_states.push_back(party_state());
  }

  void finish_summary(const Json& final_result) {
    int remaining_party_hp = 0;
    int survivors = 0;
    for (std::size_t index = 0; index < hero_count; ++index) {
      const auto hp = game.reg().get<Stats>(actors[index].entity).hp_;
      remaining_party_hp += hp;
      if (hp > 0) ++survivors;
    }
    Json expectation = nullptr;
    if (variant)
      expectation = {{"asserted", variant->asserted},
                     {"min_wins", variant->min_wins},
                     {"max_wins", variant->max_wins}};
    compact_summary = {{"schema", "fairlanes-regression-run-summary"},
                       {"version", 1},
                       {"fixture", options.fixture},
                       {"variant", options.variant.empty() ? Json(nullptr) : Json(options.variant)},
                       {"seed", options.seed},
                       {"outcome", outcome},
                       {"beat_cap_reached", outcome == "beat_cap"},
                       {"beats", beats},
                       {"simulated_seconds",
                        static_cast<double>(beats) / fl::primitives::WorldClock::kBeatsPerSecond},
                       {"remaining_party_hp", remaining_party_hp},
                       {"survivors", survivors},
                       {"expectation", std::move(expectation)},
                       {"initial_party_state", initial_party_snapshot},
                       {"party_state_before_encounters", encounter_party_states},
                       {"final_party_state", party_state()},
                       {"encounters", compact_encounter_results},
                       {"final_encounter", final_result}};
  }

  void observe() {
    if (!outcome.empty()) return;
    ++beats;
    for (auto& actor : actors) {
      auto next = state(actor.entity);
      if (next != actor.prior_state) {
        if (actor.id.starts_with("hero")) {
          const auto had_brace = std::ranges::find(actor.prior_state["statuses"], "Brace") !=
                                 actor.prior_state["statuses"].end();
          const auto has_brace =
              std::ranges::find(next["statuses"], "Brace") != next["statuses"].end();
          if (!had_brace && has_brace) ++metrics.brace_applications;
          if (had_brace && !has_brace) ++metrics.brace_consumptions;
        }
        append({{"type", "state"},
                {"actor", actor.id},
                {"before", actor.prior_state},
                {"after", next}});
        actor.prior_state = std::move(next);
      }
    }
    const bool hero_alive = std::ranges::any_of(
        actors.begin(), actors.begin() + static_cast<std::ptrdiff_t>(hero_count),
        [this](const Actor& actor) { return game.reg().get<Stats>(actor.entity).is_alive(); });
    const bool enemy_alive = std::ranges::any_of(
        actors.begin() + static_cast<std::ptrdiff_t>(hero_count), actors.end(),
        [this](const Actor& actor) { return game.reg().get<Stats>(actor.entity).is_alive(); });
    if (!hero_alive && !enemy_alive)
      outcome = "mutual_destruction";
    else if (!enemy_alive)
      outcome = "hero_victory";
    else if (!hero_alive)
      outcome = options.fixture == kArmorFixture   ? "hare_victory"
                : options.fixture == kSmokeFixture ? "mouse_victory"
                                                   : "enemy_victory";
    else if (beats >= options.beat_cap)
      outcome = "beat_cap";

    if (!outcome.empty()) {
      const auto resolved = outcome;
      append({{"type", "end"}, {"outcome", resolved}});
      const auto result = encounter_result(resolved);
      compact_encounter_results.push_back(result);
      if (recording.value("version", 1) == 2) {
        recording["encounter_results"].push_back(result);
        if (resolved == "hero_victory" && encounter_index + 1 < scenario->encounters.size()) {
          outcome.clear();
          start_next_encounter();
          return;
        }
        recording["final_party_state"] = party_state();
      }
      if (recording.value("version", 1) == 1) {
        Json remaining_hp = Json::object();
        for (const auto& actor : actors)
          remaining_hp[actor.id] = game.reg().get<Stats>(actor.entity).hp_;
        recording["result"] = {
            {"outcome", outcome},
            {"beats", beats},
            {"uwu", encounter->visual_time().v},
            {"remaining_hp", std::move(remaining_hp)},
            {"simulated_seconds",
             static_cast<double>(beats) / fl::primitives::WorldClock::kBeatsPerSecond}};
      } else {
        recording["result"] = result;
        recording["result"]["beats"] = beats;
        recording["result"]["uwu"] = static_cast<std::int64_t>(beats) * seerin::UWU_PER_BEAT.v;
        recording["result"]["simulated_seconds"] =
            static_cast<double>(beats) / fl::primitives::WorldClock::kBeatsPerSecond;
      }
      finish_summary(result);
    }
  }
};

Session::Session(Options options) : impl_(std::make_unique<Impl>(std::move(options))) {}
Session::~Session() = default;
void Session::advance_beat() {
  if (finished()) return;
  impl_->game.advance_beat();
  impl_->observe();
}
void Session::observe_beat() {
  impl_->observe();
}
bool Session::finished() const {
  return !impl_->outcome.empty();
}
std::string_view Session::outcome() const {
  return impl_->outcome;
}
std::uint64_t Session::beats() const {
  return impl_->beats;
}
std::int64_t Session::uwu() const {
  if (impl_->recording.value("version", 1) == 2)
    return static_cast<std::int64_t>(impl_->beats) * seerin::UWU_PER_BEAT.v;
  return impl_->encounter->visual_time().v;
}
const nlohmann::json& Session::recording() const {
  return impl_->recording;
}
const nlohmann::json& Session::summary() const {
  return impl_->compact_summary;
}
fl::GrandCentral& Session::game() {
  return impl_->game;
}

void Session::write(const std::string& path) const {
  std::ofstream out(path);
  if (!out) throw std::runtime_error("cannot open recording for writing: " + path);
  out << impl_->recording.dump(2) << '\n';
  if (!out) throw std::runtime_error("cannot write recording: " + path);
}

void Session::write_summary(const std::string& path) const {
  std::ofstream out(path);
  if (!out) throw std::runtime_error("cannot open summary for writing: " + path);
  out << impl_->compact_summary.dump(2) << '\n';
  if (!out) throw std::runtime_error("cannot write summary: " + path);
}

std::optional<std::string> Session::first_divergence(const nlohmann::json& expected) const {
  if (expected.value("format", std::string{}) != "fairlanes-combat-recording" ||
      expected.value("version", 0) != impl_->recording.value("version", 0)) {
    return "recording format or version differs";
  }
  if (!expected.contains("build") || !expected.at("build").is_object() ||
      expected.at("build").value("version", std::string{}) != fl::build_identity::version) {
    return "recording build version differs";
  }
  for (const auto* key :
       {"fixture", "content_sha256", "gameplay_seed", "gameplay_sequence", "beat_cap",
        "starting_world_beat", "starting_uwu", "initial_state", "fixture_parameters"}) {
    if (!expected.contains(key) || expected.at(key) != impl_->recording.at(key)) {
      return std::string("recording metadata differs at ") + key;
    }
  }
  if (!expected.contains("external_inputs") || !expected.at("external_inputs").is_array())
    return "recording has no external input array";
  const auto& expected_inputs = expected.at("external_inputs");
  const auto& actual_inputs = impl_->recording.at("external_inputs");
  if (actual_inputs.size() > expected_inputs.size() ||
      !std::equal(actual_inputs.begin(), actual_inputs.end(), expected_inputs.begin()))
    return "recording external inputs differ";
  if (impl_->recording.value("version", 1) == 2) {
    for (const auto* key : {"encounters", "initial_party_state"}) {
      if (!expected.contains(key) || expected.at(key) != impl_->recording.at(key))
        return std::string("recording metadata differs at ") + key;
    }
  }
  if (!expected.contains("events") || !expected.at("events").is_array()) {
    return "recording has no event array";
  }
  const auto& want = expected.at("events");
  const auto& got = impl_->recording.at("events");
  const auto common = std::min(want.size(), got.size());
  for (std::size_t index = 0; index < common; ++index) {
    if (want[index] != got[index]) {
      return "event " + std::to_string(index) + " differs: expected " + want[index].dump() +
             ", got " + got[index].dump();
    }
  }
  if (got.size() > want.size()) {
    return "extra event " + std::to_string(want.size()) + ": " + got[want.size()].dump();
  }
  if (finished()) {
    if (got.size() < want.size()) {
      return "missing event " + std::to_string(got.size()) + ": " + want[got.size()].dump();
    }
    if (!expected.contains("result") || expected.at("result") != impl_->recording.at("result")) {
      return "terminal result differs: expected " + expected.value("result", Json{}).dump() +
             ", got " + impl_->recording.at("result").dump();
    }
    if (impl_->recording.value("version", 1) == 2 &&
        (!expected.contains("encounter_results") ||
         expected.at("encounter_results") != impl_->recording.at("encounter_results") ||
         !expected.contains("final_party_state") ||
         expected.at("final_party_state") != impl_->recording.at("final_party_state")))
      return "progression result differs";
  }
  return std::nullopt;
}

nlohmann::json read_recording(const std::string& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("cannot open recording: " + path);
  auto result = nlohmann::json::parse(in);
  const auto version = result.value("version", 0);
  if (result.value("format", std::string{}) != "fairlanes-combat-recording" ||
      (version != 1 && version != 2)) {
    throw std::runtime_error("unsupported combat recording format or version");
  }
  return result;
}

Options options_from_recording(const nlohmann::json& recording) {
  const auto& parameters = recording.at("fixture_parameters");
  auto options =
      Options{recording.at("fixture").get<std::string>(),
              recording.at("gameplay_seed").get<std::uint64_t>(),
              recording.at("beat_cap").get<std::uint64_t>(), parameters.value("hero_armor", 0),
              parameters.value("variant", std::string{})};
  options.tower_balance.maximum_monster_level = parameters.value("tower_max_level", 20);
  options.tower_balance.maximum_monster_hp = parameters.value("tower_max_hp", 100);
  options.tower_balance.maximum_monster_attack = parameters.value("tower_max_attack", 20);
  return options;
}

} // namespace fl::regression
