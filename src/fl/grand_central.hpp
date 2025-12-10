#pragma once
// INTERNAL: Do not include from engine code.

#include <deque>
#include <memory>
#include <mutex>

#include <entt/entt.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <tracy/Tracy.hpp>

#include "fl/context.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/fancy_log_sink.hpp"
#include "fl/primitives/hero_names.hpp"
#include "fl/primitives/logging.hpp"
#include "fl/primitives/member_data.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/random_hub.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl {

class GrandCentral {
public:
  uint8_t num_accounts_{8};
  uint8_t num_parties_per_account_{5};
  entt::registry reg_;
  fl::primitives::RandomHub rng_;

  fl::primitives::LogBus log_bus_;
  fl::primitives::Logger logger_;

  std::mutex frame_mutex;

  std::vector<fl::primitives::AccountData> accounts_;

  // UI pieces
  std::shared_ptr<fl::widgets::FancyLog> fancy_log_;
  std::unique_ptr<fl::primitives::FancyLogSink> fancy_log_sink_;

  void _create_initial_accounts() {
    int player_index = 0;
    for (std::size_t a = 0; a < num_accounts_; ++a) {
      fl::primitives::AccountData account{reg_.create()};
      logger_.info(
          "[yellow](Account initialized with ID " +
          std::to_string(static_cast<std::underlying_type_t<entt::entity>>(
              account.account_id_)) +
          ")");
      for (std::size_t p = 0; p < num_parties_per_account_; ++p) {
        fl::primitives::PartyData party{reg_.create()};

        logger_.info(
            "[green](Party initialized with ID " +
            std::to_string(static_cast<std::underlying_type_t<entt::entity>>(
                party.party_id_)) +
            ")");

        fl::primitives::MemberData member{reg_.create(),
                                          hero_names[player_index]};
        player_index++;
        logger_.info(
            "[blue](Player initialized with ID " +
            std::to_string(static_cast<std::underlying_type_t<entt::entity>>(
                member.member_id_)) +
            ") as [player_name](" + hero_names[player_index - 1] + ")");

        account.parties_.push_back(std::move(party));
      }

      accounts_.push_back(std::move(account));
    }
  }

  GrandCentral(uint8_t num_accounts, uint8_t num_parties_per_account)
      : num_accounts_(num_accounts),
        num_parties_per_account_(num_parties_per_account), reg_(), rng_(),
        log_bus_(), logger_{log_bus_},
        fancy_log_(std::make_shared<fl::widgets::FancyLog>()),
        fancy_log_sink_(std::make_unique<fl::primitives::FancyLogSink>(
            log_bus_, *fancy_log_, fl::primitives::LogLevel::trace)) {
    _create_initial_accounts();
  }

  // Convenience accessor if you want the root FTXUI component:
  ftxui::Component root_component() { return fancy_log_; }

  fl::context::AccountCtx account_context(std::size_t idx) {
    return fl::context::AccountCtx{reg_, rng_, accounts_.at(idx)};
  }

  // Little helper to show logs are working
  void bootstrap_logs() {
    logger_.info("[player_name](GrandCentral) online.");
    logger_.debug("Debug: registry currently empty.");
    logger_.warn("No encounters loaded yet.");
  }

  void main_loop() {
    using namespace ftxui;

    ScreenInteractive screen = ScreenInteractive::Fullscreen();
    screen.SetCursor(Screen::Cursor{.shape = Screen::Cursor::Hidden});

    auto ui = Renderer((ftxui::Component)root_component(), [&] {
      using clock = std::chrono::steady_clock;
      static auto last = clock::now();
      const auto now = clock::now();
      // float dt = std::chrono::duration<float>(now - last).count();
      last = now;

      // tick_party_fsms(dt);

      return root_component()->Render();
    });

    ui = CatchEvent(ui, [&](Event e) {
      if (e == Event::Character('q') || e == Event::Escape) {
        screen.Exit();
        return true;
      }
      return false;
    });

    // wake UI at ~60Hz
    std::atomic<bool> running = true;

    std::thread render_ticker([&] {
      using namespace std::chrono_literals;
      while (running) {
        {
          ZoneScopedN("Render");
          std::scoped_lock lock(frame_mutex);
          screen.PostEvent(Event::Custom); // kick a rerender (~60 Hz)
        }

        std::this_thread::sleep_for(16ms);
      }
    });

    std::thread update_ticker([&] {
      using namespace std::chrono_literals;
      while (running) {
        {
          ZoneScopedN("GameTick");
          std::scoped_lock lock(frame_mutex);
        }

        // TODO 16
        // ZoneScopedN("GameTick");
        //      std::this_thread::sleep_for(16ms);

        std::this_thread::sleep_for(250ms);
      }
    });

    screen.Loop(ui);
    running = false;
    render_ticker.join();
    update_ticker.join();
  }
};

} // namespace fl
