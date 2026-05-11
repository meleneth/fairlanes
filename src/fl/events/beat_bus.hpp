#pragma once

#include <utility>
#include <variant>

#include "sr/variant_bus.hpp"

#include "fl/events/beat.hpp"

namespace fl::events {

struct BeatPulse {
  Beat beat;
};

using BeatEvent = std::variant<BeatPulse>;

class BeatBus {
public:
  template <class T, class Fn> auto on(Fn &&listener) {
    return bus_.template on<T>(std::forward<Fn>(listener));
  }

  void emit(const BeatEvent &ev) { bus_.emit(ev); }

  void beat(const Beat &b) { emit(BeatEvent{BeatPulse{b}}); }

private:
  seerin::VariantBus<BeatEvent> bus_{};
};

} // namespace fl::events
