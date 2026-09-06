#pragma once
#include "fl/events/raid_bus.hpp"
#include <unordered_map>
#include <vector>

namespace fl::events {
enum class AchievementId { RaidMutualDestruction };
struct AchievementUnlocked {
  AchievementId achievement;
  entt::entity account;
  std::uint64_t raid_id;
};
using AchievementBus = seerin::VariantBus<std::variant<AchievementUnlocked>>;
using ScopedAchievementListener = ScopedListener<AchievementBus>;
}
namespace fl::primitives {
struct SaveRaidRecords {
  std::uint64_t completed{0}, wins{0}, defeats{0}, mutual_destructions{0};
};
// Save-wide scope for the current session. Persistence is a separate milestone.
class RaidMilestones {
public:
  void bind(fl::events::RaidBus &bus, entt::entity account);
  const SaveRaidRecords &records() const { return records_; }
  bool mutual_destruction_unlocked() const { return mutual_destruction_; }
  fl::events::AchievementBus &bus() { return bus_; }
private:
  void recorded(const fl::events::RaidRecorded &event);
  SaveRaidRecords records_;
  bool mutual_destruction_{false};
  std::unordered_map<entt::entity, std::uint64_t> last_recorded_;
  fl::events::AchievementBus bus_;
  std::vector<fl::events::ScopedRaidListener> subscriptions_;
};
}
