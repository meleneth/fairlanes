#pragma once

#include <chrono>
#include <cstddef>
#include <deque>
#include <optional>
#include <string_view>
#include <vector>

#include <ftxui/component/component.hpp>

#include "fl/context.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/events/raid_bus.hpp"
#include "fl/fwd.hpp"
#include "fl/generated/ftxer_composition_root.hpp"
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

// Owns Fairlanes presentation objects and gameplay-facing UI behavior. The
// generated CompositionRoot is the sole owner of navigation and overlay state.
class RootComponent {
public:
  RootComponent(fl::context::AccountCtx ctx, std::deque<fl::primitives::AccountData>& accounts,
                FancyLog& console_log, fl::primitives::WorldClock& world_clock,
                const fl::primitives::DiscoveryJournal* discoveries = nullptr,
                fl::events::AchievementBus* achievements = nullptr,
                fl::primitives::ObjectiveSystem* objectives = nullptr);

  void bind_shell(fl::generated::ftxer::ShellCallbacks callbacks);
  fl::generated::ftxer::ScreenId initial_screen(fl::generated::ftxer::ScreenId declared) const;

  ftxui::Component make_raid();
  ftxui::Component make_party();
  ftxui::Component make_account_battle();
  ftxui::Component make_party_battle();
  ftxui::Component make_effect_gallery();
  ftxui::Component make_libram();
  ftxui::Component make_bestiary();
  ftxui::Component make_objectives();
  ftxui::Component make_village();

  ftxui::Component make_console_overlay();
  ftxui::Component make_quit_confirmation_overlay();
  ftxui::Component make_autoplay_confirmation_overlay();
  ftxui::Component make_sandbox_confirmation_overlay();
  ftxui::Component make_keybind_help_overlay();

  ftxui::Element decorate(fl::generated::ftxer::ScreenId screen, ftxui::Element content);
  bool intercept(const ftxui::Event& event);
  void overlay_opened(fl::generated::ftxer::OverlayId overlay);
  void overlay_closed(fl::generated::ftxer::OverlayId overlay);

  bool toggle_autoplay(fl::generated::ftxer::ScreenId);
  bool toggle_help(fl::generated::ftxer::ScreenId);
  bool return_to_town(fl::generated::ftxer::ScreenId screen);
  bool collapse_help(fl::generated::ftxer::ScreenId);
  bool expand_help(fl::generated::ftxer::ScreenId);
  bool overdrive_up(fl::generated::ftxer::ScreenId);
  bool overdrive_down(fl::generated::ftxer::ScreenId);
  bool previous_party(fl::generated::ftxer::ScreenId);
  bool next_party(fl::generated::ftxer::ScreenId);
  bool previous_account(fl::generated::ftxer::ScreenId);
  bool next_account(fl::generated::ftxer::ScreenId);

  bool quit_requested() const {
    return quit_requested_;
  }
  bool sandbox_requested() const {
    return sandbox_requested_;
  }
  std::size_t console_history_size() const;
  std::string_view console_input_line() const;
  void set_full_open();

private:
  ConsoleOverlay* console_overlay();
  const ConsoleOverlay* console_overlay() const;
  fl::context::AccountCtx make_context(std::size_t account_index);
  bool raid_locked() const noexcept;
  bool select_account(int delta);
  bool select_party(int delta);
  ftxui::Component make_story_screen(bool village);
  void on_party_combat_started(entt::entity account);
  void update_fps_counter();
  ftxui::Element render_help_hint() const;
  ftxui::Element render_keybind_help() const;

  fl::context::AccountCtx ctx_;
  std::deque<fl::primitives::AccountData>* accounts_{nullptr};
  FancyLog* console_log_{nullptr};
  fl::primitives::WorldClock* world_clock_{nullptr};
  UiCommandController commands_;
  const fl::primitives::DiscoveryJournal* discoveries_{nullptr};
  fl::primitives::ObjectiveSystem* objectives_{nullptr};
  fl::generated::ftxer::ShellCallbacks shell_;
  std::optional<fl::monster::MonsterKind> return_monster_;
  std::optional<fl::skills::SkillId> selected_skill_;
  bool quit_requested_{false};
  bool sandbox_requested_{false};
  bool help_hint_collapsed_{false};
  bool fps_initialized_{false};
  int fps_frame_count_{0};
  int displayed_fps_{0};
  std::uint64_t render_frames_{0};
  std::chrono::steady_clock::time_point fps_window_start_{};
  ImportantNoticeOverlay important_notices_;
  ftxui::Box viewport_box_{};

  ftxui::Component console_overlay_;
  ftxui::Component quit_cancel_;
  ftxui::Component quit_confirmation_;
  ftxui::Component autoplay_confirmation_;
  ftxui::Component sandbox_confirmation_;
  ftxui::Component keybind_help_;
  std::vector<fl::events::ScopedPartyListener> combat_started_subs_;
  std::vector<fl::events::ScopedPartyListener> milestone_subs_;
  std::vector<fl::events::ScopedRaidListener> raid_started_subs_;
  fl::events::ScopedAchievementListener achievement_sub_;
  fl::events::ScopedStoryListener objective_sub_;
  fl::events::ScopedStoryListener autoplay_sub_;
};

} // namespace fl::widgets
