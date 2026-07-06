#include <array>
#include <cstdlib>
#include <optional>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "fl/grand_central.hpp"
#include "fl/primitives/random_hub.hpp"

namespace {

class ScopedEnvVar {
public:
  explicit ScopedEnvVar(const char *name) : name_(name) {
    if (const char *value = std::getenv(name_)) {
      previous_ = std::string(value);
    }
  }

  ScopedEnvVar(const ScopedEnvVar &) = delete;
  ScopedEnvVar &operator=(const ScopedEnvVar &) = delete;

  ~ScopedEnvVar() {
#ifdef _WIN32
    if (previous_) {
      _putenv_s(name_, previous_->c_str());
    } else {
      _putenv_s(name_, "");
    }
#else
    if (previous_) {
      setenv(name_, previous_->c_str(), 1);
    } else {
      unsetenv(name_);
    }
#endif
  }

  void set(const char *value) {
#ifdef _WIN32
    _putenv_s(name_, value);
#else
    setenv(name_, value, 1);
#endif
  }

  void unset() {
#ifdef _WIN32
    _putenv_s(name_, "");
#else
    unsetenv(name_);
#endif
  }

private:
  const char *name_;
  std::optional<std::string> previous_;
};

} // namespace

TEST_CASE("RandomHub parses unsigned seed text", "[random][seed]") {
  REQUIRE(fl::primitives::parse_unsigned_seed("0") == 0ULL);
  REQUIRE(fl::primitives::parse_unsigned_seed("42") == 42ULL);
  REQUIRE(fl::primitives::parse_unsigned_seed("18446744073709551615") ==
          UINT64_MAX);

  CHECK_FALSE(fl::primitives::parse_unsigned_seed(""));
  CHECK_FALSE(fl::primitives::parse_unsigned_seed("banana"));
  CHECK_FALSE(fl::primitives::parse_unsigned_seed("12banana"));
  CHECK_FALSE(fl::primitives::parse_unsigned_seed("-1"));
  CHECK_FALSE(fl::primitives::parse_unsigned_seed("18446744073709551616"));
}

TEST_CASE("RandomHub uses FAIRLANES_RNG_SEED when present", "[random][seed]") {
  ScopedEnvVar env{"FAIRLANES_RNG_SEED"};
  env.set("8675309");

  fl::primitives::RandomHub hub;

  REQUIRE(hub.chosen_seed() == 8675309ULL);
  REQUIRE(hub.master_seed() == 8675309ULL);
  REQUIRE(hub.seed_source() == fl::primitives::RandomSeedSource::Environment);
}

TEST_CASE("RandomHub generated seed remains inspectable when env is unset",
          "[random][seed]") {
  ScopedEnvVar env{"FAIRLANES_RNG_SEED"};
  env.unset();

  fl::primitives::RandomHub hub;

  REQUIRE(hub.chosen_seed() == hub.master_seed());
  REQUIRE(hub.seed_source() == fl::primitives::RandomSeedSource::Generated);
}

TEST_CASE("RandomHub rejects invalid FAIRLANES_RNG_SEED in test builds",
          "[random][seed]") {
  ScopedEnvVar env{"FAIRLANES_RNG_SEED"};
  env.set("banana");

#if FL_ENABLE_ASSERTS
  REQUIRE_THROWS_AS(fl::primitives::RandomHub{}, std::invalid_argument);
#else
  fl::primitives::RandomHub hub;
  REQUIRE(hub.seed_source() == fl::primitives::RandomSeedSource::Generated);
#endif
}

TEST_CASE("GrandCentral contexts expose the central RNG seed",
          "[random][seed][context]") {
  ScopedEnvVar env{"FAIRLANES_RNG_SEED"};
  env.set("314159");

  fl::GrandCentral gc{1, 1, 1};
  auto account_ctx = gc.account_context(0);

  REQUIRE(gc.rng().chosen_seed() == 314159ULL);
  REQUIRE(account_ctx.rng().chosen_seed() == gc.rng().chosen_seed());
  REQUIRE(account_ctx.party_context(0).rng().chosen_seed() ==
          gc.rng().chosen_seed());
}

TEST_CASE("RandomHub replays streams from the same explicit seed",
          "[random][seed]") {
  fl::primitives::RandomHub first{123456789ULL};
  fl::primitives::RandomHub second{123456789ULL};
  fl::primitives::RandomHub different{987654321ULL};

  auto first_stream = first.stream("test/replay", 7);
  auto second_stream = second.stream("test/replay", 7);
  auto different_stream = different.stream("test/replay", 7);

  std::array<uint64_t, 5> first_draws{};
  std::array<uint64_t, 5> second_draws{};
  std::array<uint64_t, 5> different_draws{};

  for (std::size_t i = 0; i < first_draws.size(); ++i) {
    first_draws[i] = first_stream.u64();
    second_draws[i] = second_stream.u64();
    different_draws[i] = different_stream.u64();
  }

  REQUIRE(first_draws == second_draws);
  REQUIRE(first_draws != different_draws);
}
