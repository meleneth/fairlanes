# eventpp

Fairlanes uses [eventpp](https://github.com/wqking/eventpp) as low-level callback infrastructure, but the current game code usually reaches it through small project bus wrappers.

## Current Usage

The active project-level event bus is `seerin::VariantBus<std::variant<...>>`, defined in `src/sr/variant_bus.hpp`. It stores one `eventpp::CallbackList` per payload type and exposes:

- `emit(Variant{payload})` for producers
- `on<T>(...)` / `subscribe<T>(...)` for listeners
- RAII-style subscriptions for Seerin buses

Fairlanes domain buses use that wrapper:

- `fl::events::PartyBus`
- `fl::events::CombatantBus`
- `seerin::BeatBus`
- `seerin::AtbInBus`
- `seerin::AtbOutBus`

The direct `eventpp::EventDispatcher` usage that remains is the root logging path in `src/fl/primitives/logging.hpp`: `Logger` dispatches `fl::primitives::LogEvent`, and `FancyLogSink` listens and appends markup to a `FancyLog`.

## How To Read Event Code

Start from the domain type, not from `eventpp`:

- `src/fl/events/party_bus.hpp` for party and combatant events
- `src/sr/atb_events.hpp` for ATB input/output events
- `src/sr/variant_bus.hpp` for the wrapper mechanics
- `src/fl/primitives/logging.hpp` for the direct logging dispatcher

Useful searches:

- `emit(fl::events::PartyEvent`
- `emit(fl::events::CombatantEvent`
- `atb_in().emit`
- `atb_out().subscribe`
- `ScopedPartyListener`
- `ScopedCombatantListener`
- `appendListener`

## Mental Model

```text
payload type -> VariantBus::emit(...) -> typed callback list -> listener
```

For the combat loop, the important chain is concrete:

```text
seerin::Beat -> PartyTick -> ATB -> BecameActive -> skill scheduling -> FinishedTurn
```

