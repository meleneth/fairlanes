#pragma once

#include <cstdint>
#include <initializer_list>
#include <string_view>

namespace fl::primitives {

enum class CombatAffinity : std::uint8_t { Neutral = 0, Cloth = 1, Leather = 2, Plate = 3 };

class CombatAffinitySet {
public:
  constexpr CombatAffinitySet() = default;
  constexpr explicit CombatAffinitySet(CombatAffinity affinity) {
    add(affinity);
  }
  constexpr CombatAffinitySet(std::initializer_list<CombatAffinity> affinities) {
    for (auto affinity : affinities) add(affinity);
  }

  constexpr void add(CombatAffinity affinity) noexcept {
    if (affinity != CombatAffinity::Neutral)
      bits_ |= static_cast<std::uint8_t>(1U << static_cast<unsigned>(affinity));
  }
  [[nodiscard]] constexpr bool contains(CombatAffinity affinity) const noexcept {
    return affinity != CombatAffinity::Neutral &&
           (bits_ & static_cast<std::uint8_t>(1U << static_cast<unsigned>(affinity))) != 0;
  }
  [[nodiscard]] constexpr bool empty() const noexcept {
    return bits_ == 0;
  }
  [[nodiscard]] constexpr int size() const noexcept {
    return static_cast<int>((bits_ & 2U) != 0) + static_cast<int>((bits_ & 4U) != 0) +
           static_cast<int>((bits_ & 8U) != 0);
  }
  [[nodiscard]] constexpr std::uint8_t bits() const noexcept {
    return bits_;
  }

private:
  std::uint8_t bits_{0};
};

inline constexpr std::string_view to_string(CombatAffinity affinity) noexcept {
  switch (affinity) {
  case CombatAffinity::Neutral: return "Neutral";
  case CombatAffinity::Cloth: return "Cloth";
  case CombatAffinity::Leather: return "Leather";
  case CombatAffinity::Plate: return "Plate";
  }
  return "Unknown";
}

} // namespace fl::primitives
