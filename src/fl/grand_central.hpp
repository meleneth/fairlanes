#pragma once
// INTERNAL: Do not include from engine code.

#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>

#include <entt/entt.hpp>
#include <ftxui/component/component.hpp>

#include "fl/context.hpp"
#include "fl/fwd.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/logging.hpp"
#include "fl/primitives/random_hub.hpp"
#include "sr/beat_bus.hpp"

namespace fl {

class GrandCentral {
public:
  GrandCentral(uint8_t num_accounts, uint8_t num_parties_per_account,
               uint8_t num_members_per_party);
  ~GrandCentral();

  // ---- accessors (refs only, no ownership leaks) ----
  uint8_t num_accounts() const noexcept { return num_accounts_; }
  uint8_t num_parties_per_account() const noexcept {
    return num_parties_per_account_;
  }
  uint8_t num_members_per_party() const noexcept {
    return num_members_per_party_;
  }

  entt::registry &reg() noexcept { return reg_; }
  const entt::registry &reg() const noexcept { return reg_; }

  fl::primitives::RandomHub &rng() noexcept { return rng_; }
  const fl::primitives::RandomHub &rng() const noexcept { return rng_; }

  fl::primitives::LogBus &log_bus() noexcept { return log_bus_; }
  fl::primitives::Logger &logger() noexcept { return logger_; }

  std::mutex &frame_mutex() noexcept { return frame_mutex_; }

  std::deque<fl::primitives::AccountData> &accounts() noexcept {
    return accounts_;
  }
  const std::deque<fl::primitives::AccountData> &accounts() const noexcept {
    return accounts_;
  }

  fl::widgets::FancyLog &fancy_log() noexcept { return *fancy_log_; }
  fl::primitives::FancyLogSink &fancy_log_sink() noexcept {
    return *fancy_log_sink_;
  }

  seerin::BeatBus &beat_bus() noexcept { return gc_beat_bus_; }

  ftxui::Component &log_wall() noexcept { return log_wall_; }

  // ---- behavior ----
  fl::context::AccountCtx account_context(std::size_t idx);
  fl::context::AccountCtx account_context(fl::primitives::AccountData &account);

  ftxui::Component root_component();
  void innervate_event_system();
  void main_loop();

private:
  // ---- owned state ----
  uint8_t num_accounts_{8};
  uint8_t num_parties_per_account_{5};
  uint8_t num_members_per_party_{5};

  entt::registry reg_;
  fl::primitives::RandomHub rng_;
  fl::primitives::LogBus log_bus_;
  fl::primitives::Logger logger_;
  std::mutex frame_mutex_;

  std::deque<fl::primitives::AccountData> accounts_;

  std::unique_ptr<fl::widgets::FancyLog> fancy_log_;
  std::unique_ptr<fl::primitives::FancyLogSink> fancy_log_sink_;

  seerin::BeatBus gc_beat_bus_;
  ftxui::Component log_wall_;
  ftxui::Component account_battle_view_;

  // ---- internal helpers ----
  void _create_initial_accounts();
  void bootstrap_logs();
  void build_ui();
};

} // namespace fl
