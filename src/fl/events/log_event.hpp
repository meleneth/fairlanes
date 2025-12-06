#pragma once

#include <cstdint>
#include <string>

namespace fl::events {

enum class LogLevel : std::uint8_t {
  trace,
  debug,
  info,
  warn,
  error,
};

struct LogEvent {
  LogLevel level{LogLevel::info};
  std::string message;
};

} // namespace fl::events
