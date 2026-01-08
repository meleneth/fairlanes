#pragma once
// atb_context.hpp

#include "atb_events.hpp"

#include <algorithm>
#include <cstdint>

namespace seerin::atb {

struct Config {
  std::int64_t max_charge_units{1000};
  std::int64_t speed_units_per_sec{250};
};

struct Context {
  Config cfg{};

  // gauge
  std::int64_t charge_units{0};
  std::int64_t accum{0}; // remainder of (dt_ns * speed) mod 1e9

  // acting
  ns action_remaining{0};

  // stun
  ns stun_remaining{0};

  static std::int64_t clamp_i64(std::int64_t v, std::int64_t lo,
                                std::int64_t hi) {
    return std::max(lo, std::min(v, hi));
  }

  // Predict whether a Beat will fill the gauge (without mutating).
  bool will_fill_on(const Beat &b) const {
    (void)b;
    /*
    if (cfg.speed_units_per_sec <= 0)
      return false;
    if (charge_units >= cfg.max_charge_units)
      return true;

    const std::int64_t dt_ns = b.dt.count();
    if (dt_ns <= 0)
      return false;

    constexpr std::int64_t denom = 1'000'000'000LL;

    const std::int64_t speed = cfg.speed_units_per_sec;

    // Split dt to reduce overflow risk.
    const std::int64_t whole_sec = dt_ns / denom;
    const std::int64_t rem_ns = dt_ns % denom; // 0..(1e9-1)

    // whole_sec * speed might overflow if either is huge.
    // If you truly accept "crash is fine", assert it loudly in debug:
    // (or clamp dt/whole_sec to some max catch-up window)
    // assert(whole_sec <= (std::numeric_limits<std::int64_t>::max() / speed));

    const std::int64_t base_units = whole_sec * speed;

    // Fractional part: (accum + rem_ns * speed) / 1e9
    // Use __int128 only as an intermediate if you want it absolutely safe.
    const __int128 frac_numer =
        static_cast<__int128>(accum) +
        static_cast<__int128>(rem_ns) * static_cast<__int128>(speed);

    const std::int64_t frac_units =
        static_cast<std::int64_t>(frac_numer / denom);

    // Total delta units this beat
    const std::int64_t delta_units = base_units + frac_units;

    // Saturating add is usually what you actually want for a "will fill?"
    // check. This avoids overflow in charge_units + delta_units.
    const std::int64_t headroom = cfg.max_charge_units - charge_units;
    */
    ;
    return false;
    // return delta_units >= headroom;
  }

  void apply_charge(const Beat &b) {
    (void)b;
    /*
    if (cfg.speed_units_per_sec <= 0)
      return;
    if (b.dt.count() <= 0)
      return;
    if (charge_units >= cfg.max_charge_units) {
      charge_units = cfg.max_charge_units;
      return;
    }

    const std::int64_t dt_ns = b.dt.count();
    const std::int64_t speed = cfg.speed_units_per_sec;

    __int128 accum_big =
        static_cast<__int128>(accum) +
        static_cast<__int128>(dt_ns) * static_cast<__int128>(speed);
    const __int128 denom = static_cast<__int128>(1'000'000'000LL);

    const std::int64_t delta_units =
        static_cast<std::int64_t>(accum_big / denom);
    accum = static_cast<std::int64_t>(accum_big % denom);

    charge_units += delta_units;
    if (charge_units >= cfg.max_charge_units) {
      charge_units = cfg.max_charge_units;
    }
      */
  }

  bool will_finish_action_on(const Beat &b) const {
    if (b.dt.count() <= 0)
      return false;
    return b.dt >= action_remaining;
  }

  void tick_action(const Beat &b) {
    if (b.dt.count() <= 0)
      return;
    if (action_remaining.count() <= 0)
      return;
    if (b.dt >= action_remaining)
      action_remaining = ns{0};
    else
      action_remaining -= b.dt;
  }

  bool will_end_stun_on(const Beat &b) const {
    if (b.dt.count() <= 0)
      return false;
    return b.dt >= stun_remaining;
  }

  void tick_stun(const Beat &b) {
    if (b.dt.count() <= 0)
      return;
    if (stun_remaining.count() <= 0)
      return;
    if (b.dt >= stun_remaining)
      stun_remaining = ns{0};
    else
      stun_remaining -= b.dt;
  }

  void reset_after_action() {
    charge_units = 0;
    accum = 0;
    action_remaining = ns{0};
  }
};

} // namespace seerin::atb
