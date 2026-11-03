#include "fl/regression/session.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include <entt/entity/entity.hpp>
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
#include "fl/ecs/systems/manual_equipment.hpp"
#include "fl/ecs/systems/effective_combat_stats.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/generated/floor_content.hpp"
#include "fl/generated/regression_content.hpp"
#include "fl/grand_central.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "fl/primitives/encounter_data.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/world_clock.hpp"
#include "fl/skills/skill.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/generated/monster_content.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::regression {
namespace {

using Json = nlohmann::json;
using fl::ecs::components::Stats;

std::string skill_id(fl::skills::SkillKey skill) {
  return std::string(fl::skills::stable_id(skill.base)) + "/" +
         std::to_string(skill.rank.value());
}

Json stats_json(entt::registry &reg, entt::entity actor) {
  const auto &stats = reg.get<Stats>(actor);
  const auto effective = fl::ecs::systems::effective_combat_stats(reg, actor);
  const auto &r = effective.resistances;
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

Json atb_json(const fl::ecs::components::AtbCharge &atb) {
  return {{"charge", atb.charge},
          {"max_charge", atb.max_charge},
          {"charge_per_beat", atb.charge_per_beat}};
}

Json statuses_for(entt::registry &reg, entt::entity actor) {
  std::vector<std::string> statuses;
  if (const auto *effects =
          reg.try_get<fl::ecs::components::CombatStatuses>(actor)) {
    for (const auto &effect : effects->effects) {
      statuses.push_back(effect.name);
    }
  }
  if (reg.all_of<fl::ecs::components::Poison>(actor))
    statuses.emplace_back("Poison");
  if (reg.all_of<fl::ecs::components::Freeze>(actor))
    statuses.emplace_back("Freeze");
  if (reg.all_of<fl::ecs::components::DireBleed>(actor))
    statuses.emplace_back("Dire Bleed");
  std::ranges::sort(statuses);
  return statuses;
}

int party_size_for(std::string_view fixture) {
  const auto *scenario = generated_content::find(fixture);
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
  fl::primitives::EncounterData *encounter{};
  std::vector<Actor> actors;
  std::size_t hero_count{1};
  std::vector<fl::events::ScopedCombatantListener> listeners;
  Json recording;
  std::uint64_t beats{0};
  std::string outcome;

  explicit Impl(Options selected)
      : options(std::move(selected)),
        game(1, 1, party_size_for(options.fixture), false, false, options.seed),
        account_ctx(game.account_context(0)),
        party_ctx(account_ctx.party_context(0)) {
    if (const auto *scenario = generated_content::find(options.fixture)) {
      setup_generated(*scenario);
      return;
    }
    const bool armor_fixture = options.fixture == kArmorFixture;
    if (options.fixture != kSmokeFixture && !armor_fixture) {
      throw std::invalid_argument("unknown regression fixture: " +
                                  options.fixture);
    }
    if (options.hero_armor < 0 ||
        (!armor_fixture && options.hero_armor != 0)) {
      throw std::invalid_argument("invalid Armor for regression fixture");
    }
    if (options.beat_cap == 0) {
      throw std::invalid_argument("beat cap must be positive");
    }

    auto &hero_member = party_ctx.party_data().members().front();
    const auto hero = hero_member.member_id();
    party_ctx.party_data().preserve_terminal_encounter(true);
    auto &hero_stats = game.reg().get<Stats>(hero);
    hero_stats.name_ = "Regression Hero";
    hero_stats.hp_ = hero_stats.max_hp_ = armor_fixture ? 8 : 20;
    hero_stats.attack_ = armor_fixture ? 0 : 2;
    hero_stats.armor_ = options.hero_armor;
    hero_stats.resistances_ = {};
    auto &party_member = game.reg().get<fl::ecs::components::PartyMember>(hero);
    party_member.closet().unequip_skill(fl::skills::SkillId::Observe);
    hero_member.grimoire().learn(fl::skills::SkillId::Thump);
    if (!party_member.equip_known_skill(fl::skills::SkillId::Thump)) {
      throw std::runtime_error("could not equip fixture Thump");
    }

    auto &built = fl::primitives::EncounterBuilder{party_ctx}.one_on_one(
        armor_fixture ? fl::monster::MonsterKind::BumpkinHare
                      : fl::monster::MonsterKind::FieldMouse);
    encounter = &built;
    const auto mouse = built.attackers().members().front();
    auto &mouse_stats = game.reg().get<Stats>(mouse);
    mouse_stats.hp_ = mouse_stats.max_hp_ = armor_fixture ? 14 : 8;
    mouse_stats.attack_ = 0;
    mouse_stats.armor_ = 0;
    mouse_stats.resistances_ = {};
    const auto enemy_skill = armor_fixture ? fl::skills::SkillId::Bump
                                           : fl::skills::SkillId::Thump;
    game.reg().replace<fl::ecs::components::SkillSlots>(
        mouse, fl::ecs::components::SkillSlots::with_known(
                   fl::skills::SkillKey{enemy_skill}));

    actors = {{"hero", hero, {}}, {armor_fixture ? "hare" : "mouse", mouse, {}}};
    for (auto &actor : actors)
      actor.prior_state = state(actor.entity);

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
        {"fixture_parameters", {{"hero_armor", options.hero_armor}}},
        {"starting_world_beat", game.world_clock().elapsed_beats()},
        {"starting_uwu", encounter->visual_time().v},
        {"expected_outcome", armor_fixture ? Json(nullptr) : Json("hero_victory")},
        {"external_inputs", Json::array()},
        {"initial_state",
         Json::array(
             {{{"id", "hero"},
               {"kind", "player"},
               {"name", hero_stats.name_},
               {"stats", stats_json(game.reg(), hero)},
               {"atb",
                atb_json(game.reg().get<fl::ecs::components::AtbCharge>(hero))},
               {"statuses", statuses_for(game.reg(), hero)},
               {"loadout", Json::array({"thump/1"})},
               {"known_skills", Json::array({"observe/1", "thump/1"})},
               {"action_policy", "single_equipped_skill"},
               {"gear", Json::array()}},
              {{"id", armor_fixture ? "hare" : "mouse"},
               {"kind", armor_fixture ? "bumpkin_hare" : "field_mouse"},
               {"name", mouse_stats.name_},
               {"stats", stats_json(game.reg(), mouse)},
               {"atb", atb_json(game.reg().get<fl::ecs::components::AtbCharge>(
                           mouse))},
               {"statuses", statuses_for(game.reg(), mouse)},
               {"loadout", Json::array({armor_fixture ? "bump/1" : "thump/1"})},
               {"known_skills", Json::array({armor_fixture ? "bump/1" : "thump/1"})},
               {"action_policy", "single_known_skill"},
               {"gear", Json::array()}}})},
        {"events", Json::array()},
        {"result", nullptr}};

    for (const auto &actor : actors)
      bind_events(actor.entity);
    game.innervate_event_system(false);
  }

  void setup_generated(const generated_content::Scenario &scenario) {
    const auto *variant = generated_content::find_variant(scenario, options.variant);
    if (!variant) throw std::invalid_argument("unknown regression variant: " + options.variant);
    if (options.hero_armor != 0)
      throw std::invalid_argument("generated regression does not accept --armor");
    if (options.beat_cap != scenario.beat_cap)
      throw std::invalid_argument("generated regression beat cap differs from declaration");

    auto &party = party_ctx.party_data();
    party.preserve_terminal_encounter(true);
    std::size_t index = 0;
    for (auto &member : party.members()) {
      const auto setup = scenario.party[index];
      if (setup.policy != generated_content::ActionPolicy::SingleEquippedSkill)
        throw std::invalid_argument("unsupported regression action policy");
      const auto entity = member.member_id();
      auto &stats = game.reg().get<Stats>(entity);
      stats.name_ = "Regression Hero " + std::to_string(index + 1);
      stats.hp_ = stats.max_hp_ = setup.hp;
      stats.attack_ = setup.attack;
      stats.armor_ = setup.armor;
      stats.resistances_ = setup.resistances;
      auto &party_member = game.reg().get<fl::ecs::components::PartyMember>(entity);
      party_member.closet().unequip_skill(fl::skills::SkillId::Observe);
      member.grimoire().learn(setup.skill);
      if (!party_member.equip_known_skill(setup.skill))
        throw std::runtime_error("could not equip generated regression skill");
      actors.push_back({"hero" + std::to_string(index + 1), entity, {}});
      ++index;
    }
    hero_count = actors.size();

    // The variant item travels through inventory and the ordinary equip system.
    const auto item = fl::loot::generated_content::definition(
        variant->equipped_item).item.create(game.reg());
    party.add_item(item);
    const auto &gear = game.reg().get<fl::ecs::components::Equipment>(item);
    const auto request = fl::events::EquipmentEquipRequested{
        actors.front().entity, gear.slot(), item};
    if (!fl::ecs::systems::ManualEquipmentSystem::commit(party_ctx, request))
      throw std::runtime_error("could not equip regression variant item");
    party.revitalize_members();

    auto &built = fl::primitives::EncounterBuilder{
        party_ctx, fl::primitives::EncounterMode::HeroesJourney,
        fl::primitives::EncounterBuilder::TowerFloor{
            std::string(scenario.biome_id), scenario.floor}}.thump_it_out();
    encounter = &built;
    const auto *floor = fl::primitives::generated_content::floor_lineup(
        scenario.biome_id, scenario.floor);
    for (std::size_t i = 0; i < built.attackers().members().size(); ++i)
      actors.push_back({"enemy" + std::to_string(i + 1),
                        built.attackers().members()[i], {}});
    for (auto &actor : actors) actor.prior_state = state(actor.entity);

    Json initial = Json::array();
    for (std::size_t i = 0; i < actors.size(); ++i) {
      const auto &actor = actors[i];
      const auto &stats = game.reg().get<Stats>(actor.entity);
      const bool hero = i < hero_count;
      Json known = Json::array();
      if (hero) {
        known.push_back(skill_id(fl::skills::SkillKey{scenario.party[i].skill}));
      } else {
        for (const auto skill : fl::monster::generated_content::known_skills(
                 floor->monsters[i - hero_count]))
          known.push_back(skill_id(fl::skills::SkillKey{skill}));
      }
      initial.push_back({{"id", actor.id},
                         {"kind", hero ? "player" : "monster"},
                         {"name", stats.name_},
                         {"stats", stats_json(game.reg(), actor.entity)},
                         {"atb", atb_json(game.reg().get<fl::ecs::components::AtbCharge>(actor.entity))},
                         {"statuses", statuses_for(game.reg(), actor.entity)},
                         {"loadout", known},
                         {"known_skills", known},
                         {"action_policy", hero ? "single_equipped_skill" : "monster_decision_rules"},
                         {"gear", hero && i == 0 ? Json::array({std::string(gear.unique_id())}) : Json::array()}});
    }
    recording = {{"format", "fairlanes-combat-recording"},
                 {"version", 1},
                 {"fixture", options.fixture},
                 {"build", {{"version", fl::build_identity::version},
                             {"revision", fl::build_identity::revision},
                             {"source_dirty", fl::build_identity::source_dirty}}},
                 {"content_sha256", fl::build_identity::content_sha256},
                 {"gameplay_seed", options.seed},
                 {"gameplay_sequence", game.rng().base_sequence()},
                 {"beat_cap", options.beat_cap},
                 {"fixture_parameters", {{"variant", options.variant},
                                         {"equipped_item", std::string(gear.unique_id())}}},
                 {"starting_world_beat", game.world_clock().elapsed_beats()},
                 {"starting_uwu", encounter->visual_time().v},
                 {"expected_outcome", nullptr},
                 {"external_inputs", Json::array()},
                 {"initial_state", initial},
                 {"events", Json::array()},
                 {"result", nullptr}};
    for (const auto &actor : actors) bind_events(actor.entity);
    game.innervate_event_system(false);
  }

  std::string_view actor_id(entt::entity entity) const {
    for (const auto &actor : actors) {
      if (actor.entity == entity)
        return actor.id;
    }
    throw std::runtime_error("combat event referenced an unknown actor");
  }

  Json state(entt::entity entity) {
    return {{"hp", game.reg().get<Stats>(entity).hp_},
            {"statuses", statuses_for(game.reg(), entity)}};
  }

  void append(Json event) {
    auto &events = recording["events"];
    event["seq"] = events.size();
    event["uwu"] = encounter->visual_time().v;
    events.push_back(std::move(event));
  }

  void bind_events(entt::entity actor) {
    auto &bus = encounter->combatant_bus(actor);
    listeners.emplace_back(bus,
                           std::in_place_type<fl::events::SkillActionSelected>,
                           [this](const auto &event) {
                             append({{"type", "action"},
                                     {"actor", actor_id(event.actor)},
                                     {"target", actor_id(event.target)},
                                     {"skill", skill_id(event.skill)}});
                           });
    listeners.emplace_back(bus,
                           std::in_place_type<fl::events::SkillCastStarted>,
                           [this](const auto &event) {
                             append({{"type", "cast_start"},
                                     {"actor", actor_id(event.caster)},
                                     {"skill", skill_id(event.skill)}});
                           });
    listeners.emplace_back(
        bus, std::in_place_type<fl::events::SkillHitLanded>,
        [this](const auto &event) {
          append({{"type", "hit"},
                  {"actor", actor_id(event.source)},
                  {"target", actor_id(event.target)},
                  {"skill", skill_id(event.skill)},
                  {"damage", event.damage},
                  {"hp_after", game.reg().get<Stats>(event.target).hp_}});
        });
    listeners.emplace_back(
        bus, std::in_place_type<fl::events::SkillCastFinished>,
        [this](const auto &event) {
          append({{"type", "cast_finish"}, {"actor", actor_id(event.caster)}});
        });
  }

  void observe() {
    if (!outcome.empty())
      return;
    ++beats;
    for (auto &actor : actors) {
      auto next = state(actor.entity);
      if (next != actor.prior_state) {
        append({{"type", "state"},
                {"actor", actor.id},
                {"before", actor.prior_state},
                {"after", next}});
        actor.prior_state = std::move(next);
      }
    }
    const bool hero_alive = std::ranges::any_of(
        actors.begin(), actors.begin() + static_cast<std::ptrdiff_t>(hero_count),
        [this](const Actor &actor) { return game.reg().get<Stats>(actor.entity).is_alive(); });
    const bool enemy_alive = std::ranges::any_of(
        actors.begin() + static_cast<std::ptrdiff_t>(hero_count), actors.end(),
        [this](const Actor &actor) { return game.reg().get<Stats>(actor.entity).is_alive(); });
    if (!hero_alive && !enemy_alive)
      outcome = "mutual_destruction";
    else if (!enemy_alive)
      outcome = "hero_victory";
    else if (!hero_alive)
      outcome = options.fixture == kArmorFixture ? "hare_victory" :
                options.fixture == kSmokeFixture ? "mouse_victory" : "enemy_victory";
    else if (beats >= options.beat_cap)
      outcome = "beat_cap";

    if (!outcome.empty()) {
      append({{"type", "end"}, {"outcome", outcome}});
      Json remaining_hp = Json::object();
      for (const auto &actor : actors)
        remaining_hp[actor.id] = game.reg().get<Stats>(actor.entity).hp_;
      recording["result"] = {{"outcome", outcome},
                             {"beats", beats},
                             {"uwu", encounter->visual_time().v},
                             {"remaining_hp", remaining_hp},
                             {"simulated_seconds",
                              static_cast<double>(beats) /
                                  fl::primitives::WorldClock::kBeatsPerSecond}};
    }
  }
};

Session::Session(Options options)
    : impl_(std::make_unique<Impl>(std::move(options))) {}
Session::~Session() = default;
void Session::advance_beat() {
  if (finished())
    return;
  impl_->game.advance_beat();
  impl_->observe();
}
void Session::observe_beat() { impl_->observe(); }
bool Session::finished() const { return !impl_->outcome.empty(); }
std::string_view Session::outcome() const { return impl_->outcome; }
std::uint64_t Session::beats() const { return impl_->beats; }
std::int64_t Session::uwu() const { return impl_->encounter->visual_time().v; }
const nlohmann::json &Session::recording() const { return impl_->recording; }
fl::GrandCentral &Session::game() { return impl_->game; }

void Session::write(const std::string &path) const {
  std::ofstream out(path);
  if (!out)
    throw std::runtime_error("cannot open recording for writing: " + path);
  out << impl_->recording.dump(2) << '\n';
  if (!out)
    throw std::runtime_error("cannot write recording: " + path);
}

std::optional<std::string>
Session::first_divergence(const nlohmann::json &expected) const {
  if (!expected.contains("build") || !expected.at("build").is_object() ||
      expected.at("build").value("version", std::string{}) !=
          fl::build_identity::version) {
    return "recording build version differs";
  }
  for (const auto *key :
       {"fixture", "content_sha256", "gameplay_seed", "gameplay_sequence",
        "beat_cap", "starting_world_beat", "starting_uwu", "initial_state",
        "external_inputs", "fixture_parameters"}) {
    if (!expected.contains(key) ||
        expected.at(key) != impl_->recording.at(key)) {
      return std::string("recording metadata differs at ") + key;
    }
  }
  if (!expected.contains("events") || !expected.at("events").is_array()) {
    return "recording has no event array";
  }
  const auto &want = expected.at("events");
  const auto &got = impl_->recording.at("events");
  const auto common = std::min(want.size(), got.size());
  for (std::size_t index = 0; index < common; ++index) {
    if (want[index] != got[index]) {
      return "event " + std::to_string(index) + " differs: expected " +
             want[index].dump() + ", got " + got[index].dump();
    }
  }
  if (got.size() > want.size()) {
    return "extra event " + std::to_string(want.size()) + ": " +
           got[want.size()].dump();
  }
  if (finished()) {
    if (got.size() < want.size()) {
      return "missing event " + std::to_string(got.size()) + ": " +
             want[got.size()].dump();
    }
    if (!expected.contains("result") ||
        expected.at("result") != impl_->recording.at("result")) {
      return "terminal result differs: expected " +
             expected.value("result", Json{}).dump() + ", got " +
             impl_->recording.at("result").dump();
    }
  }
  return std::nullopt;
}

nlohmann::json read_recording(const std::string &path) {
  std::ifstream in(path);
  if (!in)
    throw std::runtime_error("cannot open recording: " + path);
  auto result = nlohmann::json::parse(in);
  if (result.value("format", std::string{}) != "fairlanes-combat-recording" ||
      result.value("version", 0) != 1) {
    throw std::runtime_error("unsupported combat recording format or version");
  }
  return result;
}

Options options_from_recording(const nlohmann::json &recording) {
  const auto &parameters = recording.at("fixture_parameters");
  return Options{recording.at("fixture").get<std::string>(),
                 recording.at("gameplay_seed").get<std::uint64_t>(),
                 recording.at("beat_cap").get<std::uint64_t>(),
                 parameters.value("hero_armor", 0),
                 parameters.value("variant", std::string{})};
}

} // namespace fl::regression
