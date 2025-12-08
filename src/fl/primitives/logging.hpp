#pragma once
#include <cstdint>
#include <string>
#include <string_view>

#include <eventpp/eventdispatcher.h>

namespace fl::primitives {

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

enum class LogKey { Message };

using LogBus = eventpp::EventDispatcher<LogKey, void(const LogEvent &)>;

class Logger {
  LogBus &bus_;

public:
  explicit Logger(LogBus &bus) : bus_(bus) {}

  void log(LogLevel level, std::string_view msg) {
    LogEvent ev;
    ev.level = level;
    ev.message.assign(msg);
    bus_.dispatch(LogKey::Message, ev);
  }

  void trace(std::string_view msg) { log(LogLevel::trace, msg); }
  void debug(std::string_view msg) { log(LogLevel::debug, msg); }
  void info(std::string_view msg) { log(LogLevel::info, msg); }
  void warn(std::string_view msg) { log(LogLevel::warn, msg); }
  void error(std::string_view msg) { log(LogLevel::error, msg); }
};

struct Logging {
  LogBus bus;
  Logger logger;

  Logging() : bus(), logger(bus) {}
};

} // namespace fl::primitives
