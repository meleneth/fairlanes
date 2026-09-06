#include "fl/primitives/raid_statistics.hpp"
namespace fl::primitives {
RaidStatistics::RaidStatistics(fl::events::RaidBus &bus, entt::entity account, bool enabled)
    : bus_(bus), account_(account) {
  if (!enabled) return;
  loot_sub_ = fl::events::ScopedRaidListener{bus_, std::in_place_type<fl::events::RaidLootAwarded>,
      [this](const auto &event) {
        if (event.account != account_ || event.id != last_started_ || event.id <= last_awarded_) return;
        last_awarded_ = event.id;
        records_.trinkets_awarded += event.items;
      }};
  start_sub_ = fl::events::ScopedRaidListener{bus_, std::in_place_type<fl::events::RaidStarted>,
      [this](const auto &event) { started(event); }};
  resolved_sub_ = fl::events::ScopedRaidListener{bus_, std::in_place_type<fl::events::RaidResolved>,
      [this](const auto &event) { resolved(event); }};
}
void RaidStatistics::started(const fl::events::RaidStarted &event) {
  if (event.account != account_ || event.id <= last_started_) return;
  last_started_ = event.id;
  ++records_.attempts;
  records_.party_participations += event.parties;
}
void RaidStatistics::resolved(const fl::events::RaidResolved &event) {
  if (event.account != account_ || event.id != last_started_ || event.id <= last_resolved_) return;
  last_resolved_ = event.id;
  if (event.result == fl::events::RaidResult::Victory) {
    ++records_.wins;
    records_.party_victory_credits += event.parties;
  } else {
    ++records_.defeats;
    if (event.result == fl::events::RaidResult::MutualDestruction) ++records_.mutual_destructions;
  }
  records_.combat_beats += event.combat_beats;
  // Downstream evaluation observes committed records, independent of listener order.
  bus_.emit(fl::events::RaidRecorded{event});
}
} // namespace fl::primitives
