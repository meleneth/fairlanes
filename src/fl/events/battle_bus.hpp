#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <variant>

#include <entt/entt.hpp>
#include <eventpp/eventdispatcher.h>

namespace fl::events {

// What kind of thing happened?
enum class BattleEventId : std::uint8_t {
  StartCombat = 0,
  Tick = 1,
  EndCombat = 2,
};

// --- Event payloads (data-only) ---

struct StartCombat {
  entt::entity encounter{entt::null}; // or whatever "battle root" entity is
};

struct Tick {
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

// One envelope type so EventPP listeners always receive a single type.
// This keeps the dispatcher signature stable as the event set grows.
struct BattleEvent {
  BattleEventId id{BattleEventId::Tick};
  std::variant<StartCombat, Tick, EndCombat> data;

  static BattleEvent make_start(entt::entity encounter);
  static BattleEvent make_tick(std::chrono::milliseconds dt);
  static BattleEvent make_end(entt::entity encounter, EndCombatReason reason);
};

// EventPP dispatcher type
using BattleDispatcher =
    eventpp::EventDispatcher<BattleEventId, void(const BattleEvent &)>;

class BattleBus {
public:
  using Listener = std::function<void(const BattleEvent &)>;

  // Listener mgmt
  void add_listener(BattleEventId id, Listener listener);

  // Emit
  void emit(const BattleEvent &ev);
  void start_combat(entt::entity encounter);
  void tick(std::chrono::milliseconds dt);
  void end_combat(entt::entity encounter, EndCombatReason reason);

private:
  BattleDispatcher dispatcher_{};
};

} // namespace fl::events
