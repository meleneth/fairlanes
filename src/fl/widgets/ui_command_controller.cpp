#include "ui_command_controller.hpp"

#include <charconv>
#include <cctype>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include <entt/entt.hpp>

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
  return std::to_string(static_cast<std::underlying_type_t<entt::entity>>(
      entity));
}

} // namespace

UiCommandController::UiCommandController(
    std::deque<fl::primitives::AccountData> &accounts, FancyLog &output_log)
    : accounts_(&accounts), output_log_(&output_log) {}

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

void UiCommandController::show_help(std::string_view topic) {
  if (topic.empty()) {
    write("current account: " + std::to_string(account_index_));
    write("current party: " + std::to_string(party_index_));
    write("commands: screen, list, account, party, help");
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
          ": account #" + entity_label(account.account_id()) + " parties=" +
          std::to_string(account.parties().size()));
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
          " members=" + std::to_string(party.members().size()) + " items=" +
          std::to_string(party.items().size()));
  }
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
