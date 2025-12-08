#pragma once

#include <eventpp/eventdispatcher.h>

#include "fl/widgets/fancy_log.hpp"
#include "logging.hpp"

namespace fl::primitives {

class FancyLogSink {
  fl::widgets::FancyLog &view_;
  fl::primitives::LogLevel min_level_;
  fl::primitives::LogBus::Handle handle_;

public:
  FancyLogSink(
      fl::primitives::LogBus &bus, fl::widgets::FancyLog &view,
      fl::primitives::LogLevel min_level = fl::primitives::LogLevel::info)
      : view_(view), min_level_(min_level),
        handle_(bus.appendListener(
            fl::primitives::LogKey::Message,
            [this](const fl::primitives::LogEvent &ev) { on_log(ev); })) {}

private:
  void on_log(const fl::primitives::LogEvent &ev) {
    if (static_cast<int>(ev.level) < static_cast<int>(min_level_)) {
      return;
    }

    view_.append_markup(ev.message);
  }
};

} // namespace fl::primitives
