#include "ui_command_controller.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include <entt/entt.hpp>

#include "fl/ecs/components/atb_charge.hpp"
#include "fl/ecs/components/dire_bleed.hpp"
#include "fl/ecs/components/freeze.hpp"
#include "fl/ecs/components/poison.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/visual_effects.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::widgets {

namespace {

std::string trim_copy(std::string_view value) {
  while (!value.empty() &&
         std::isspace(static_cast<unsigned char>(value.front()))) {
    value.remove_prefix(1);
  }
  while (!value.empty() &&
         std::isspace(static_cast<unsigned char>(value.back()))) {
    value.remove_suffix(1);
  }
  return std::string(value);
}

std::vector<std::string_view> split_words(std::string_view value) {
  std::vector<std::string_view> words;

  while (!value.empty()) {
    while (!value.empty() &&
           std::isspace(static_cast<unsigned char>(value.front()))) {
      value.remove_prefix(1);
    }

    const auto *start = value.data();
    std::size_t length = 0;
    while (length < value.size() &&
           !std::isspace(static_cast<unsigned char>(value[length]))) {
      ++length;
    }

    if (length > 0) {
      words.emplace_back(start, length);
      value.remove_prefix(length);
    }
  }

  return words;
}

bool parse_index(std::string_view value, std::size_t &out) {
  const auto *first = value.data();
  const auto *last = first + value.size();
  auto [ptr, ec] = std::from_chars(first, last, out);
  return ec == std::errc{} && ptr == last;
}

std::string entity_label(entt::entity entity) {
  return std::to_string(
      static_cast<std::underlying_type_t<entt::entity>>(entity));
}

std::string entity_name(entt::registry &reg, entt::entity entity) {
  if (entity == entt::null) {
    return "<none>";
  }

  using fl::ecs::components::Stats;
  if (auto *stats = reg.try_get<Stats>(entity)) {
    return stats->name_;
  }

  return "entity #" + entity_label(entity);
}

std::string debug_entity_label(entt::registry &reg, entt::entity entity) {
  if (entity == entt::null) {
    return "<none>";
  }

  return entity_name(reg, entity) + " #" + entity_label(entity);
}

bool contains_entity(const std::vector<entt::entity> &entities,
                     entt::entity entity) {
  return std::find(entities.begin(), entities.end(), entity) != entities.end();
}

std::string join_strings(const std::vector<std::string> &parts,
                         std::string_view separator) {
  std::string out;
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i > 0) {
      out += separator;
    }
    out += parts[i];
  }
  return out;
}

std::string ready_names(entt::registry &reg,
                        const std::vector<entt::entity> &ready_queue) {
  if (ready_queue.empty()) {
    return "<empty>";
  }

  std::vector<std::string> names;
  names.reserve(ready_queue.size());
  for (auto entity : ready_queue) {
    names.push_back(debug_entity_label(reg, entity));
  }
  return join_strings(names, ", ");
}

std::size_t alive_count(entt::registry &reg,
                        const std::vector<entt::entity> &entities) {
  using fl::ecs::components::Stats;

  std::size_t count = 0;
  for (auto entity : entities) {
    if (auto *stats = reg.try_get<Stats>(entity)) {
      if (stats->is_alive()) {
        ++count;
      }
    }
  }
  return count;
}

std::string combatant_debug_line(entt::registry &reg, std::string_view role,
                                 entt::entity entity,
                                 const std::vector<entt::entity> &ready_queue,
                                 entt::entity active) {
  namespace c = fl::ecs::components;

  auto line = "  " + std::string(role) + " " + debug_entity_label(reg, entity);

  if (auto *stats = reg.try_get<c::Stats>(entity)) {
    line += " hp=" + std::to_string(stats->hp_) + "/" +
            std::to_string(stats->max_hp_);
    line += stats->is_alive() ? " alive" : " dead";
  } else {
    line += " hp=<missing-stats>";
  }

  if (auto *charge = reg.try_get<c::AtbCharge>(entity)) {
    line += " atb=" + std::to_string(charge->charge) + "/" +
            std::to_string(charge->max_charge);
  } else {
    line += " atb=<missing>";
  }

  std::vector<std::string> statuses;
  if (entity == active) {
    statuses.emplace_back("active");
  }
  if (contains_entity(ready_queue, entity)) {
    statuses.emplace_back("ready");
  }
  if (auto *poison = reg.try_get<c::Poison>(entity)) {
    statuses.emplace_back(
        "poison dmg=" + std::to_string(poison->damage_per_tick) +
        " ticks=" + std::to_string(poison->ticks_remaining) +
        " source=" + entity_name(reg, poison->source));
  }
  if (auto *freeze = reg.try_get<c::Freeze>(entity)) {
    statuses.emplace_back("freeze clear_after=" +
                          std::to_string(freeze->clear_after_beats));
  }
  if (auto *bleed = reg.try_get<c::DireBleed>(entity)) {
    statuses.emplace_back(
        "dire-bleed dmg=" + std::to_string(bleed->damage_per_tick) +
        " source=" + entity_name(reg, bleed->source));
  }
  if (reg.any_of<c::StatusTint>(entity)) {
    statuses.emplace_back("status-tint");
  }
  if (reg.any_of<c::DamageFlash>(entity)) {
    statuses.emplace_back("damage-flash");
  }
  if (reg.any_of<c::ActiveGlow>(entity)) {
    statuses.emplace_back("active-glow");
  }
  if (auto *decals = reg.try_get<c::CombatantDecals>(entity)) {
    statuses.emplace_back("decals=" + std::to_string(decals->effects.size()));
  }

  line += " statuses=";
  line += statuses.empty() ? "none" : join_strings(statuses, "; ");
  return line;
}

} // namespace

UiCommandController::UiCommandController(
    std::deque<fl::primitives::AccountData> &accounts, FancyLog &output_log,
    fl::primitives::WorldClock &world_clock)
    : accounts_(&accounts), output_log_(&output_log),
      world_clock_(&world_clock) {}

void UiCommandController::set_show_account_view(ShowAccountView callback) {
  show_account_view_ = std::move(callback);
}

void UiCommandController::set_show_party_view(ShowPartyView callback) {
  show_party_view_ = std::move(callback);
}

void UiCommandController::handle(std::string_view command) {
  const auto owned = trim_copy(command);
  const auto words = split_words(owned);
  if (words.empty()) {
    return;
  }

  const auto verb = words[0];

  if (verb == "help") {
    show_help(words.size() >= 2 ? words[1] : std::string_view{});
    return;
  }

  if (verb == "screen" || verb == "view") {
    if (words.size() < 2) {
      write("usage: screen account|party");
      write("try: help screen");
      return;
    }

    if (words[1] == "account" || words[1] == "accounts") {
      show_account_view();
      return;
    }

    if (words[1] == "party" || words[1] == "parties") {
      show_party_view();
      return;
    }

    write("unknown screen: " + std::string(words[1]));
    return;
  }

  if (verb == "list" || verb == "ls") {
    if (words.size() < 2) {
      write("usage: list accounts|parties");
      write("try: help list");
      return;
    }

    if (words[1] == "accounts" || words[1] == "account") {
      list_accounts();
      return;
    }

    if (words[1] == "parties" || words[1] == "party") {
      list_parties();
      return;
    }

    write("unknown list target: " + std::string(words[1]));
    return;
  }

  if (verb == "debug" || verb == "dbg" || verb == "ready") {
    if (verb == "ready" || words.size() < 2 || words[1] == "party" ||
        words[1] == "ready") {
      debug_party();
      return;
    }

    write("usage: debug party");
    write("try: help debug");
    return;
  }

  if (verb == "overdrive" || verb == "speed") {
    if (words.size() < 2) {
      write("overdrive is x" +
            std::to_string(world_clock_->beat_rate_multiplier()));
      write("usage: overdrive <1-" +
            std::to_string(fl::primitives::WorldClock::kMaxBeatRateMultiplier) +
            ">");
      return;
    }

    set_overdrive(words[1]);
    return;
  }

  if (verb == "account" || verb == "acct") {
    if (words.size() < 2) {
      write("usage: account <index>");
      write("try: help account");
      return;
    }

    std::size_t index = 0;
    if (!parse_index(words[1], index)) {
      write("account index must be a number");
      return;
    }

    select_account(index);
    return;
  }

  if (verb == "party") {
    if (words.size() < 2) {
      write("usage: party <index>");
      write("try: help party");
      return;
    }

    std::size_t index = 0;
    if (!parse_index(words[1], index)) {
      write("party index must be a number");
      return;
    }

    select_party(index);
    return;
  }

  write("unknown command: " + std::string(verb));
  write("try: help");
}

std::size_t UiCommandController::account_index() const noexcept {
  return account_index_;
}

std::size_t UiCommandController::party_index() const noexcept {
  return party_index_;
}

void UiCommandController::adjust_overdrive(int delta) {
  if (!world_clock_) {
    return;
  }

  const auto current = world_clock_->beat_rate_multiplier();
  world_clock_->set_beat_rate_multiplier(current + delta);
  const auto actual = world_clock_->beat_rate_multiplier();
  write("overdrive set to x" + std::to_string(actual) + " (" +
        std::to_string(world_clock_->effective_beats_per_wall_second()) +
        " beats/sec)");
}

void UiCommandController::select_account_relative(int delta) {
  if (!accounts_ || accounts_->empty()) {
    write("No accounts.");
    return;
  }

  const auto count = static_cast<int>(accounts_->size());
  auto next = (static_cast<int>(account_index_) + delta) % count;
  if (next < 0) {
    next += count;
  }

  select_account(static_cast<std::size_t>(next));
}

void UiCommandController::select_party_relative(int delta) {
  if (!accounts_ || account_index_ >= accounts_->size()) {
    write("No selected account.");
    return;
  }

  auto &parties = accounts_->at(account_index_).parties();
  if (parties.empty()) {
    write("No parties.");
    return;
  }

  const auto count = static_cast<int>(parties.size());
  auto next = (static_cast<int>(party_index_) + delta) % count;
  if (next < 0) {
    next += count;
  }

  select_party(static_cast<std::size_t>(next));
}

void UiCommandController::show_help(std::string_view topic) {
  if (topic.empty()) {
    write("current account: " + std::to_string(account_index_));
    write("current party: " + std::to_string(party_index_));
    write("commands: screen, list, debug, account, party, overdrive, help");
    write("try: help <command>");
    return;
  }

  if (topic == "screen" || topic == "view") {
    write("screen account: show all parties for the selected account");
    write("screen party: show the selected party and inventory");
    return;
  }

  if (topic == "list" || topic == "ls") {
    write("list accounts: show account indices and entity ids");
    write("list parties: show party indices for the selected account");
    return;
  }

  if (topic == "account" || topic == "acct") {
    write("account <index>: select an account by index");
    write("use list accounts to see valid account indices");
    return;
  }

  if (topic == "party") {
    write("party <index>: select a party in the current account");
    write("use list parties to see valid party indices");
    return;
  }

  if (topic == "debug" || topic == "dbg" || topic == "ready") {
    write("debug party: show current party combat/ATB state");
    write("ready: shortcut for debug party");
    return;
  }

  if (topic == "overdrive" || topic == "speed") {
    write("overdrive <multiplier>: run world beats faster in wall time");
    write("overdrive 1: return to normal time");
    return;
  }

  if (topic == "help") {
    write("help: show context-sensitive command help");
    write("help <command>: show help for one command");
    return;
  }

  write("no help for: " + std::string(topic));
  write("try: help");
}

void UiCommandController::show_account_view() {
  if (show_account_view_) {
    show_account_view_(account_index_);
  }
  write("showing account view");
}

void UiCommandController::show_party_view() {
  if (show_party_view_) {
    show_party_view_(account_index_, party_index_);
  }
  write("showing party view");
}

void UiCommandController::list_accounts() {
  if (!accounts_ || accounts_->empty()) {
    write("No accounts.");
    return;
  }

  for (std::size_t i = 0; i < accounts_->size(); ++i) {
    auto &account = accounts_->at(i);
    write((i == account_index_ ? "* " : "  ") + std::to_string(i) +
          ": account #" + entity_label(account.account_id()) +
          " parties=" + std::to_string(account.parties().size()));
  }
}

void UiCommandController::list_parties() {
  if (!accounts_ || account_index_ >= accounts_->size()) {
    write("No selected account.");
    return;
  }

  auto &parties = accounts_->at(account_index_).parties();
  if (parties.empty()) {
    write("No parties.");
    return;
  }

  for (std::size_t i = 0; i < parties.size(); ++i) {
    auto &party = parties[i];
    write((i == party_index_ ? "* " : "  ") + std::to_string(i) + ": " +
          std::string(party.name()) + " #" + entity_label(party.party_id()) +
          " members=" + std::to_string(party.members().size()) +
          " items=" + std::to_string(party.items().size()));
  }
}

void UiCommandController::debug_party() {
  if (!accounts_ || account_index_ >= accounts_->size()) {
    write("No selected account.");
    return;
  }

  auto &account = accounts_->at(account_index_);
  auto &parties = account.parties();
  if (party_index_ >= parties.size()) {
    write("No selected party.");
    return;
  }

  auto &party = parties[party_index_];
  auto &reg = party.party_ctx().reg();

  write("debug party account=" + std::to_string(account_index_) + " party=" +
        std::to_string(party_index_) + " name=" + std::string(party.name()) +
        " entity=#" + entity_label(party.party_id()));

  if (!party.has_encounter()) {
    write("encounter=<none> members=" + std::to_string(party.members().size()));
    for (auto &member : party.members()) {
      write(combatant_debug_line(reg, "member", member.member_id(), {},
                                 entt::null));
    }
    return;
  }

  auto &encounter = party.encounter_data();
  auto &atb = encounter.atb_engine();
  const auto &ready = atb.ready_queue();
  const auto active = atb.active_combatant();

  write("encounter=in-combat visual_time=" +
        std::to_string(encounter.visual_time().v) + " pending_events=" +
        std::to_string(encounter.pending_scheduled_events()) +
        " legacy_ready_size=" + std::to_string(encounter.ready_queue().size()));

  if (!encounter.ready_queue().empty()) {
    const auto &legacy_front = encounter.ready_queue().front();
    write("legacy_ready_front actor=" +
          debug_entity_label(reg, legacy_front.actor) +
          " target=" + debug_entity_label(reg, legacy_front.target) +
          " nonce=" + std::to_string(legacy_front.nonce));
  }

  write("atb_active=" + debug_entity_label(reg, active));
  write("atb_ready size=" + std::to_string(ready.size()) + ": " +
        ready_names(reg, ready));

  const auto &attackers = encounter.attackers().members();
  const auto &defenders = encounter.defenders().members();
  write("attackers total=" + std::to_string(attackers.size()) +
        " alive=" + std::to_string(alive_count(reg, attackers)));
  for (auto entity : attackers) {
    write(combatant_debug_line(reg, "attacker", entity, ready, active));
  }

  write("defenders total=" + std::to_string(defenders.size()) +
        " alive=" + std::to_string(alive_count(reg, defenders)));
  for (auto entity : defenders) {
    write(combatant_debug_line(reg, "defender", entity, ready, active));
  }
}

void UiCommandController::set_overdrive(std::string_view multiplier) {
  std::size_t value = 0;
  if (!parse_index(multiplier, value) || value == 0) {
    write("overdrive multiplier must be a positive whole number");
    return;
  }

  world_clock_->set_beat_rate_multiplier(static_cast<int>(value));
  const auto actual = world_clock_->beat_rate_multiplier();
  write("overdrive set to x" + std::to_string(actual) + " (" +
        std::to_string(world_clock_->effective_beats_per_wall_second()) +
        " beats/sec)");
}

void UiCommandController::select_account(std::size_t account_index) {
  if (!accounts_ || account_index >= accounts_->size()) {
    write("no account at index " + std::to_string(account_index));
    return;
  }

  account_index_ = account_index;
  party_index_ = 0;
  if (show_party_view_) {
    show_party_view_(account_index_, party_index_);
  }
  write("selected account " + std::to_string(account_index_));
}

void UiCommandController::select_party(std::size_t party_index) {
  if (!accounts_ || account_index_ >= accounts_->size()) {
    write("No selected account.");
    return;
  }

  auto &parties = accounts_->at(account_index_).parties();
  if (party_index >= parties.size()) {
    write("no party at index " + std::to_string(party_index));
    return;
  }

  party_index_ = party_index;
  if (show_party_view_) {
    show_party_view_(account_index_, party_index_);
  }
  write("selected party " + std::to_string(party_index_));
}

void UiCommandController::write(std::string_view line) {
  if (output_log_) {
    output_log_->append_plain(std::string(line));
  }
}

} // namespace fl::widgets
