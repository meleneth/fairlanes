#pragma once

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

} // namespace fl::primitives
