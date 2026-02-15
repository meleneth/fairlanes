#pragma once
#include <variant>

#include <eventpp/callbacklist.h>

#include "atb_events.hpp"  // for seerin::Beat
#include "variant_bus.hpp" // for seerin::VariantBus

namespace seerin {

// Restricted, global fanout bus: only Beat ticks.
using BeatEvent = std::variant<Beat>;
using BeatBus = VariantBus<BeatEvent>;
using BeatSub = eventpp::CallbackList<void(const Beat &)>::Handle;

} // namespace seerin
