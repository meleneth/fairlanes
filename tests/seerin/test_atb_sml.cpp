#include <catch2/catch_test_macros.hpp>

#include "sr/atb_engine.hpp"

namespace {
using namespace seerin;
template <typename T> int count(const std::vector<AtbOutEvent> &evs) {
  int n = 0;
  for (const auto &ev : evs) {
    std::visit(
        [&](const auto &e) {
          using U = std::decay_t<decltype(e)>;
          if constexpr (std::is_same_v<U, T>)
            ++n;
        },
        ev);
  }
  return n;
}
} // namespace

TEST_CASE("ATB: 60 beats makes a combatant ready (SML, no deltas)") {
  using namespace seerin;
  AtbInBus in;
  AtbOutBus out;
  AtbEngine eng(in, out);

  std::vector<AtbOutEvent> recorded;
  out.on<BecameReady>(
      [&](const BecameReady &e) { recorded.push_back(AtbOutEvent{e}); });

  in.emit(AtbInEvent{AddCombatant{1}});

  for (int i = 0; i < 59; ++i) {
    in.emit(AtbInEvent{Beat{}});
  }
  REQUIRE(count<BecameReady>(recorded) == 0);

  in.emit(AtbInEvent{Beat{}});
  REQUIRE(count<BecameReady>(recorded) == 1);
}
