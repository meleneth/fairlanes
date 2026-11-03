#pragma once

#include <string>
#include <variant>

#include "fl/events/party_bus.hpp"

namespace fl::events {
struct StoryFact { std::string id; };
struct ObjectiveRevealed { std::string id; };
struct AutoplayRequested { bool enabled; };
struct AutoplayChanged { bool enabled; };
struct VillagePurchaseRequested { std::string item_id; };
using StoryEvent = std::variant<StoryFact, ObjectiveRevealed,
                                AutoplayRequested, AutoplayChanged,
                                VillagePurchaseRequested>;
using StoryBus = seerin::VariantBus<StoryEvent>;
using ScopedStoryListener = ScopedListener<StoryBus>;
} // namespace fl::events
