#pragma once
#include <cstdint>
#include <type_traits>

namespace seerin {

struct uWu final {
  int64_t v = 0;

  constexpr uWu() = default;
  constexpr explicit uWu(int64_t x) : v(x) {}

  // Arithmetic
  friend constexpr uWu operator+(uWu a, uWu b) { return uWu{a.v + b.v}; }
  friend constexpr uWu operator-(uWu a, uWu b) { return uWu{a.v - b.v}; }
  friend constexpr uWu &operator+=(uWu &a, uWu b) {
    a.v += b.v;
    return a;
  }
  friend constexpr uWu &operator-=(uWu &a, uWu b) {
    a.v -= b.v;
    return a;
  }

  // Comparisons
  friend constexpr bool operator==(uWu a, uWu b) { return a.v == b.v; }
  friend constexpr bool operator!=(uWu a, uWu b) { return a.v != b.v; }
  friend constexpr bool operator<(uWu a, uWu b) { return a.v < b.v; }
  friend constexpr bool operator<=(uWu a, uWu b) { return a.v <= b.v; }
  friend constexpr bool operator>(uWu a, uWu b) { return a.v > b.v; }
  friend constexpr bool operator>=(uWu a, uWu b) { return a.v >= b.v; }
};

constexpr uWu UWU_PER_BEAT{80};
constexpr int BEATS_PER_SEC = 12;

static_assert(std::is_trivially_copyable_v<uWu>);
static_assert(UWU_PER_BEAT.v == 80);

} // namespace seerin
