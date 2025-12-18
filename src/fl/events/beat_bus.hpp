#pragma once

#include <eventpp/callbacklist.h>

#include "fl/events/beat.hpp"

namespace fl::events {

/// Broadcasts Beat events to all subscribers.
/// Usage:
///   beat_bus.append([](const Beat& b){ ... });
///   beat_bus(b);   // emit
using BeatBus = eventpp::CallbackList<void(const Beat &)>;

} // namespace fl::events
