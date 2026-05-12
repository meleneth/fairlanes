#pragma once

#include <cstddef>
#include <deque>
#include <functional>
#include <string_view>

#include "fl/primitives/account_data.hpp"

namespace fl::widgets {

class FancyLog;

class UiCommandController {
public:
  using ShowAccountView = std::function<void(std::size_t account_index)>;
  using ShowPartyView =
      std::function<void(std::size_t account_index, std::size_t party_index)>;

  explicit UiCommandController(
      std::deque<fl::primitives::AccountData> &accounts, FancyLog &output_log);

  void set_show_account_view(ShowAccountView callback);
  void set_show_party_view(ShowPartyView callback);

  void handle(std::string_view command);

  [[nodiscard]] std::size_t account_index() const noexcept;
  [[nodiscard]] std::size_t party_index() const noexcept;

private:
  void show_help(std::string_view topic = {});
  void show_account_view();
  void show_party_view();
  void list_accounts();
  void list_parties();
  void select_account(std::size_t account_index);
  void select_party(std::size_t party_index);
  void write(std::string_view line);

  std::deque<fl::primitives::AccountData> *accounts_{nullptr};
  FancyLog *output_log_{nullptr};
  std::size_t account_index_{0};
  std::size_t party_index_{0};

  ShowAccountView show_account_view_;
  ShowPartyView show_party_view_;
};

} // namespace fl::widgets
