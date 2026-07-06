#pragma once

#include <cstddef>
#include <deque>
#include <functional>
#include <string_view>

#include "fl/primitives/account_data.hpp"
#include "fl/primitives/world_clock.hpp"

namespace fl::widgets {

class FancyLog;

class UiCommandController {
public:
  using ShowAccountView = std::function<void(std::size_t account_index)>;
  using ShowPartyView =
      std::function<void(std::size_t account_index, std::size_t party_index)>;
  using ShowEffectGallery = std::function<void()>;

  explicit UiCommandController(
      std::deque<fl::primitives::AccountData> &accounts, FancyLog &output_log,
      fl::primitives::WorldClock &world_clock);

  void set_show_account_view(ShowAccountView callback);
  void set_show_party_view(ShowPartyView callback);
  void set_show_effect_gallery(ShowEffectGallery callback);

  void handle(std::string_view command);

  [[nodiscard]] std::size_t account_index() const noexcept;
  [[nodiscard]] std::size_t party_index() const noexcept;

  void adjust_overdrive(int delta);
  void select_account_relative(int delta);
  void select_party_relative(int delta);

private:
  void show_help(std::string_view topic = {});
  void show_account_view();
  void show_party_view();
  void show_effect_gallery();
  void list_accounts();
  void list_parties();
  void debug_party();
  void set_overdrive(std::string_view multiplier);
  void select_account(std::size_t account_index);
  void select_party(std::size_t party_index);
  void write(std::string_view line);

  std::deque<fl::primitives::AccountData> *accounts_{nullptr};
  FancyLog *output_log_{nullptr};
  fl::primitives::WorldClock *world_clock_{nullptr};
  std::size_t account_index_{0};
  std::size_t party_index_{0};

  ShowAccountView show_account_view_;
  ShowPartyView show_party_view_;
  ShowEffectGallery show_effect_gallery_;
};

} // namespace fl::widgets
