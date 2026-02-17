#include "grand_central.hpp"

#include <deque>
#include <memory>
#include <mutex>

#include <entt/entt.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <tracy/Tracy.hpp>

// #include "fl/assert.hpp"
#include "fl/context.hpp"
#include "fl/ecs/components/is_account.hpp"
#include "fl/ecs/components/is_party.hpp"
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
#include "fl/widgets/log_wall.hpp"
#include "sr/beat_bus.hpp"
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
        "[yellow](Account " + std::to_string(a) + " initialized with ID " +
        std::to_string(static_cast<std::underlying_type_t<entt::entity>>(
            account_data.account_id())) +
        ")");
    auto entity = account_data.account_id();
    reg_.emplace<fl::ecs::components::IsAccount>(
        entity, entity, account_data.log(), account_data);

    for (std::size_t p = 0; p < num_parties_per_account_; ++p) {
      auto party_name = party_names.at(party_index);

      auto account_ctx = account_context(account_data);

      auto party_id = reg_.create();
      auto &party_data = account_data.parties().emplace_back(
          party_id, account_ctx, party_name);
      reg_.emplace<fl::ecs::components::IsParty>(party_id, party_id,
                                                 party_data);

      ++party_index;

      account_data.log().append_markup(
          "[green](Party " + party_name + " initialized with ID " +
          std::to_string(static_cast<std::underlying_type_t<entt::entity>>(
              party_data.party_id())) +
          ")");

      for (std::size_t m = 0; m < num_members_per_party_; ++m) {
        // 3) Create the member IN the party’s owning container first
        auto &member = party_data.members().emplace_back(
            reg_.create(), hero_names[player_index]);

        party_data.log().append_markup(
            "[blue](Player initialized with ID " +
            std::to_string(static_cast<std::underlying_type_t<entt::entity>>(
                member.member_id())) +
            ") as [player_name](" + hero_names[player_index] + ")");

        reg_.emplace<fl::ecs::components::Stats>(member.member_id(),
                                                 hero_names[player_index]);

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
      logger_{log_bus_}, fancy_log_(std::make_unique<fl::widgets::FancyLog>()),
      fancy_log_sink_(std::make_unique<fl::primitives::FancyLogSink>(
          log_bus_, *fancy_log_, fl::primitives::LogLevel::trace)) {
  fl::monster::register_all_monsters();
  _create_initial_accounts();
  bootstrap_logs();
  build_ui();
}

GrandCentral::~GrandCentral() { fancy_log_sink_.reset(); }

// Convenience accessor if you want the root FTXUI component:
ftxui::Component GrandCentral::root_component() { return log_wall_; }

fl::context::AccountCtx GrandCentral::account_context(std::size_t idx) {
  return fl::context::AccountCtx{reg_, rng_, accounts_.at(idx)};
}

fl::context::AccountCtx
GrandCentral::account_context(fl::primitives::AccountData &account) {
  return fl::context::AccountCtx{reg_, rng_, account};
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

    ZoneScopedN("Render");
    // tick_party_fsms(dt);
    std::scoped_lock lock(frame_mutex_);

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

  std::jthread render_ticker([&](std::stop_token st) {
    using namespace std::chrono_literals;
    while (!st.stop_requested() && running.load(std::memory_order_relaxed)) {
      screen.PostEvent(ftxui::Event::Custom);
      std::this_thread::sleep_for(16ms);
    }
  });

  std::jthread update_ticker([&](std::stop_token st) {
    using clock = std::chrono::steady_clock;

    constexpr auto kFrameDt = std::chrono::duration_cast<clock::duration>(
        std::chrono::duration<double>(1.0 / 12.0));

    // constexpr auto kOverrunBudget = std::chrono::milliseconds(5);

    auto next_tick = clock::now();
    auto sim_time = next_tick;

    while (!st.stop_requested() && running.load(std::memory_order_relaxed)) {

      ZoneScopedN("FrameTick");

      //      const auto now = clock::now();

      // ---- Budget enforcement ----
      // FIXME seems bad to not have it
      /*if (now > next_tick + kOverrunBudget) {
        const auto over = now - next_tick;
        const auto over_us =
            std::chrono::duration_cast<std::chrono::microseconds>(over).count();

        fl::assert_fmt(false, std::source_location::current(),
                       FMT_STRING("Frame tick over bdget by {} us"), over_us);
      }
                       */

      // ---- Advance authoritative sim time ----
      sim_time += kFrameDt;

      {
        std::scoped_lock lock(frame_mutex_);
        logger_.info("[player_name](Beat) event.");

        gc_beat_bus_.emit(seerin::Beat{});
      }

      // ---- Schedule next tick ----
      next_tick += kFrameDt;

      // ---- Pace ----
      std::this_thread::sleep_until(next_tick);
    }
  });
  screen.Loop(ui);
}

void GrandCentral::bootstrap_logs() {

  log_wall_ = ftxui::Make<fl::widgets::LogWall>(*fancy_log_, accounts_);

  logger_.info("[player_name](GrandCentral) online.");
  logger_.debug("Debug: registry currently empty.");
  logger_.warn("No encounters loaded yet.");
}

void GrandCentral::innervate_event_system() {
  // Parties hook to main beat (accounts do not).
  logger_.info("[spell_name](innervate) event system.");

  for (auto &account : accounts_) {
    for (auto &party : account.parties()) {
      party.hook_to_beat(gc_beat_bus_);
    }
  }
}

void GrandCentral::build_ui() {
  log_wall_ = ftxui::Make<fl::widgets::LogWall>(*fancy_log_, accounts_);
}

} // namespace fl
