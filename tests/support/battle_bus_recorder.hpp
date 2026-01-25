#pragma once

#include <chrono>
#include <vector>

#include "fl/events/battle_bus.hpp" // adjust include path

namespace fl::test_support {

struct RecordedBattleEvent {
  fl::events::BattleEventId id{};
  fl::events::BattleEvent ev{};
};

class BattleBusRecorder {
public:
  void attach(fl::events::BattleBus &bus) {
    // Record every event id we care about.
    (void)bus.add_listener(
        fl::events::BattleEventId::StartCombat,
        [this](const fl::events::BattleEvent &ev) { record(ev); });
    (void)bus.add_listener(
        fl::events::BattleEventId::Tick,
        [this](const fl::events::BattleEvent &ev) { record(ev); });
    (void)bus.add_listener(
        fl::events::BattleEventId::EndCombat,
        [this](const fl::events::BattleEvent &ev) { record(ev); });
  }

  void clear() { events_.clear(); }

  std::size_t size() const { return events_.size(); }
  bool empty() const { return events_.empty(); }

  const std::vector<RecordedBattleEvent> &events() const { return events_; }

  // Convenience helpers for tests
  std::vector<fl::events::BattleEventId> ids() const {
    std::vector<fl::events::BattleEventId> out;
    out.reserve(events_.size());
    for (auto const &e : events_)
      out.push_back(e.id);
    return out;
  }

  const RecordedBattleEvent &at(std::size_t i) const { return events_.at(i); }

private:
  void record(const fl::events::BattleEvent &ev) {
    events_.push_back(RecordedBattleEvent{ev.id, ev});
  }

  std::vector<RecordedBattleEvent> events_{};
};

} // namespace fl::test_support
