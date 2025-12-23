#pragma once

#include <cstdint>
#include <functional>
#include <variant>

#include <eventpp/eventdispatcher.h>

#include "fl/events/beat.hpp"

namespace fl::events {

// If you truly only ever want one event, this is still fine.
// It buys you handles + consistent pattern across buses.
enum class BeatEventId : std::uint8_t {
  Beat = 0,
};

// Payload wrapper (lets you add more payload types later without changing
// signature)
struct BeatPulse {
  Beat beat;
};

// Envelope
struct BeatEvent {
  BeatEventId id{BeatEventId::Beat};
  std::variant<BeatPulse> data;

  static BeatEvent make_beat(const Beat &beat) {
    BeatEvent ev;
    ev.id = BeatEventId::Beat;
    ev.data = BeatPulse{beat};
    return ev;
  }
};

using BeatDispatcher =
    eventpp::EventDispatcher<BeatEventId, void(const BeatEvent &)>;

class BeatBus {
public:
  using Listener = std::function<void(const BeatEvent &)>;
  using Handle = BeatDispatcher::Handle;

  [[nodiscard]] Handle add_listener(BeatEventId id, Listener listener) {
    return dispatcher_.appendListener(id, std::move(listener));
  }

  void emit(const BeatEvent &ev) { dispatcher_.dispatch(ev.id, ev); }

  // Convenience emitter
  void beat(const Beat &b) { emit(BeatEvent::make_beat(b)); }

private:
  BeatDispatcher dispatcher_{};
};

} // namespace fl::events
