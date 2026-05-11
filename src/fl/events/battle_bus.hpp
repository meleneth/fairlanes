#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <utility>
#include <variant>

#include <entt/entt.hpp>

#include "sr/variant_bus.hpp"

namespace fl::events {

// --- Event payloads (data-only) ---

struct StartCombat {
  entt::entity encounter{entt::null}; // or whatever "battle root" entity is
};

struct BattleTick {
  std::chrono::milliseconds dt{0};
};

enum class EndCombatReason : std::uint8_t {
  Victory = 0,
  Defeat = 1,
  Fled = 2,
  Aborted = 3,
};

struct EndCombat {
  entt::entity encounter{entt::null};
  EndCombatReason reason{EndCombatReason::Aborted};
};

using BattleEvent = std::variant<StartCombat, BattleTick, EndCombat>;

class BattleBus {
public:
  template <class T, class Fn> auto on(Fn &&listener) {
    return bus_.template on<T>(std::forward<Fn>(listener));
  }

  void emit(const BattleEvent &ev) { bus_.emit(ev); }
  void start_combat(entt::entity encounter);
  void tick(std::chrono::milliseconds dt);
  void end_combat(entt::entity encounter, EndCombatReason reason);

private:
  seerin::VariantBus<BattleEvent> bus_{};
};

} // namespace fl::events
