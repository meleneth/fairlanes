#include <CLI/CLI.hpp>

#include <exception>
#include <iostream>
#include <string>

#include "fl/primitives/world_clock.hpp"
#include "fl/regression/session.hpp"

int main(int argc, char **argv) {
  CLI::App app{"Fairlanes seeded combat regression runner"};
  fl::regression::Options options;
  std::string output_path;
  bool assert_expected = false;
  app.add_option("--fixture", options.fixture, "Fixture ID")->required();
  app.add_option("--seed", options.seed, "Gameplay RNG seed")->required();
  app.add_option("--record", output_path, "Recording JSON output path")
      ->required();
  app.add_option("--beat-cap", options.beat_cap, "Maximum simulated beats");
  app.add_flag("--assert-expected", assert_expected,
               "Fail unless the fixture reaches its expected outcome");
  CLI11_PARSE(app, argc, argv);

  try {
    fl::regression::Session session{options};
    while (!session.finished())
      session.advance_beat();
    session.write(output_path);
    std::cout << "fixture=" << options.fixture << " seed=" << options.seed
              << " outcome=" << session.outcome()
              << " beats=" << session.beats() << " uwu=" << session.uwu()
              << " simulated_seconds="
              << static_cast<double>(session.beats()) /
                     fl::primitives::WorldClock::kBeatsPerSecond
              << " recording=" << output_path << '\n';
    if (assert_expected && session.outcome() != "hero_victory") {
      std::cerr << "expected hero_victory; got " << session.outcome() << '\n';
      return 2;
    }
  } catch (const std::exception &ex) {
    std::cerr << "fairlanes_regression: " << ex.what() << '\n';
    return 1;
  }
  return 0;
}
