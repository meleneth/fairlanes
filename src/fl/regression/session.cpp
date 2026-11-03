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
#include "fl/ecs/components/freeze.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/poison.hpp"
#include "fl/ecs/components/skill_slots.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/grand_central.hpp"
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

std::string_view skill_id(fl::skills::SkillKey skill) {
  if (skill.base == fl::skills::SkillId::Thump && skill.rank.value() == 1) {
    return "thump/1";
  }
  throw std::runtime_error("smoke fixture selected an undeclared skill");
}

Json stats_json(const Stats &stats) {
  const auto &r = stats.resistances_;
  return {{"hp", stats.hp_},
          {"max_hp", stats.max_hp_},
          {"attack", stats.attack_},
          {"armor", stats.armor_},
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

} // namespace

struct Session::Impl {
  struct Actor {
    std::string_view id;
    entt::entity entity;
    Json prior_state;
  };

  Options options;
  GrandCentral game;
  fl::context::AccountCtx account_ctx;
  fl::context::PartyCtx party_ctx;
  fl::primitives::EncounterData *encounter{};
  std::array<Actor, 2> actors;
  std::vector<fl::events::ScopedCombatantListener> listeners;
  Json recording;
  std::uint64_t beats{0};
  std::string outcome;

  explicit Impl(Options selected)
      : options(std::move(selected)), game(1, 1, 1, false, false, options.seed),
        account_ctx(game.account_context(0)),
        party_ctx(account_ctx.party_context(0)) {
    if (options.fixture != kSmokeFixture) {
      throw std::invalid_argument("unknown regression fixture: " +
                                  options.fixture);
    }
    if (options.beat_cap == 0) {
      throw std::invalid_argument("beat cap must be positive");
    }

    auto &hero_member = party_ctx.party_data().members().front();
    const auto hero = hero_member.member_id();
    party_ctx.party_data().preserve_terminal_encounter(true);
    auto &hero_stats = game.reg().get<Stats>(hero);
    hero_stats.name_ = "Regression Hero";
    hero_stats.hp_ = hero_stats.max_hp_ = 20;
    hero_stats.attack_ = 2;
    hero_stats.armor_ = 0;
    hero_stats.resistances_ = {};
    auto &party_member = game.reg().get<fl::ecs::components::PartyMember>(hero);
    party_member.closet().unequip_skill(fl::skills::SkillId::Observe);
    hero_member.grimoire().learn(fl::skills::SkillId::Thump);
    if (!party_member.equip_known_skill(fl::skills::SkillId::Thump)) {
      throw std::runtime_error("could not equip fixture Thump");
    }

    auto &built = fl::primitives::EncounterBuilder{party_ctx}.one_on_one(
        fl::monster::MonsterKind::FieldMouse);
    encounter = &built;
    const auto mouse = built.attackers().members().front();
    auto &mouse_stats = game.reg().get<Stats>(mouse);
    mouse_stats.hp_ = mouse_stats.max_hp_ = 8;
    mouse_stats.attack_ = 0;
    mouse_stats.armor_ = 0;
    mouse_stats.resistances_ = {};
    game.reg().replace<fl::ecs::components::SkillSlots>(
        mouse, fl::ecs::components::SkillSlots::with_known(
                   fl::skills::SkillKey{fl::skills::SkillId::Thump}));

    actors = {{{"hero", hero, {}}, {"mouse", mouse, {}}}};
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
        {"starting_world_beat", game.world_clock().elapsed_beats()},
        {"starting_uwu", encounter->visual_time().v},
        {"expected_outcome", "hero_victory"},
        {"external_inputs", Json::array()},
        {"initial_state",
         Json::array(
             {{{"id", "hero"},
               {"kind", "player"},
               {"name", hero_stats.name_},
               {"stats", stats_json(hero_stats)},
               {"atb",
                atb_json(game.reg().get<fl::ecs::components::AtbCharge>(hero))},
               {"statuses", statuses_for(game.reg(), hero)},
               {"loadout", Json::array({"thump/1"})},
               {"known_skills", Json::array({"observe/1", "thump/1"})},
               {"action_policy", "single_equipped_skill"},
               {"gear", Json::array()}},
              {{"id", "mouse"},
               {"kind", "field_mouse"},
               {"name", mouse_stats.name_},
               {"stats", stats_json(mouse_stats)},
               {"atb", atb_json(game.reg().get<fl::ecs::components::AtbCharge>(
                           mouse))},
               {"statuses", statuses_for(game.reg(), mouse)},
               {"loadout", Json::array({"thump/1"})},
               {"known_skills", Json::array({"thump/1"})},
               {"action_policy", "single_known_skill"},
               {"gear", Json::array()}}})},
        {"events", Json::array()},
        {"result", nullptr}};

    for (const auto &actor : actors)
      bind_events(actor.entity);
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
    const bool hero_alive = game.reg().get<Stats>(actors[0].entity).is_alive();
    const bool mouse_alive = game.reg().get<Stats>(actors[1].entity).is_alive();
    if (!hero_alive && !mouse_alive)
      outcome = "mutual_destruction";
    else if (!mouse_alive)
      outcome = "hero_victory";
    else if (!hero_alive)
      outcome = "mouse_victory";
    else if (beats >= options.beat_cap)
      outcome = "beat_cap";

    if (!outcome.empty()) {
      append({{"type", "end"}, {"outcome", outcome}});
      recording["result"] = {{"outcome", outcome},
                             {"beats", beats},
                             {"uwu", encounter->visual_time().v},
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
        "external_inputs"}) {
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
  return Options{recording.at("fixture").get<std::string>(),
                 recording.at("gameplay_seed").get<std::uint64_t>(),
                 recording.at("beat_cap").get<std::uint64_t>()};
}

} // namespace fl::regression
