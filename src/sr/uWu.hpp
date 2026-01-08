#pragma once
// uWu.hpp
// Simple "Universal Wait uWu" gauge for fixed tick increments.
#include <cstdint>

namespace seerin {

class uWu {
public:
  using rep = std::int64_t;

  static constexpr rep kTickDelta = 80;

  explicit constexpr uWu(rep max_charge_units)
      : max_(max_charge_units), cur_(0) {}

  constexpr rep value() const { return cur_; }
  constexpr rep max() const { return max_; }
  constexpr bool full() const { return cur_ >= max_; }

  // Pure predicate: would a tick fill it?
  constexpr bool will_fill_on_tick() const {
    if (full())
      return true;
    const rep headroom = max_ - cur_; // safe because !full()
    return kTickDelta >= headroom;
  }

  // Mutation: apply one tick (saturating, no overflow).
  constexpr void tick() {
    if (full())
      return;
    const rep headroom = max_ - cur_;
    cur_ = (kTickDelta >= headroom) ? max_ : (cur_ + kTickDelta);
  }

  // Optional: apply N ticks without overflow / UB.
  constexpr void tick_n(rep ticks) {
    if (ticks <= 0 || full())
      return;

    // Saturating multiply-add via headroom comparison to avoid overflow.
    const rep headroom = max_ - cur_;
    // If ticks * kTickDelta >= headroom, fill.
    // Avoid overflow: compare ticks >= ceil(headroom / kTickDelta)
    const rep need = (headroom + kTickDelta - 1) / kTickDelta;
    if (ticks >= need) {
      cur_ = max_;
      return;
    }
    cur_ +=
        ticks * kTickDelta; // safe because ticks < need ⇒ product < headroom
  }

private:
  rep max_;
  rep cur_;
};
} // namespace seerin
