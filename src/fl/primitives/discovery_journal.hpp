#pragma once

#include <map>
#include <optional>
#include <set>
#include <vector>

#include "fl/events/party_bus.hpp"

namespace fl::primitives {

// Owned by the game, shared across its accounts. No character learning state.
class DiscoveryJournal {
public:
  void encounter(fl::monster::MonsterKind kind);
  void witness(fl::skills::SkillId skill,
               std::optional<fl::monster::MonsterKind> monster = std::nullopt);
  bool knows(fl::skills::SkillId skill) const;
  const auto &monsters() const noexcept { return monsters_; }
  std::vector<std::optional<fl::skills::SkillId>>
  skill_slots(fl::monster::MonsterKind kind) const;

private:
  std::map<fl::monster::MonsterKind, std::set<fl::skills::SkillId>> monsters_;
  std::set<fl::skills::SkillId> skills_;
};

// The owner must destroy this before the journal, registry, and subscribed
// buses.
class DiscoveryJournalListener {
public:
  DiscoveryJournalListener(DiscoveryJournal &journal, entt::registry &reg)
      : journal_(journal), reg_(reg) {}
  void bind(fl::events::PartyBus &bus);

private:
  DiscoveryJournal &journal_;
  entt::registry &reg_;
  std::vector<fl::events::ScopedPartyListener> listeners_;
};

} // namespace fl::primitives
