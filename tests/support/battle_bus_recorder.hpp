#pragma once

#include <chrono>
#include <vector>

#include "fl/events/battle_bus.hpp" // adjust include path

namespace fl::test_support {

struct RecordedBattleEvent {
  fl::events::BattleEvent ev{};
};

class BattleBusRecorder {
public:
  void attach(fl::events::BattleBus &bus) {
    (void)bus.on<fl::events::StartCombat>(
        [this](const fl::events::StartCombat &ev) {
          record(fl::events::BattleEvent{ev});
        });
    (void)bus.on<fl::events::BattleTick>(
        [this](const fl::events::BattleTick &ev) {
      record(fl::events::BattleEvent{ev});
    });
    (void)bus.on<fl::events::EndCombat>(
        [this](const fl::events::EndCombat &ev) {
          record(fl::events::BattleEvent{ev});
        });
  }

  void clear() { events_.clear(); }

  std::size_t size() const { return events_.size(); }
  bool empty() const { return events_.empty(); }

  const std::vector<RecordedBattleEvent> &events() const { return events_; }

  // Convenience helpers for tests
  const RecordedBattleEvent &at(std::size_t i) const { return events_.at(i); }

private:
  void record(const fl::events::BattleEvent &ev) {
    events_.push_back(RecordedBattleEvent{ev});
  }

  std::vector<RecordedBattleEvent> events_{};
};

} // namespace fl::test_support
