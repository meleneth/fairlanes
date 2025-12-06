#pragma once

#include <eventpp/eventdispatcher.h>

#include "log_event.hpp"

namespace fl::events {

enum class LogKey { Message };

using LogBus = eventpp::EventDispatcher<LogKey, void(const LogEvent &)>;

} // namespace fl::events
