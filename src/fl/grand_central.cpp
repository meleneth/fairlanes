#include "grand_central.hpp"

#include <deque>
#include <memory>
#include <mutex>

#include <entt/entt.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <tracy/Tracy.hpp>

#include "fl/context.hpp"
#include "fl/monsters/register_monsters.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/fancy_log_sink.hpp"
#include "fl/primitives/hero_names.hpp"
#include "fl/primitives/logging.hpp"
#include "fl/primitives/member_data.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/party_names.hpp"
#include "fl/primitives/random_hub.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl {

void GrandCentral::_create_initial_accounts() {
  int party_index = 0;
  int player_index = 0;

  // The Pattern
  // create ID
  // pass to Data ctor
  // Data ctor takes a context?
  // Data can upgrade your context -
  // account_data.account_context(WorldCoreCtx &ctx)
  // party_data.party_context(AccountCtx &ctx);
  // party_ctx.party_loop_ctx();

  for (std::size_t a = 0; a < num_accounts_; ++a) {
    auto &account_data = accounts_.emplace_back(reg_.create());

    logger_.info(
        "[yellow](Account initialized with ID " +
        std::to_string(static_cast<std::underlying_type_t<entt::entity>>(
            account_data.account_id_)) +
        ")");

    for (std::size_t p = 0; p < num_parties_per_account_; ++p) {

      auto &party_data = account_data.parties_.emplace_back(reg_.create());
      auto account_ctx = account_context(account_data);
      auto party_ctx = account_ctx.party_context(party_data);
      auto party_loop_ctx = party_ctx.party_loop_context();
      party_data.init_party(party_loop_ctx, party_names[party_index]);

      ++party_index;

      logger_.info(
          "[green](Party initialized with ID " +
          std::to_string(static_cast<std::underlying_type_t<entt::entity>>(
              party_data.party_id_)) +
          ")");

      for (std::size_t m = 0; m < num_members_per_party_; ++m) {
        // 3) Create the member IN the party’s owning container first
        auto &member = party_data.members_.emplace_back(
            reg_.create(), hero_names[player_index]);

        logger_.info(
            "[blue](Player initialized with ID " +
            std::to_string(static_cast<std::underlying_type_t<entt::entity>>(
                member.member_id_)) +
            ") as [player_name](" + hero_names[player_index] + ")");

        ++player_index;
      }
    }
  }
}

GrandCentral::GrandCentral(uint8_t num_accounts,
                           uint8_t num_parties_per_account,
                           uint8_t num_members_per_party)
    : num_accounts_(num_accounts),
      num_parties_per_account_(num_parties_per_account),
      num_members_per_party_(num_members_per_party), reg_(), rng_(), log_bus_(),
      logger_{log_bus_}, fancy_log_(std::make_shared<fl::widgets::FancyLog>()),
      fancy_log_sink_(std::make_unique<fl::primitives::FancyLogSink>(
          log_bus_, *fancy_log_, fl::primitives::LogLevel::trace)) {
  fl::monster::register_all_monsters();
  _create_initial_accounts();
}

// Convenience accessor if you want the root FTXUI component:
ftxui::Component GrandCentral::root_component() { return fancy_log_; }

fl::context::AccountCtx GrandCentral::account_context(std::size_t idx) {
  return fl::context::AccountCtx{reg_, rng_, accounts_.at(idx)};
}

fl::context::AccountCtx
GrandCentral::account_context(fl::primitives::AccountData &account) {
  return fl::context::AccountCtx{reg_, rng_, account};
}
// Little helper to show logs are working
void GrandCentral::bootstrap_logs() {
  logger_.info("[player_name](GrandCentral) online.");
  logger_.debug("Debug: registry currently empty.");
  logger_.warn("No encounters loaded yet.");
}

void GrandCentral::main_loop() {
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

} // namespace fl
