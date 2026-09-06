#include "fl/primitives/raid_milestones.hpp"
namespace fl::primitives {
void RaidMilestones::bind(fl::events::RaidBus &bus, entt::entity account) {
  if (!last_recorded_.emplace(account, 0).second) return;
  subscriptions_.emplace_back(bus, std::in_place_type<fl::events::RaidRecorded>,
      [this, account](const auto &event) {
        if (event.outcome.account == account) recorded(event);
      });
}
void RaidMilestones::recorded(const fl::events::RaidRecorded &event) {
  const auto &outcome = event.outcome;
  auto &last = last_recorded_.at(outcome.account);
  if (outcome.id <= last) return;
  last = outcome.id;
  ++records_.completed;
  if (outcome.result == fl::events::RaidResult::Victory) ++records_.wins;
  else ++records_.defeats;
  if (outcome.result != fl::events::RaidResult::MutualDestruction) return;
  ++records_.mutual_destructions;
  if (mutual_destruction_) return;
  mutual_destruction_ = true;
  bus_.emit(fl::events::AchievementUnlocked{
      fl::events::AchievementId::RaidMutualDestruction, outcome.account, outcome.id});
}
}
