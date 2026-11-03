#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <nlohmann/json_fwd.hpp>

namespace fl {
class GrandCentral;
}

namespace fl::regression {

inline constexpr std::string_view kSmokeFixture = "woodland_mouse_duel";
inline constexpr std::string_view kArmorFixture = "woodland_armor_duel";

struct Options {
  std::string fixture{std::string{kSmokeFixture}};
  std::uint64_t seed{42};
  std::uint64_t beat_cap{1200};
  int hero_armor{0};
  std::string variant{};
};

class Session {
public:
  explicit Session(Options options);
  ~Session();
  Session(const Session &) = delete;
  Session &operator=(const Session &) = delete;

  void advance_beat();
  // Call after GrandCentral has dispatched a beat in its client loop.
  void observe_beat();
  [[nodiscard]] bool finished() const;
  [[nodiscard]] std::string_view outcome() const;
  [[nodiscard]] std::uint64_t beats() const;
  [[nodiscard]] std::int64_t uwu() const;
  [[nodiscard]] const nlohmann::json &recording() const;
  [[nodiscard]] fl::GrandCentral &game();
  void write(const std::string &path) const;
  [[nodiscard]] std::optional<std::string>
  first_divergence(const nlohmann::json &expected) const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

nlohmann::json read_recording(const std::string &path);
Options options_from_recording(const nlohmann::json &recording);

} // namespace fl::regression
