#include <CLI/CLI.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include "fl/grand_central.hpp"
#include "fl/primitives/world_clock.hpp"

int main(int argc, char **argv) {
  using fl::GrandCentral;

  CLI::App app{"fairlanes"};

  fl::GrandCentralRunOptions opts;

  app.add_flag("--no-ui", opts.no_ui, "Run headless (no FTXUI screen)");

  app.add_option(
         "--overdrive",
         opts.overdrive,
         "Beat-rate multiplier 1-" +
             std::to_string(fl::primitives::WorldClock::kMaxBeatRateMultiplier))
      ->check(CLI::Range(1, fl::primitives::WorldClock::kMaxBeatRateMultiplier));

  app.add_option(
         "--cutoff-seconds",
         opts.cutoff_seconds,
         "Stop after this many simulated seconds")
      ->check(CLI::PositiveNumber);

  CLI11_PARSE(app, argc, argv);

  GrandCentral gc{8, 5, 5};
  gc.innervate_event_system();

  gc.main_loop(opts);

  return 0;
}
