#pragma once

#include <cstddef>
#include <string>
#include <variant>

#include "fl/events/party_bus.hpp"

namespace fl::events {
struct StoryFact {
  std::string id;
};
struct ObjectiveRevealed {
  std::string id;
};
struct AutoplayRequested {
  bool enabled;
};
struct AutoplayChanged {
  bool enabled;
};
struct VillagePurchaseRequested {
  std::string item_id;
};
struct RecruitmentClaimRequested {
  std::string opportunity_id;
};
struct OriginAutoProgressionDecision {
  std::size_t party_index{0};
  std::string action;
  std::string target;
  int floor{0};
};
using StoryEvent = std::variant<StoryFact, ObjectiveRevealed, AutoplayRequested, AutoplayChanged,
                                VillagePurchaseRequested, RecruitmentClaimRequested,
                                OriginAutoProgressionDecision>;
using StoryBus = seerin::VariantBus<StoryEvent>;
using ScopedStoryListener = ScopedListener<StoryBus>;
} // namespace fl::events
