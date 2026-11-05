#pragma once

#include <algorithm>

namespace fl::primitives {

// Intent is independent of damage channels. Only direct hits use flat Armor.
enum class HitKind { Scripted, Direct, Periodic };

/// Represents incoming damage across multiple channels.
struct Damage {
  int physical{0};
  int magical{0};
  int fire{0};
  int ice{0};
  int lightning{0};

  bool is_zero() const {
    return physical == 0 && magical == 0 && fire == 0 && ice == 0 && lightning == 0;
  }

  Damage& operator+=(const Damage& rhs) {
    physical += rhs.physical;
    magical += rhs.magical;
    fire += rhs.fire;
    ice += rhs.ice;
    lightning += rhs.lightning;
    return *this;
  }
};

// Percentage mitigation by damage channel. Values are clamped at resolution.
struct Resistances {
  int physical{0};
  int magical{0};
  int fire{0};
  int ice{0};
  int lightning{0};

  Resistances& operator+=(const Resistances& rhs) {
    physical += rhs.physical;
    magical += rhs.magical;
    fire += rhs.fire;
    ice += rhs.ice;
    lightning += rhs.lightning;
    return *this;
  }
};

inline Damage apply_resistance(Damage damage, const Resistances& resistance) {
  const auto mitigate = [](int value, int percent) {
    return value * (100 - std::clamp(percent, 0, 100)) / 100;
  };
  damage.physical = mitigate(damage.physical, resistance.physical);
  damage.magical = mitigate(damage.magical, resistance.magical);
  damage.fire = mitigate(damage.fire, resistance.fire);
  damage.ice = mitigate(damage.ice, resistance.ice);
  damage.lightning = mitigate(damage.lightning, resistance.lightning);
  return damage;
}

} // namespace fl::primitives
