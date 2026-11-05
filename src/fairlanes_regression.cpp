#include <CLI/CLI.hpp>

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

#include <nlohmann/json.hpp>

#include "fl/generated/regression_content.hpp"
#include "fl/primitives/world_clock.hpp"
#include "fl/regression/session.hpp"

int main(int argc, char** argv) {
  CLI::App app{"Fairlanes seeded combat regression runner"};
  fl::regression::Options options;
  std::string output_path;
  std::string output_dir;
  bool assert_expected = false;
  bool all_seeds = false;
  app.add_option("--fixture", options.fixture, "Fixture ID")->required();
  auto* seed_option = app.add_option("--seed", options.seed, "Gameplay RNG seed");
  app.add_option("--record", output_path, "Recording JSON output path");
  app.add_option("--record-dir", output_dir, "Batch recording directory");
  app.add_option("--variant", options.variant, "Generated regression variant ID");
  app.add_flag("--all-seeds", all_seeds, "Run the declaration's full seed set");
  auto* beat_cap_option = app.add_option("--beat-cap", options.beat_cap, "Maximum simulated beats");
  app.add_option("--armor", options.hero_armor, "Player's initial flat Armor");
  app.add_option("--tower-max-level", options.tower_balance.maximum_monster_level,
                 "Monster level represented by 100% tower scaling")
      ->check(CLI::PositiveNumber);
  app.add_option("--tower-max-hp", options.tower_balance.maximum_monster_hp,
                 "Monster HP represented by 100% tower scaling")
      ->check(CLI::PositiveNumber);
  app.add_option("--tower-max-attack", options.tower_balance.maximum_monster_attack,
                 "Monster Attack represented by 100% tower scaling")
      ->check(CLI::PositiveNumber);
  app.add_flag("--assert-expected", assert_expected,
               "Fail unless the fixture reaches its expected outcome");
  CLI11_PARSE(app, argc, argv);

  try {
    if (const auto* scenario = fl::regression::generated_content::find(options.fixture)) {
      if (beat_cap_option->count() == 0) options.beat_cap = scenario->beat_cap;
    }
    if (all_seeds) {
      const auto* scenario = fl::regression::generated_content::find(options.fixture);
      if (!scenario || output_dir.empty() || options.variant.empty())
        throw std::invalid_argument(
            "--all-seeds needs a generated fixture, --variant and --record-dir");
      const auto* variant =
          fl::regression::generated_content::find_variant(*scenario, options.variant);
      if (!variant) throw std::invalid_argument("unknown regression variant");
      std::filesystem::create_directories(output_dir);
      int wins = 0;
      int caps = 0;
      for (const auto seed : scenario->seeds) {
        options.seed = seed;
        fl::regression::Session session{options};
        while (!session.finished()) session.advance_beat();
        const auto path =
            (std::filesystem::path(output_dir) /
             (options.fixture + "." + options.variant + ".seed" + std::to_string(seed) + ".json"))
                .string();
        session.write(path);
        wins += session.outcome() == "hero_victory" ? 1 : 0;
        caps += session.outcome() == "beat_cap" ? 1 : 0;
        std::cout << "seed=" << seed << " outcome=" << session.outcome()
                  << " beats=" << session.beats()
                  << " remaining_hp=" << session.recording().at("result").at("remaining_hp").dump()
                  << " recording=" << path << '\n';
      }
      std::cout << "fixture=" << options.fixture << " variant=" << options.variant
                << " wins=" << wins << "/" << scenario->seeds.size() << " caps=" << caps << '\n';
      if (assert_expected && (!variant->asserted || caps > 0 || wins < variant->min_wins ||
                              wins > variant->max_wins)) {
        std::cerr << "win-count expectation missed or unexpected beat cap\n";
        return 2;
      }
      return 0;
    }
    if (seed_option->count() == 0 || output_path.empty())
      throw std::invalid_argument("single run needs --seed and --record");
    fl::regression::Session session{options};
    while (!session.finished()) session.advance_beat();
    session.write(output_path);
    std::cout << "fixture=" << options.fixture << " seed=" << options.seed
              << " outcome=" << session.outcome() << " beats=" << session.beats()
              << " uwu=" << session.uwu()
              << " remaining_hp=" << session.recording().at("result").at("remaining_hp").dump()
              << " simulated_seconds="
              << static_cast<double>(session.beats()) / fl::primitives::WorldClock::kBeatsPerSecond
              << " recording=" << output_path << '\n';
    if (assert_expected && options.fixture != fl::regression::kSmokeFixture) {
      std::cerr << "use --all-seeds for generated win-count assertions\n";
      return 2;
    }
    if (assert_expected && session.outcome() != "hero_victory") {
      std::cerr << "expected hero_victory; got " << session.outcome() << '\n';
      return 2;
    }
  } catch (const std::exception& ex) {
    std::cerr << "fairlanes_regression: " << ex.what() << '\n';
    return 1;
  }
  return 0;
}
