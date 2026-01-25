#pragma once

#include <eventpp/eventdispatcher.h>

#include "fl/widgets/fancy_log.hpp"
#include "logging.hpp"

namespace fl::primitives {

class FancyLogSink {
  fl::primitives::LogBus &bus_;
  fl::widgets::FancyLog &view_;
  fl::primitives::LogLevel min_level_;
  fl::primitives::LogBus::Handle handle_;

public:
  FancyLogSink(
      fl::primitives::LogBus &bus, fl::widgets::FancyLog &view,
      fl::primitives::LogLevel min_level = fl::primitives::LogLevel::info)
      : bus_(bus), view_(view), min_level_(min_level),
        handle_(bus_.appendListener(
            fl::primitives::LogKey::Message,
            [this](const fl::primitives::LogEvent &ev) { on_log(ev); })) {
    fprintf(stderr, "Sink view=%p this=%p\n", (void *)&view_, (void *)this);
  }

  ~FancyLogSink() {
    bus_.removeListener(fl::primitives::LogKey::Message, handle_);
  }

  FancyLogSink(const FancyLogSink &) = delete;
  FancyLogSink &operator=(const FancyLogSink &) = delete;

  FancyLogSink(FancyLogSink &&) = delete;
  FancyLogSink &operator=(FancyLogSink &&) = delete;

private:
  void on_log(const fl::primitives::LogEvent &ev) {
    if (static_cast<int>(ev.level) < static_cast<int>(min_level_))
      return;
    view_.append_markup(ev.message);
  }
};

} // namespace fl::primitives
