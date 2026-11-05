#pragma once

#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "fl/context.hpp"
#include "fl/events/raid_bus.hpp"
#include "fl/events/story_bus.hpp"
#include "fl/generated/story_content.hpp"

namespace fl::primitives {

// Save-wide story state for the first playable account. Facts are dispatched
// through a bus; generated objectives observe fact IDs, while effects remain
// handwritten and testable here.
class ObjectiveSystem {
public:
  explicit ObjectiveSystem(fl::context::AccountCtx account);
  ObjectiveSystem(const ObjectiveSystem&) = delete;
  ObjectiveSystem& operator=(const ObjectiveSystem&) = delete;

  void emit(std::string_view event_id);
  fl::events::StoryBus& bus() noexcept {
    return bus_;
  }
  bool active(std::string_view objective_id) const;
  bool completed(std::string_view objective_id) const;
  bool can_talk(std::string_view dialogue_id) const;
  bool can_buy_village_guard_hide() const;
  bool buy_village_guard_hide();
  int darrin_marks() const;
  void request_autoplay(bool enabled);
  void request_purchase(std::string_view item_id);
  bool autoplay_enabled() const noexcept {
    return autoplay_enabled_;
  }
  std::size_t autoplay_step_index() const noexcept {
    return autoplay_step_index_;
  }
  void advance_autoplay();
  const std::vector<std::string>& unhandled_events() const {
    return unhandled_;
  }
  static bool supports(std::string_view event_id);

private:
  void receive(std::string_view event_id);
  void handle(std::string_view event_id);
  fl::context::AccountCtx account_;
  std::unordered_set<std::string> facts_;
  std::vector<std::string> unhandled_;
  fl::events::StoryBus bus_;
  fl::events::ScopedStoryListener fact_sub_;
  fl::events::ScopedStoryListener autoplay_sub_;
  fl::events::ScopedStoryListener purchase_sub_;
  bool autoplay_enabled_{false};
  std::size_t autoplay_step_index_{0};
  bool returned_after_two_{false};
  fl::events::ScopedRaidListener raid_sub_;
  fl::events::ScopedPartyListener victory_sub_;
  fl::events::ScopedPartyListener return_sub_;
};

} // namespace fl::primitives
