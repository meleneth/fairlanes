#pragma once
#include "atb_engine.hpp" // adapt your ATB engine to subscribe to EncounterBus
#include "encounter_bus.hpp"
#include "encounter_view.hpp"
#include "timed_scheduler.hpp"

namespace seerin {

template <EncounterView View> class EncounterRuntime {
public:
  explicit EncounterRuntime(View &view)
      : view_(view), bus_(),
        scheduler_([&](const EncounterEvent &ev) { bus_.emit(ev); }),
        atb_(bus_, bus_) // if your ATB engine currently expects separate
                         // in/out, adjust to EncounterBus
  {
    // Beat advances scheduler clock.
    bus_.on<Beat>([this](const Beat &) { scheduler_.on_beat(); });

    // Example: validate actions (this is where encounter access matters)
    bus_.on<ActionRequested>([this](const ActionRequested &a) {
      if (!view_.is_alive(a.caster))
        return;
      if (!view_.is_alive(a.target))
        return;

      // Placeholder “resolve”: schedule an effect 2 beats later
      scheduler_.schedule_in_beats(
          2, EncounterEvent{ApplyEffect{a.caster, a.target, a.skill, 10}});
    });

    // ApplyEffect could also be handled here or by another core system.
  }

  EncounterBus &bus() { return bus_; }
  TimedScheduler<EncounterEvent> &scheduler() { return scheduler_; }
  View &view() { return view_; }

private:
  View &view_;
  EncounterBus bus_;
  TimedScheduler<EncounterEvent> scheduler_;
  AtbEngine atb_;
};

} // namespace seerin
