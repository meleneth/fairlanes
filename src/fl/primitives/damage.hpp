#pragma once
#include <algorithm>

namespace fl::primitives {

/// Represents incoming damage across multiple channels.
struct Damage {
  int physical{0};
  int magical{0};
  int fire{0};
  int ice{0};
  int lightning{0};

  bool is_zero() const {
    return physical == 0 && magical == 0 && fire == 0 && ice == 0 &&
           lightning == 0;
  }

  Damage &operator+=(const Damage &rhs) {
    physical += rhs.physical;
    magical += rhs.magical;
    fire += rhs.fire;
    ice += rhs.ice;
    lightning += rhs.lightning;
    return *this;
  }
};

/// Represents per-type resistances as percentages (0–100).
struct Resistances {
  int physical{0};
  int magical{0};
  int fire{0};
  int ice{0};
  int lightning{0};
};

/// Applies resistances to a given damage object.
inline Damage apply_resistance(const Damage &dmg, const Resistances &res) {
  Damage out;
  auto apply = [](int value, int resist) {
    resist = std::clamp(resist, 0, 100);
    return value * (100 - resist) / 100;
  };
  out.physical = apply(dmg.physical, res.physical);
  out.magical = apply(dmg.magical, res.magical);
  out.fire = apply(dmg.fire, res.fire);
  out.ice = apply(dmg.ice, res.ice);
  out.lightning = apply(dmg.lightning, res.lightning);
  return out;
}

} // namespace fl::primitives
