#include <CLI/CLI.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <nlohmann/json.hpp>

#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

#include "fl/chaos_attract_grand_central.hpp"
#include "fl/grand_central.hpp"
#include "fl/primitives/world_clock.hpp"
#include "fl/regression/session.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "fl/widgets/story_start_picker.hpp"

int main(int argc, char **argv) {
  using fl::GrandCentral;

  CLI::App app{"fairlanes"};

  fl::GrandCentralRunOptions opts;
  bool skip_chaos_attract = false;
  bool story_autoplay = false;
  std::string replay_path;

  app.add_flag("--no-ui", opts.no_ui, "Run headless (no FTXUI screen)");
  app.add_flag("--skip-chaos-attract", skip_chaos_attract,
               "Start normal gameplay without the Chaos Attract screen");
  app.add_flag("--story-autoplay", story_autoplay,
               "Follow the Ruby-authored opening story route automatically");
  app.add_option("--replay", replay_path,
                 "Open and verify a combat recording in the client view");

  app.add_option(
         "--overdrive", opts.overdrive,
         "Beat-rate multiplier 1-" +
             std::to_string(fl::primitives::WorldClock::kMaxBeatRateMultiplier))
      ->check(
          CLI::Range(1, fl::primitives::WorldClock::kMaxBeatRateMultiplier));

  app.add_option("--cutoff-seconds", opts.cutoff_seconds,
                 "Stop after this many simulated seconds")
      ->check(CLI::PositiveNumber);

  CLI11_PARSE(app, argc, argv);

  try {
    if (!replay_path.empty()) {
      const auto expected = fl::regression::read_recording(replay_path);
      fl::regression::Session replay{
          fl::regression::options_from_recording(expected)};
      if (auto divergence = replay.first_divergence(expected)) {
        throw std::runtime_error(*divergence);
      }
      std::optional<std::string> divergence;
      auto after_beat = [&] {
        replay.observe_beat();
        divergence = replay.first_divergence(expected);
        if (divergence) {
          replay.game().accounts().front().parties().front().log().append_plain(
              "Replay divergence: " + *divergence);
          return false;
        }
        if (replay.finished()) {
          replay.game().accounts().front().parties().front().log().append_plain(
              "Replay verified: " + std::string(replay.outcome()));
          return false;
        }
        return true;
      };
      replay.game().world_clock().set_beat_rate_multiplier(opts.overdrive);
      if (opts.no_ui) {
        while (!replay.finished()) {
          replay.advance_beat();
          divergence = replay.first_divergence(expected);
          if (divergence) break;
        }
      } else {
        opts.after_beat = after_beat;
        replay.game().main_loop(opts);
      }
      if (divergence) {
        std::cerr << "replay divergence: " << *divergence << '\n';
        return 2;
      }
      if (!replay.finished()) {
        std::cerr << "replay interrupted before terminal result\n";
        return 3;
      }
      std::cout << "replay verified: outcome=" << replay.outcome()
                << " beats=" << replay.beats() << " uwu=" << replay.uwu()
                << " path=" << replay_path << '\n';
      return 0;
    }

    if (!opts.no_ui && !skip_chaos_attract) {
      fl::ChaosAttractGrandCentral attract;
      const auto attract_exit = attract.main_loop(
          fl::ChaosAttractRunOptions{.cutoff_seconds = opts.cutoff_seconds});
      if (attract_exit != fl::ChaosAttractExit::Begin) {
        return 0;
      }
      const auto story_mode = fl::widgets::choose_story_start_mode();
      if (story_mode == fl::widgets::StoryStartMode::Cancel) return 0;
      story_autoplay =
          story_mode == fl::widgets::StoryStartMode::Autoplay;
    }

    GrandCentral gc{1, 1, 0, true, true, std::nullopt, true, story_autoplay};
    gc.innervate_event_system();
    gc.main_loop(opts);
    if (gc.sandbox_requested()) {
      GrandCentral sandbox{8, 5, 5};
      sandbox.innervate_event_system();
      sandbox.main_loop(opts);
    }
  } catch (const std::exception &ex) {
    std::cerr << "fairlanes: " << ex.what() << '\n';
    return 1;
  } catch (...) {
    std::cerr << "fairlanes: unknown fatal error\n";
    return 1;
  }

  return 0;
}
