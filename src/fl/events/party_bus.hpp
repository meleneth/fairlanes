#pragma once

#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>

#include <functional>
#include <cstdint>
#include <string>
#include <utility>
#include <variant>

#include "fl/loot/equipment_slot.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/primitives/cycle.hpp"
#include "fl/resources/combat_resource.hpp"
#include "fl/skills/skill.hpp"
#include "fl/consumables/consumable_kind.hpp"
#include "fl/economy/material.hpp"
#include "fl/economy/profession.hpp"
#include "fl/economy/recipe.hpp"
#include "sr/variant_bus.hpp"

namespace fl::events {

struct PartyCreated {};
struct MemberJoined {
  entt::entity member{entt::null};
};
struct PartyWiped {};
struct PartyVictory {};
struct PartyEncounterBoundary {
  entt::entity party{entt::null};
};
struct TowerFloorCleared {
  std::string biome_id;
  int floor{1};
};
struct TowerCompleted {
  std::string biome_id;
};
struct TowerDestinationRequested {
  std::string biome_id;
  int floor{1};
};
struct TowerDestinationChanged {
  std::string biome_id;
  int floor{1};
};
struct FarmingTargetRequested {
  std::string focus;
};
struct FarmingTargetChanged {
  std::string focus;
};
struct RosterTransferRequested {
  entt::entity member{entt::null};
  entt::entity source_party{entt::null};
  entt::entity destination_party{entt::null};
};
struct RosterTransferCommitted {
  entt::entity member{entt::null};
  entt::entity source_party{entt::null};
  entt::entity destination_party{entt::null};
};
struct RecruitmentOpportunityUnlocked {
  std::string opportunity_id;
  std::string biome_id;
  int floor{0};
  std::size_t destination_party{0};
};
struct HeroRecruited {
  std::string opportunity_id;
  std::size_t party_index{0};
  entt::entity member{entt::null};
};
struct PartyRosterCompleted {
  std::size_t party_index{0};
};
struct PartyUnlocked {
  std::size_t party_index{0};
};
struct BiomeUnlocked {
  std::string biome_id;
};
struct CycleUnlocked {
  fl::primitives::Cycle cycle;
};
// A normal encounter is fully populated and ready for combat ticks.
struct PartyCombatStarted {};
struct PartyLeftCombat {};
struct PartyReturnToTownRequested {};
struct PartyReturnedToTown {};
// Extraction preserves observation learning without claiming combat victory.
struct PartySummonedToRaid {};
struct PartyRaidResolved {
  bool victory{false};
};
struct PartyGainedXP {
  entt::entity member{entt::null};
  int amount{0};
};
struct PartyGainedLevel {
  entt::entity member{entt::null};
  int level{1};
};
struct PartyHealed {
  entt::entity member{entt::null};
  int amount{0};
};
struct PartyRevitalizeRequested {};
struct LootDropRequested {
  entt::entity source{entt::null};
  entt::entity party{entt::null};
};
struct PartyTick {};
struct MonsterEncountered {
  fl::monster::MonsterKind kind;
};
struct SkillWitnessed {
  entt::entity user{entt::null};
  fl::skills::SkillKey skill{fl::skills::SkillId::Observe};
};
struct EquipmentEquipRequested {
  entt::entity member{entt::null};
  fl::loot::EquipmentSlot slot{fl::loot::EquipmentSlot::chest};
  entt::entity item{entt::null};
};
struct EquipmentEquipped {
  entt::entity member{entt::null};
  fl::loot::EquipmentSlot slot{fl::loot::EquipmentSlot::chest};
  entt::entity item{entt::null};
  entt::entity previous{entt::null};
};
struct PartyActivityChanged {
  std::string activity;
};
struct MaterialGathered {
  entt::entity worker{entt::null};
  fl::economy::Material material{fl::economy::Material::WoodlandHerb};
  int quantity{0};
  int duration_beats{0};
};
struct MaterialsConsumed {
  fl::economy::RecipeId recipe{fl::economy::RecipeId::HealingPotion};
  int quantity{0};
};
struct ProductCrafted {
  entt::entity worker{entt::null};
  fl::economy::RecipeId recipe{fl::economy::RecipeId::HealingPotion};
  entt::entity item{entt::null};
};
struct ProfessionExperienceGained {
  entt::entity worker{entt::null};
  fl::economy::Profession profession{fl::economy::Profession::Herbalism};
  int amount{0};
};
struct SkillStudyCompleted {
  entt::entity student{entt::null};
  fl::skills::SkillKey skill{fl::skills::SkillId::Observe};
};
struct ProductionBlocked {
  std::string reason;
};

using PartyEvent = std::variant<
    PartyCreated, MemberJoined, PartyWiped, PartyVictory, PartyEncounterBoundary, TowerFloorCleared,
    TowerCompleted, TowerDestinationRequested, TowerDestinationChanged, FarmingTargetRequested,
    FarmingTargetChanged, RosterTransferRequested, RosterTransferCommitted,
    RecruitmentOpportunityUnlocked, HeroRecruited, PartyRosterCompleted, PartyUnlocked,
    BiomeUnlocked, CycleUnlocked, PartyCombatStarted, PartyLeftCombat, PartyReturnToTownRequested,
    PartyReturnedToTown, PartySummonedToRaid, PartyRaidResolved, PartyGainedXP, PartyGainedLevel,
    PartyHealed, PartyRevitalizeRequested, LootDropRequested, PartyTick, MonsterEncountered,
    SkillWitnessed, EquipmentEquipRequested, EquipmentEquipped, PartyActivityChanged,
    MaterialGathered, MaterialsConsumed, ProductCrafted, ProfessionExperienceGained,
    SkillStudyCompleted, ProductionBlocked>;

using PartyBus = seerin::VariantBus<PartyEvent>;

struct PoisonApplied {
  entt::entity source{entt::null};
  entt::entity target{entt::null};
  int damage_per_tick{0};
  int duration_seconds{0};
};
struct FreezeApplied {
  entt::entity source{entt::null};
  entt::entity target{entt::null};
  int duration_seconds{0};
};
struct FreezeStarted {
  entt::entity target{entt::null};
};
struct FreezeEnded {
  entt::entity target{entt::null};
};
struct DireBleedApplied {
  entt::entity source{entt::null};
  entt::entity target{entt::null};
  int damage_per_tick{0};
};
struct DireBleedTicked {
  entt::entity source{entt::null};
  entt::entity target{entt::null};
  int damage{0};
  int hp_after{0};
  bool killed{false};
};
struct DireBleedCleared {
  entt::entity target{entt::null};
};
struct BraceMitigated {
  entt::entity source{entt::null};
  entt::entity target{entt::null};
  int damage{0};
};
struct SkillHitLanded {
  entt::entity source{entt::null};
  entt::entity target{entt::null};
  fl::skills::SkillKey skill{fl::skills::SkillId::Observe};
  int damage{0};
};
struct SkillCastStarted {
  entt::entity caster{entt::null};
  fl::skills::SkillKey skill{fl::skills::SkillId::Observe};
};
struct SkillActionSelected {
  entt::entity actor{entt::null};
  entt::entity target{entt::null};
  fl::skills::SkillKey skill{fl::skills::SkillId::Observe};
};
struct SkillCastFinished {
  entt::entity caster{entt::null};
};
struct ResourceChanged {
  entt::entity actor{entt::null};
  fl::resources::CombatResource resource{fl::resources::CombatResource::Adrenaline};
  fl::resources::ResourceChangeReason reason{fl::resources::ResourceChangeReason::SkillCost};
  int before{0};
  int after{0};
  fl::skills::SkillKey skill{fl::skills::SkillId::Observe};
};
struct SkillResourceRejected {
  entt::entity actor{entt::null};
  fl::skills::SkillKey skill{fl::skills::SkillId::Observe};
  fl::resources::CombatResource resource{fl::resources::CombatResource::Adrenaline};
  int required{0};
  int available{0};
};
struct ConsumableUsed {
  entt::entity actor{entt::null};
  entt::entity item{entt::null};
  fl::consumables::ConsumableKind kind{fl::consumables::ConsumableKind::HealingPotion};
  int quantity_after{0};
  int effective_amount{0};
  int wasted_amount{0};
};
struct TimedBuffApplied {
  entt::entity actor{entt::null};
  fl::consumables::TimedBuffKind kind{fl::consumables::TimedBuffKind::Attack};
  int magnitude{0};
  std::uint64_t expires_world_beat{0};
  bool replaced{false};
};
struct TimedBuffExpired {
  entt::entity actor{entt::null};
  fl::consumables::TimedBuffKind kind{fl::consumables::TimedBuffKind::Attack};
  int magnitude{0};
};
struct FleeAttempted {
  entt::entity source{entt::null};
  int chance_percent{0};
  int roll{0};
  bool success{false};
};
struct CombatantFled {
  entt::entity source{entt::null};
};
struct PlayerDied {
  entt::entity player{entt::null};
  entt::entity killer{entt::null};
};

using CombatantEvent =
    std::variant<PoisonApplied, FreezeApplied, FreezeStarted, FreezeEnded, DireBleedApplied,
                 DireBleedTicked, DireBleedCleared, BraceMitigated, SkillHitLanded,
                 SkillCastStarted, SkillActionSelected, SkillCastFinished, FleeAttempted,
                 ResourceChanged, SkillResourceRejected, ConsumableUsed, TimedBuffApplied,
                 TimedBuffExpired, CombatantFled, PlayerDied>;

using CombatantBus = seerin::VariantBus<CombatantEvent>;

template <class Bus> struct ScopedListener {
  std::function<void()> disconnect_;
  bool connected_{false};

  ScopedListener() = default;

  template <class T, class Fn> ScopedListener(Bus& bus, std::in_place_type_t<T>, Fn&& fn) {
    auto handle = bus.template on<T>(std::forward<Fn>(fn));
    disconnect_ = [&bus, handle]() mutable { bus.template callbacks<T>().remove(handle); };
    connected_ = true;
  }

  ScopedListener(ScopedListener&& rhs) noexcept {
    *this = std::move(rhs);
  }
  ScopedListener& operator=(ScopedListener&& rhs) noexcept {
    if (this == &rhs) return *this;
    reset();
    disconnect_ = std::move(rhs.disconnect_);
    connected_ = rhs.connected_;
    rhs.connected_ = false;
    return *this;
  }

  ~ScopedListener() {
    reset();
  }

  void reset() {
    if (connected_ && disconnect_) {
      disconnect_();
    }
    connected_ = false;
    disconnect_ = nullptr;
  }

  ScopedListener(const ScopedListener&) = delete;
  ScopedListener& operator=(const ScopedListener&) = delete;
};

using ScopedPartyListener = ScopedListener<PartyBus>;
using ScopedCombatantListener = ScopedListener<CombatantBus>;

} // namespace fl::events
