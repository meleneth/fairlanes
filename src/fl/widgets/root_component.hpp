#pragma once

#include <chrono>
#include <cstddef>
#include <deque>
#include <string_view>
#include <vector>

#include <ftxui/component/component.hpp>

#include "fl/context.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/fwd.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/discovery_journal.hpp"
#include "fl/primitives/objective_system.hpp"
#include "fl/primitives/raid_milestones.hpp"
#include "fl/primitives/world_clock.hpp"
#include "fl/widgets/important_notice_overlay.hpp"
#include "fl/widgets/ui_command_controller.hpp"

namespace fl::widgets {

class ConsoleOverlay;
class FancyLog;

class RootComponent : public ftxui::ComponentBase {
public:
  RootComponent(fl::context::AccountCtx ctx,
                std::deque<fl::primitives::AccountData> &accounts,
                FancyLog &console_log, fl::primitives::WorldClock &world_clock,
                const fl::primitives::DiscoveryJournal *discoveries = nullptr,
                fl::events::AchievementBus *achievements = nullptr,
                fl::primitives::ObjectiveSystem *objectives = nullptr);

  bool OnEvent(ftxui::Event event) override;
  ftxui::Element Render() override;
  ftxui::Component ActiveChild() override;

  bool quit_requested() const { return quit_requested_; }
  bool sandbox_requested() const { return sandbox_requested_; }
  bool console_open() const;
  std::size_t console_history_size() const;
  std::string_view console_input_line() const;
  std::string_view active_screen_id() const noexcept;
  std::string_view focus_owner() const noexcept;

  void toggle_console();
  void focus_console();
  void restore_active_focus();
  void set_full_open();
  bool show_generated_screen(std::string_view screen);
  bool invoke_generated_action(std::string_view action);

  void show_account_battle(std::size_t account_index);
  void show_party_battle(std::size_t account_index, std::size_t party_index);
  void show_party(std::size_t account_index, std::size_t party_index);
  void show_effect_gallery();
  void show_objectives();
  void show_village();
  void show_libram(std::optional<fl::skills::SkillId> skill = std::nullopt);
  void
  show_bestiary(std::optional<fl::monster::MonsterKind> monster = std::nullopt);

private:
  enum class ActiveScreen {
    raid,
    party,
    account_battle,
    party_battle,
    effect_gallery,
    libram,
    bestiary,
    story,
  };

  void replace_screen(ftxui::Component screen);
  void on_party_combat_started(entt::entity account);
  bool raid_screen_locked() const noexcept;
  ConsoleOverlay *console_overlay();
  fl::context::AccountCtx make_context(std::size_t account_index);
  void toggle_active_screen();
  void toggle_party_battle_screen();
  void return_from_story();
  bool village_available_from_current_screen() const noexcept;
  bool return_selected_party_to_town();
  void refresh_selected_account(std::size_t account_index,
                                std::size_t party_index);
  void update_fps_counter();
  ftxui::Element render_help_hint() const;
  ftxui::Element render_keybind_help() const;

  fl::context::AccountCtx ctx_;
  std::deque<fl::primitives::AccountData> *accounts_{nullptr};
  FancyLog *console_log_{nullptr};
  fl::primitives::WorldClock *world_clock_{nullptr};
  UiCommandController commands_;
  const fl::primitives::DiscoveryJournal *discoveries_;
  fl::primitives::ObjectiveSystem *objectives_{nullptr};
  std::optional<fl::monster::MonsterKind> return_monster_;
  ActiveScreen active_screen_kind_{ActiveScreen::account_battle};
  std::optional<ActiveScreen> story_return_screen_;
  std::optional<std::pair<entt::entity, std::uint64_t>> shown_raid_;
  bool keybind_help_open_{false};
  bool quit_confirmation_open_{false};
  bool quit_requested_{false};
  bool sandbox_confirmation_open_{false};
  bool autoplay_confirmation_open_{false};
  bool console_above_modals_{false};
  bool sandbox_requested_{false};
  bool help_hint_collapsed_{false};
  bool normal_combat_resumed_{false};
  bool fps_initialized_{false};
  int fps_frame_count_{0};
  int displayed_fps_{0};
  std::uint64_t render_frames_{0};
  std::chrono::steady_clock::time_point fps_window_start_{};
  ImportantNoticeOverlay important_notices_;
  ftxui::Box viewport_box_{};

  ftxui::Component quit_cancel_;
  ftxui::Component quit_confirmation_;
  ftxui::Component active_screen_;
  ftxui::Component console_overlay_;
  // Account/party buses outlive the root; disconnect before its state dies.
  std::vector<fl::events::ScopedPartyListener> combat_started_subs_;
  std::vector<fl::events::ScopedPartyListener> milestone_subs_;
  fl::events::ScopedAchievementListener achievement_sub_;
  fl::events::ScopedStoryListener objective_sub_;
  fl::events::ScopedStoryListener autoplay_sub_;
};

} // namespace fl::widgets
