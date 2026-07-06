#include <CLI/CLI.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include <exception>
#include <iostream>

#include "fl/chaos_attract_grand_central.hpp"
#include "fl/grand_central.hpp"
#include "fl/primitives/world_clock.hpp"

int main(int argc, char **argv) {
  using fl::GrandCentral;

  CLI::App app{"fairlanes"};

  fl::GrandCentralRunOptions opts;
  bool skip_chaos_attract = false;

  app.add_flag("--no-ui", opts.no_ui, "Run headless (no FTXUI screen)");
  app.add_flag("--skip-chaos-attract", skip_chaos_attract,
               "Start normal gameplay without the Chaos Attract screen");

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
    if (!opts.no_ui && !skip_chaos_attract) {
      fl::ChaosAttractGrandCentral attract;
      const auto attract_exit = attract.main_loop(
          fl::ChaosAttractRunOptions{.cutoff_seconds = opts.cutoff_seconds});
      if (attract_exit != fl::ChaosAttractExit::Begin) {
        return 0;
      }
    }

    GrandCentral gc{8, 5, 5};
    gc.innervate_event_system();

    gc.main_loop(opts);
  } catch (const std::exception &ex) {
    std::cerr << "fairlanes: " << ex.what() << '\n';
    return 1;
  } catch (...) {
    std::cerr << "fairlanes: unknown fatal error\n";
    return 1;
  }

  return 0;
}
