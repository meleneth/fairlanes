#include "chaos_attract_grand_central.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include "fl/grand_central.hpp"
#include "fl/tracy_shim.hpp"
#include "fl/web_screen_bridge.hpp"
#include "fl/widgets/all_account_battle_screen.hpp"
#include "fl/widgets/chaos_attract_root.hpp"

namespace fl {

ChaosAttractGrandCentral::ChaosAttractGrandCentral()
    : attract_game_(std::make_unique<GrandCentral>(8, 5, 5)) {
  attract_game_->innervate_event_system();
  auto battle_surface = ftxui::Make<fl::widgets::AllAccountBattleScreen>(
      attract_game_->reg(), attract_game_->accounts());
  root_component_ = ftxui::Make<fl::widgets::ChaosAttractRoot>(
      &attract_game_->world_clock(), std::move(battle_surface));
}

ChaosAttractGrandCentral::~ChaosAttractGrandCentral() = default;

ChaosAttractExit
ChaosAttractGrandCentral::main_loop(ChaosAttractRunOptions opts) {
  ZoneScopedN("ChaosAttractGrandCentral::main_loop");

  using namespace ftxui;

  auto *root =
      dynamic_cast<fl::widgets::ChaosAttractRoot *>(root_component_.get());
  auto result = ChaosAttractExit::Quit;
  std::atomic<bool> running{true};

  ScreenInteractive screen = ScreenInteractive::Fullscreen();
  WebScreenRegistration web_screen_registration{screen};
  screen.SetCursor(Screen::Cursor{.shape = Screen::Cursor::Hidden});

  const auto started_at = std::chrono::steady_clock::now();
  auto cutoff_reached = [&] {
    if (!opts.cutoff_seconds) {
      return false;
    }

    const auto elapsed = std::chrono::steady_clock::now() - started_at;
    return std::chrono::duration<double>(elapsed).count() >=
           *opts.cutoff_seconds;
  };

  auto ui = Renderer(root_component_, [&] {
    ZoneScopedN("ChaosAttractRender");
    if (cutoff_reached()) {
      running.store(false, std::memory_order_relaxed);
      screen.Exit();
    }

    std::scoped_lock lock(attract_game_->frame_mutex());
    attract_game_->resolve_visuals_for_render();
    return root_component_->Render();
  });

  ui = CatchEvent(ui, [&](Event event) {
    if (event == Event::Character('q') || event == Event::Escape) {
      result = ChaosAttractExit::Quit;
      screen.Exit();
      return true;
    }

    if (event == Event::Return) {
      root->OnEvent(event);
      result = ChaosAttractExit::Begin;
      screen.Exit();
      return true;
    }

    return true;
  });

  std::jthread render_ticker([&](std::stop_token st) {
    using namespace std::chrono_literals;
    while (!st.stop_requested() && running.load(std::memory_order_relaxed)) {
      ZoneScopedN("ChaosAttractRenderTickerPost");
      screen.PostEvent(ftxui::Event::Custom);
      FrameMark;
      std::this_thread::sleep_for(33ms);
    }
  });

  std::jthread update_ticker([&](std::stop_token st) {
    using clock = std::chrono::steady_clock;
    auto next_tick = clock::now();

    while (!st.stop_requested() && running.load(std::memory_order_relaxed)) {
      ZoneScopedN("ChaosAttractFrameTick");
      if (cutoff_reached()) {
        running.store(false, std::memory_order_relaxed);
        screen.Exit();
        break;
      }

      const auto frame_dt = std::chrono::duration_cast<clock::duration>(
          std::chrono::duration<double>(
              1.0 /
              attract_game_->world_clock().effective_beats_per_wall_second()));

      attract_game_->advance_beat();

      next_tick += frame_dt;
      std::this_thread::sleep_until(next_tick);
      FrameMark;
    }
  });

  screen.Loop(ui);
  running.store(false, std::memory_order_relaxed);

  return result;
}

} // namespace fl
