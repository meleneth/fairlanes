#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "fl/context.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/events/story_bus.hpp"

namespace fl::primitives {

// Deliberately narrow strategy for the first Origin route. Combat execution,
// tower state, recruitment, and encounter scheduling remain authoritative in
// their existing owners; this object only submits legal strategic requests.
class OriginAutoProgression {
public:
  OriginAutoProgression(fl::context::AccountCtx account, std::size_t party_index,
                        fl::events::StoryBus& story_bus, bool initially_enabled = false);
  OriginAutoProgression(const OriginAutoProgression&) = delete;
  OriginAutoProgression& operator=(const OriginAutoProgression&) = delete;

  void evaluate();
  bool enabled() const noexcept {
    return enabled_;
  }

private:
  enum class Outcome { None, Victory, Defeat };
  struct Recovery {
    std::string biome_id;
    int frontier{1};
    int farming_victories{0};
  };

  static constexpr int kAttemptsBeforeRetreat = 3;
  static constexpr int kFarmingVictoriesBeforeRetry = 2;

  PartyData& party() const;
  void set_enabled(bool enabled);
  void request_evaluation() noexcept;
  bool recruit_if_possible();
  bool handle_defeat();
  bool handle_recovery_victory();
  bool select_least_progressed_biome();
  void decide(std::string_view action, std::string_view target = {}, int floor = 0);
  void become_blocked(std::string_view reason);

  fl::context::AccountCtx account_;
  std::size_t party_index_{0};
  fl::events::StoryBus* story_bus_{nullptr};
  bool enabled_{false};
  bool evaluation_pending_{false};
  Outcome outcome_{Outcome::None};
  std::optional<fl::events::TowerFloorCleared> last_clear_;
  std::optional<std::string> completed_biome_;
  std::optional<Recovery> recovery_;
  bool ignore_next_raid_wipe_{false};
  fl::events::ScopedStoryListener autoplay_sub_;
  std::vector<fl::events::ScopedPartyListener> party_subs_;
  std::vector<fl::events::ScopedPartyListener> recruitment_subs_;
};

} // namespace fl::primitives
