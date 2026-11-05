#pragma once
#include "fl/primitives/raid_milestones.hpp"
// INTERNAL: Do not include from engine code.

#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>

#include <entt/entity/registry.hpp>
#include <ftxui/component/component.hpp>

#include "fl/context.hpp"
#include "fl/fwd.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/discovery_journal.hpp"
#include "fl/primitives/logging.hpp"
#include "fl/primitives/objective_system.hpp"
#include "fl/primitives/random_hub.hpp"
#include "fl/primitives/tower_balance.hpp"
#include "fl/primitives/world_clock.hpp"
#include "sr/beat_bus.hpp"

namespace fl {

struct GrandCentralRunOptions {
  bool no_ui{false};
  int overdrive{1};
  std::optional<double> cutoff_seconds;
  // Return false to freeze gameplay beats while leaving the client view open.
  std::function<bool()> after_beat;
};

class GrandCentral {
public:
  GrandCentral(uint8_t num_accounts, uint8_t num_parties_per_account, uint8_t num_members_per_party,
               bool record_discoveries = true, bool record_progress = true,
               std::optional<std::uint64_t> gameplay_seed = std::nullopt, bool story_mode = false,
               bool story_autoplay = false, fl::primitives::TowerBalance tower_balance = {});
  ~GrandCentral();

  fl::primitives::RaidMilestones& raid_milestones() {
    return raid_milestones_;
  }
  fl::primitives::ObjectiveSystem* objectives() {
    return objectives_.get();
  }
  bool sandbox_requested() const;

  // ---- accessors (refs only, no ownership leaks) ----
  uint8_t num_accounts() const noexcept {
    return num_accounts_;
  }
  uint8_t num_parties_per_account() const noexcept {
    return num_parties_per_account_;
  }
  uint8_t num_members_per_party() const noexcept {
    return num_members_per_party_;
  }

  entt::registry& reg() noexcept {
    return reg_;
  }
  const fl::primitives::DiscoveryJournal& discoveries() const noexcept {
    return accounts_.front().discoveries();
  }
  const entt::registry& reg() const noexcept {
    return reg_;
  }

  fl::primitives::RandomHub& rng() noexcept {
    return rng_;
  }
  const fl::primitives::RandomHub& rng() const noexcept {
    return rng_;
  }

  fl::primitives::LogBus& log_bus() noexcept {
    return log_bus_;
  }
  fl::primitives::Logger& logger() noexcept {
    return logger_;
  }
  fl::primitives::WorldClock& world_clock() noexcept {
    return world_clock_;
  }
  const fl::primitives::WorldClock& world_clock() const noexcept {
    return world_clock_;
  }

  std::mutex& frame_mutex() noexcept {
    return frame_mutex_;
  }

  std::deque<fl::primitives::AccountData>& accounts() noexcept {
    return accounts_;
  }
  const std::deque<fl::primitives::AccountData>& accounts() const noexcept {
    return accounts_;
  }

  fl::widgets::FancyLog& game_log() noexcept {
    return *game_log_;
  }
  fl::primitives::FancyLogSink& fancy_log_sink() noexcept {
    return *fancy_log_sink_;
  }

  seerin::BeatBus& beat_bus() noexcept {
    return gc_beat_bus_;
  }

  ftxui::Component& log_wall() noexcept {
    return log_wall_;
  }

  // ---- behavior ----
  fl::context::AccountCtx account_context(std::size_t idx);
  fl::context::AccountCtx account_context(fl::primitives::AccountData& account);

  ftxui::Component root_component();
  void innervate_event_system(bool enable_visitors = true);
  void advance_beat();
  void resolve_visuals_for_render();
  void main_loop(GrandCentralRunOptions opts = {});

private:
  // ---- owned state ----
  bool record_progress_{true};
  bool story_mode_{false};
  bool story_autoplay_{false};
  fl::primitives::TowerBalance tower_balance_{};
  uint8_t num_accounts_{8};
  uint8_t num_parties_per_account_{5};
  uint8_t num_members_per_party_{5};

  entt::registry reg_;
  fl::primitives::RandomHub rng_;
  fl::primitives::LogBus log_bus_;
  fl::primitives::Logger logger_;
  fl::primitives::WorldClock world_clock_;
  std::mutex frame_mutex_;
  seerin::BeatBus gc_beat_bus_;
  seerin::BeatSub story_autoplay_sub_{};

  std::deque<fl::primitives::AccountData> accounts_;
  fl::primitives::RaidMilestones raid_milestones_;
  std::unique_ptr<fl::primitives::ObjectiveSystem> objectives_;
  std::vector<fl::events::ScopedRaidListener> raid_log_subscriptions_;

  std::unique_ptr<fl::widgets::FancyLog> game_log_;
  std::unique_ptr<fl::primitives::FancyLogSink> fancy_log_sink_;
  ftxui::Component log_wall_;
  ftxui::Component account_battle_view_;
  ftxui::Component root_component_;

  // ---- internal helpers ----
  void _create_initial_accounts();
  void bootstrap_logs();
  void build_ui();
};

} // namespace fl
