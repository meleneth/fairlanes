#include "fl/primitives/discovery_journal.hpp"

#include <algorithm>

#include "fl/ecs/components/monster_identity.hpp"
#include "fl/generated/monster_content.hpp"

namespace fl::primitives {

void DiscoveryJournal::encounter(fl::monster::MonsterKind kind) {
  monsters_.try_emplace(kind);
}

void DiscoveryJournal::witness(
    fl::skills::SkillId skill,
    std::optional<fl::monster::MonsterKind> monster) {
  skills_.insert(skill);
  if (monster) {
    const auto declared =
        fl::monster::generated_content::known_skills(*monster);
    if (std::ranges::find(declared, skill) != declared.end()) {
      monsters_[*monster].insert(skill);
    }
  }
}

bool DiscoveryJournal::knows(fl::skills::SkillId skill) const {
  return skills_.contains(skill);
}

std::vector<std::optional<fl::skills::SkillId>>
DiscoveryJournal::skill_slots(fl::monster::MonsterKind kind) const {
  std::vector<std::optional<fl::skills::SkillId>> slots;
  const auto found = monsters_.find(kind);
  if (found == monsters_.end()) {
    return slots;
  }
  for (auto skill : fl::monster::generated_content::known_skills(kind)) {
    slots.push_back(found->second.contains(skill) ? std::optional{skill}
                                                  : std::nullopt);
  }
  return slots;
}

void DiscoveryJournalListener::bind(fl::events::PartyBus &bus) {
  listeners_.emplace_back(bus,
                          std::in_place_type<fl::events::MonsterEncountered>,
                          [this](const fl::events::MonsterEncountered &event) {
                            journal_.encounter(event.kind);
                          });
  listeners_.emplace_back(
      bus, std::in_place_type<fl::events::SkillWitnessed>,
      [this](const fl::events::SkillWitnessed &event) {
        const auto *identity =
            reg_.try_get<fl::ecs::components::MonsterIdentity>(event.user);
        journal_.witness(event.skill.base, identity
                                               ? std::optional{identity->kind}
                                               : std::nullopt);
      });
}

} // namespace fl::primitives
