#pragma once

#include <cassert>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include <pcg_random.hpp>

namespace fl::primitives {

// Choose your engine here.
using Engine = pcg64;

enum class RandomSeedSource {
  Environment,
  Generated,
  Explicit,
};

struct RandomSeedConfig {
  uint64_t seed{0};
  RandomSeedSource source{RandomSeedSource::Generated};
};

// ------------------------------ Utilities ------------------------------

constexpr uint64_t fnv1a64(std::string_view s) noexcept {
  constexpr uint64_t kOffset = 14695981039346656037ULL;
  constexpr uint64_t kPrime = 1099511628211ULL;

  uint64_t h = kOffset;
  for (size_t i = 0; i < s.size(); ++i) {
    const unsigned char c = static_cast<unsigned char>(s[i]);
    h ^= c;
    h *= kPrime;
  }
  return h;
}

constexpr uint64_t splitmix64_once(uint64_t x) noexcept {
  x += 0x9E3779B97F4A7C15ULL;
  x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
  x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
  return x ^ (x >> 31);
}

inline uint64_t time_seed_now() {
  using clock = std::chrono::steady_clock;
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          clock::now().time_since_epoch())
          .count());
}

inline std::optional<uint64_t> parse_unsigned_seed(std::string_view text) {
  if (text.empty()) {
    return std::nullopt;
  }

  uint64_t value = 0;
  const auto *begin = text.data();
  const auto *end = begin + text.size();
  const auto [ptr, ec] = std::from_chars(begin, end, value, 10);
  if (ec != std::errc{} || ptr != end) {
    return std::nullopt;
  }

  return value;
}

inline RandomSeedConfig generated_random_seed_config() {
  return RandomSeedConfig{splitmix64_once(time_seed_now()),
                          RandomSeedSource::Generated};
}

inline RandomSeedConfig resolve_random_seed_from_environment() {
  constexpr std::string_view kEnvName = "FAIRLANES_RNG_SEED";
  const char *env = std::getenv(kEnvName.data());
  if (env == nullptr) {
    return generated_random_seed_config();
  }

  if (const auto parsed = parse_unsigned_seed(env)) {
    return RandomSeedConfig{*parsed, RandomSeedSource::Environment};
  }

#if FL_ENABLE_ASSERTS
  throw std::invalid_argument(
      "FAIRLANES_RNG_SEED must be an unsigned integer");
#else
  std::cerr << "[fairlanes] warning: ignoring invalid FAIRLANES_RNG_SEED=\""
            << env << "\"; using generated seed instead\n";
  return generated_random_seed_config();
#endif
}

inline void report_random_seed(const RandomSeedConfig &config) {
#if FL_ENABLE_ASSERTS
  const char *source = "generated";
  if (config.source == RandomSeedSource::Environment) {
    source = "environment";
  } else if (config.source == RandomSeedSource::Explicit) {
    source = "explicit";
  }
  std::cerr << "[fairlanes] RNG seed " << config.seed << " (" << source
            << ")\n";
#else
  (void)config;
#endif
}

// ------------------------------ RandomStream ------------------------------

class RandomStream {
public:
  RandomStream() = default;

  RandomStream(uint64_t master_seed, uint64_t sequence,
               uint64_t cache_namespace, std::string key)
      : master_seed_(master_seed), sequence_(sequence), key_(std::move(key)) {
    key_ = std::to_string(cache_namespace) + ":" +
           std::to_string(master_seed_) + ":" + std::to_string(sequence_) +
           ":" + key_;
  }

  // Thread-local engine per (stream key). Deterministic (no thread salt).
  Engine &eng() const {
    thread_local std::unordered_map<std::string, Engine> tls;
    auto it = tls.find(key_);
    if (it == tls.end()) {
      it = tls.emplace(key_, Engine(master_seed_, sequence_)).first;
    }
    return it->second;
  }

  // Core draws
  uint64_t u64() { return std::uniform_int_distribution<uint64_t>()(eng()); }
  uint32_t u32() { return std::uniform_int_distribution<uint32_t>()(eng()); }

  template <typename Int> Int uniform_int(Int lo, Int hi_inclusive) {
    return std::uniform_int_distribution<Int>(lo, hi_inclusive)(eng());
  }

  // Index in [0, count)
  int random_index(int count) {
    assert(count > 0);
    if (count == 1)
      return 0;
    return uniform_int<int>(0, count - 1);
  }

  uint64_t master_seed() const noexcept { return master_seed_; }
  uint64_t sequence() const noexcept { return sequence_; }
  const std::string &key() const noexcept { return key_; }

private:
  uint64_t master_seed_{0};
  uint64_t sequence_{1};       // pcg handles increment internally
  std::string key_{"<unset>"}; // TLS slot key
};

// ------------------------------ RandomHub ------------------------------

class RandomHub {
public:
  // If seed == nullopt, resolve from FAIRLANES_RNG_SEED or generate one.
  explicit RandomHub(std::optional<uint64_t> seed = std::nullopt,
                     uint64_t base_sequence = 0)
      : RandomHub(seed ? RandomSeedConfig{*seed, RandomSeedSource::Explicit}
                       : resolve_random_seed_from_environment(),
                  base_sequence) {}

  explicit RandomHub(RandomSeedConfig seed_config, uint64_t base_sequence = 0)
      : master_seed_(seed_config.seed), base_sequence_(base_sequence),
        seed_source_(seed_config.source),
        cache_namespace_(next_cache_namespace()) {
    report_random_seed(seed_config);
  }

  uint64_t master_seed() const noexcept { return master_seed_; }
  uint64_t chosen_seed() const noexcept { return master_seed_; }
  RandomSeedSource seed_source() const noexcept { return seed_source_; }
  uint64_t base_sequence() const noexcept { return base_sequence_; }

  // Named stream: sequence selector derived from base_sequence ^ hash(name) ^
  // sub_seq
  RandomStream stream(std::string_view name, uint64_t sub_seq = 0) const {
    const uint64_t seq = mix_seq(base_sequence_, fnv1a64(name), sub_seq);
    return RandomStream(master_seed_, seq, cache_namespace_,
                        key_for(name, sub_seq));
  }

  // Pure numeric stream (no name)
  RandomStream stream(uint64_t sub_seq) const {
    const uint64_t seq =
        mix_seq(base_sequence_, 0xD1B54A32D192ED03ULL, sub_seq);
    return RandomStream(master_seed_, seq, cache_namespace_,
                        key_for("<num>", sub_seq));
  }

private:
  static uint64_t next_cache_namespace() {
    static std::atomic<uint64_t> next{1};
    return next.fetch_add(1, std::memory_order_relaxed);
  }

  static uint64_t mix_seq(uint64_t base, uint64_t name_hash, uint64_t sub) {
    // 3-way mix → stream selector
    return splitmix64_once(base ^ (name_hash + 0x9E3779B97F4A7C15ULL) ^
                           (sub * 0xBF58476D1CE4E5B9ULL));
  }

  static std::string key_for(std::string_view name, uint64_t sub) {
    if (sub == 0)
      return std::string(name);
    return std::string(name) + "#" + std::to_string(sub);
  }

  uint64_t master_seed_;
  uint64_t base_sequence_;
  RandomSeedSource seed_source_{RandomSeedSource::Generated};
  uint64_t cache_namespace_{0};
};

} // namespace fl::primitives
