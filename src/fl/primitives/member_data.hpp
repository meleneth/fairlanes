// grand_central.hpp
#pragma once

#include <string>
#include <utility>

#include <entt/entt.hpp>

#include "fl/events/party_bus.hpp"
#include "fl/skills/grimoire.hpp"

namespace fl::primitives {

struct MemberData {
public:
  explicit MemberData(entt::entity member_id, std::string name)
      : member_id_{member_id}, name_{std::move(name)} {
    grimoire_.learn(fl::skills::SkillId::Observe);
  }

  MemberData(MemberData &&) noexcept = default;
  MemberData &operator=(MemberData &&) noexcept = default;

  MemberData(const MemberData &) = delete;
  MemberData &operator=(const MemberData &) = delete;

  // ---- accessors ----
  entt::entity member_id() const noexcept { return member_id_; }

  const std::string &name() const noexcept { return name_; }

  fl::skills::Grimoire &grimoire() noexcept { return grimoire_; }
  const fl::skills::Grimoire &grimoire() const noexcept { return grimoire_; }

  fl::events::PartyBus &bus() noexcept { return bus_; }
  const fl::events::PartyBus &bus() const noexcept { return bus_; }

  fl::events::CombatantBus &combatant_bus() noexcept { return combatant_bus_; }
  const fl::events::CombatantBus &combatant_bus() const noexcept {
    return combatant_bus_;
  }

  void hook_level_progression(entt::registry &reg);

private:
  entt::entity member_id_{entt::null};
  std::string name_;
  fl::skills::Grimoire grimoire_;

  fl::events::PartyBus bus_;
  fl::events::CombatantBus combatant_bus_;
  fl::events::ScopedPartyListener level_gained_sub_{};
};

} // namespace fl::primitives
