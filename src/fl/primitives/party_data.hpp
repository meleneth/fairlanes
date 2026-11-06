// party_data.hpp
#pragma once

#include <array>
#include <cstddef>
#include <deque>
#include <functional> // std::invoke
#include <iterator>
#include <list>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <entt/entity/registry.hpp>

#include "fl/context.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/fsm/party_loop_machine.hpp"
#include "fl/fwd.hpp"
#include "fl/primitives/encounter_data.hpp"
#include "fl/primitives/encounter_mode.hpp"
#include "fl/primitives/farming_plan.hpp"
#include "fl/primitives/world_clock.hpp"
#include "fl/skills/skill.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "member_data.hpp"
#include "sr/beat_bus.hpp"

namespace fl::primitives {

class PartyRoster {
  template <class BaseIterator, class Reference, class Pointer> class IndirectIterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = MemberData;
    using difference_type = std::ptrdiff_t;
    using reference = Reference;
    using pointer = Pointer;

    IndirectIterator() = default;
    explicit IndirectIterator(BaseIterator iterator) : iterator_(iterator) {}
    reference operator*() const {
      return **iterator_;
    }
    pointer operator->() const {
      return *iterator_;
    }
    IndirectIterator& operator++() {
      ++iterator_;
      return *this;
    }
    IndirectIterator operator++(int) {
      auto copy = *this;
      ++*this;
      return copy;
    }
    friend bool operator==(const IndirectIterator&, const IndirectIterator&) = default;

  private:
    BaseIterator iterator_{};
  };

public:
  using iterator = IndirectIterator<std::vector<MemberData*>::iterator, MemberData&, MemberData*>;
  using const_iterator = IndirectIterator<std::vector<MemberData*>::const_iterator,
                                          const MemberData&, const MemberData*>;

  iterator begin() noexcept {
    return iterator{members_.begin()};
  }
  iterator end() noexcept {
    return iterator{members_.end()};
  }
  const_iterator begin() const noexcept {
    return const_iterator{members_.begin()};
  }
  const_iterator end() const noexcept {
    return const_iterator{members_.end()};
  }
  bool empty() const noexcept {
    return members_.empty();
  }
  std::size_t size() const noexcept {
    return members_.size();
  }
  MemberData& front() noexcept {
    return *members_.front();
  }
  const MemberData& front() const noexcept {
    return *members_.front();
  }
  MemberData& back() noexcept {
    return *members_.back();
  }
  const MemberData& back() const noexcept {
    return *members_.back();
  }
  MemberData& operator[](std::size_t index) noexcept {
    return *members_[index];
  }
  const MemberData& operator[](std::size_t index) const noexcept {
    return *members_[index];
  }
  void add(MemberData& member) {
    members_.push_back(&member);
  }
  bool remove(entt::entity member);
  bool contains(entt::entity member) const noexcept;

private:
  std::vector<MemberData*> members_;
};

struct PartyData {
public:
  static constexpr int kTownPenaltySeconds = 60;
  static constexpr int kTownPenaltyBeats =
      fl::primitives::WorldClock::beats_from_seconds(kTownPenaltySeconds);

  explicit PartyData(entt::entity party_id, fl::context::AccountCtx& account_ctx, std::string name);
  ~PartyData();

  PartyData(PartyData&&) = delete;
  PartyData& operator=(PartyData&&) = delete;

  PartyData(const PartyData&) = delete;
  PartyData& operator=(const PartyData&) = delete;

  // ---- accessors ----
  entt::entity party_id() const noexcept {
    return party_id_;
  }
  std::string_view name() const noexcept {
    return name_;
  }

  fl::widgets::FancyLog& log() const {
    return *log_;
  }
  bool has_log() const noexcept {
    return static_cast<bool>(log_);
  }

  PartyRoster& members() noexcept {
    return members_;
  }
  const PartyRoster& members() const noexcept {
    return members_;
  }

  fl::primitives::EncounterData& encounter_data() {
    return raid_encounter_ ? *raid_encounter_ : *encounter_data_;
  }
  const fl::primitives::EncounterData& encounter_data() const {
    return raid_encounter_ ? *raid_encounter_ : *encounter_data_;
  }

  fl::events::PartyBus& party_bus() noexcept {
    return party_bus_;
  }
  const fl::events::PartyBus& party_bus() const noexcept {
    return party_bus_;
  }

  fl::context::PartyCtx& loop_ctx() noexcept {
    return party_ctx_;
  }
  const fl::context::PartyCtx& loop_ctx() const noexcept {
    return party_ctx_;
  }

  fl::context::PartyCtx& party_ctx() noexcept {
    return party_ctx_;
  }
  const fl::context::PartyCtx& party_ctx() const noexcept {
    return party_ctx_;
  }

  fl::fsm::PartyLoopMachine& loop_machine() {
    return *party_loop_machine_;
  }
  const fl::fsm::PartyLoopMachine& loop_machine() const {
    return *party_loop_machine_;
  }
  EncounterData& create_encounter();
  bool has_encounter() const noexcept {
    return raid_encounter_ != nullptr || encounter_data_ != nullptr;
  }
  bool in_combat() const noexcept {
    return has_encounter() && !encounter_data().is_over();
  }
  void preserve_terminal_encounter(bool enabled) noexcept {
    preserve_terminal_encounter_ = enabled;
  }
  bool preserves_terminal_encounter() const noexcept {
    return preserve_terminal_encounter_;
  }
  struct TowerRoute {
    std::string_view biome_id;
    int floor;
  };
  static constexpr std::array<std::string_view, 5> kOriginBiomeIds{
      "origin_woodland_tower", "origin_plains_tower", "origin_murky_swamp_tower",
      "origin_caves_tower", "origin_desert_tower"};
  std::optional<TowerRoute> tower_route() const noexcept;
  bool select_tower(std::string_view biome_id, std::optional<int> floor = std::nullopt);
  int latest_tower_floor(std::string_view biome_id) const noexcept;
  int highest_tower_floor_cleared(std::string_view biome_id) const noexcept;
  void begin_tower_encounter();
  bool tower_encounter_ready() const noexcept {
    return !encounter_start_blocked_ && !tower_complete_waiting_;
  }
  bool at_encounter_boundary() const noexcept {
    return !has_encounter() && !in_raid();
  }
  void block_encounter_start(bool blocked) noexcept {
    encounter_start_blocked_ = blocked;
  }
  bool encounter_start_blocked() const noexcept {
    return encounter_start_blocked_ || origin_auto_progression_blocked_;
  }
  void block_origin_auto_progression(bool blocked) noexcept {
    origin_auto_progression_blocked_ = blocked;
  }
  bool plains_unlocked() const noexcept {
    return biome_unlocked("origin_plains_tower");
  }
  bool biome_unlocked(std::string_view biome_id) const noexcept;
  std::size_t unlocked_origin_biome_count() const noexcept;
  bool tower_choice_pending() const noexcept {
    return tower_choice_pending_;
  }
  int tower_floor_attempts() const noexcept {
    return tower_floor_attempts_;
  }
  // ---- behavior ----
  void advance_beat();
  entt::entity recruit_member(std::string name);
  entt::entity recruit_origin_member(std::string name);
  bool story_shop_pending() const noexcept {
    return story_shop_pending_;
  }
  void begin_story_shop() noexcept {
    story_shop_pending_ = true;
  }
  void finish_story_shop() noexcept {
    story_shop_pending_ = false;
  }
  int darrin_marks() const noexcept {
    return darrin_marks_;
  }
  void award_darrin_marks(int amount) noexcept {
    darrin_marks_ += amount;
  }
  bool spend_darrin_marks(int amount) noexcept {
    if (amount < 0 || darrin_marks_ < amount) return false;
    darrin_marks_ -= amount;
    return true;
  }
  void hook_to_beat(seerin::BeatBus& gc_beat_bus);

  bool needs_town();
  bool all_members_dead() const;
  void revitalize_members();
  void start_town_penalty();
  void leave_combat();
  void return_to_town();
  // Call between combat ticks, before enrollment into the account raid.
  void summon_to_raid();
  bool in_raid() const noexcept {
    return raid_encounter_ != nullptr;
  }
  void join_raid(EncounterData& encounter);
  void resolve_raid(bool victory);
  void detach_raid() noexcept {
    raid_encounter_ = nullptr;
  }
  void watch_skill_learned_this_combat(entt::entity member, fl::skills::SkillKey skill);
  bool drop_skill(entt::entity member, std::size_t slot_index);
  struct TeacherEncounter {
    std::string biome_id;
    int floor{0};
    fl::monster::MonsterKind monster;
  };
  std::optional<TeacherEncounter> find_accessible_teacher(entt::entity member,
                                                          fl::skills::SkillKey skill) const;
  bool farm_for_forgotten_skill(entt::entity member, fl::skills::SkillKey skill);

  GrimoireDiscipline grimoire_discipline() const noexcept {
    return grimoire_discipline_;
  }
  void select_grimoire_discipline(GrimoireDiscipline discipline);
  const FarmingPlan& farming_plan() const noexcept {
    return farming_plan_;
  }
  ProgressionControlMode progression_control_mode() const noexcept {
    return progression_control_mode_;
  }
  void set_progression_control_mode(ProgressionControlMode mode) noexcept {
    progression_control_mode_ = mode;
  }
  std::optional<FarmFocus> previous_farm_focus() const noexcept {
    return previous_farm_focus_;
  }
  FarmingChoiceAdvice farming_choice_advice() const noexcept;
  bool farm_focus_selected() const noexcept {
    return farm_focus_selected_;
  }
  bool needs_farm_focus_choice() const noexcept {
    return encounter_mode_ == EncounterMode::HeroesJourney && !farm_focus_selected_;
  }
  EncounterMode encounter_mode() const noexcept {
    return encounter_mode_;
  }
  void set_encounter_mode(EncounterMode mode) noexcept {
    encounter_mode_ = mode;
  }
  void apply_recommended_farming_plan();
  void select_farming_plan(GrimoireDiscipline discipline, FarmFocus focus);

  bool town_penalty_active() const noexcept {
    return town_penalty_beats_remaining_ > 0;
  }

  int town_penalty_beats_remaining() const noexcept {
    return town_penalty_beats_remaining_;
  }

  void tick_town_penalty();

  void add_party_member(entt::entity member) {
    (void)member;
  }

  template <class Fn> void for_each_member(Fn&& fn) const {
    for (const auto& member : members_) {
      std::invoke(std::forward<Fn>(fn), member.member_id());
    }
  }

  void add_item(entt::entity item);
  std::span<const entt::entity> items() const noexcept;
  void replace_items(std::vector<entt::entity> items);

private:
  entt::entity party_id_{entt::null};
  std::string name_;
  entt::entity account_id_{entt::null};
  std::unique_ptr<fl::widgets::FancyLog> log_;
  fl::events::PartyBus party_bus_;
  fl::context::PartyCtx party_ctx_;
  std::unique_ptr<fl::fsm::PartyLoopMachine> party_loop_machine_;
  std::unique_ptr<fl::primitives::EncounterData> encounter_data_{nullptr};
  EncounterData* raid_encounter_{nullptr}; // borrowed from account RaidData
  PartyRoster members_;
  seerin::AtbEngine atb_;
  std::vector<entt::entity> inventory_;
  GrimoireDiscipline grimoire_discipline_{GrimoireDiscipline::Brawn};
  FarmingPlan farming_plan_{make_farming_plan(GrimoireDiscipline::Brawn, FarmFocus::Brawn)};
  ProgressionControlMode progression_control_mode_{ProgressionControlMode::Manual};
  std::optional<FarmFocus> previous_farm_focus_;
  std::optional<FarmingPlan> pending_farming_plan_;
  bool farm_focus_selected_{false};
  EncounterMode encounter_mode_{EncounterMode::ChaosAttractor};
  std::string tower_biome_{};
  std::string encounter_tower_biome_{};
  int encounter_tower_floor_{0};
  struct OriginTowerState {
    std::string_view biome_id;
    int selected_floor{1};
    int latest_floor{1};
    int cleared_floor{0};
    bool unlocked{false};
  };
  std::array<OriginTowerState, 5> origin_towers_{{
      {kOriginBiomeIds[0], 1, 1, 0, true},
      {kOriginBiomeIds[1], 1, 1, 0, false},
      {kOriginBiomeIds[2], 1, 1, 0, false},
      {kOriginBiomeIds[3], 1, 1, 0, false},
      {kOriginBiomeIds[4], 1, 1, 0, false},
  }};
  OriginTowerState* tower_state(std::string_view biome_id) noexcept;
  const OriginTowerState* tower_state(std::string_view biome_id) const noexcept;
  void unlock_next_origin_biome(std::string_view cleared_biome);
  int tower_floor_attempts_{0};
  bool tower_choice_pending_{false};
  bool story_shop_pending_{false};
  int darrin_marks_{0};
  bool plains_reward_awarded_{false};
  struct PendingTowerDestination {
    std::string biome_id;
    int floor{1};
    bool repeat{false};
  };
  std::optional<PendingTowerDestination> pending_tower_destination_;
  bool repeat_selected_floor_{false};
  bool encounter_start_blocked_{false};
  bool origin_auto_progression_blocked_{false};
  bool tower_complete_waiting_{false};

  seerin::BeatBus party_beat_bus_{};
  seerin::BeatSub gc_forward_sub_{};
  fl::events::ScopedPartyListener town_revitalize_sub_{};
  fl::events::ScopedPartyListener loot_drop_sub_{};
  fl::events::ScopedPartyListener manual_equipment_sub_{};
  fl::events::ScopedPartyListener tower_progress_sub_{};
  int town_penalty_beats_remaining_{0};

  struct PendingLearnedSkill {
    entt::entity member{entt::null};
    fl::skills::SkillKey skill{fl::skills::SkillId::Observe};
    fl::events::ScopedPartyListener wipe_sub{};
    fl::events::ScopedPartyListener victory_sub{};
    fl::events::ScopedPartyListener returned_to_town_sub{};
    fl::events::ScopedPartyListener summoned_sub{};
    fl::events::ScopedPartyListener raid_resolved_sub{};
  };

  std::list<PendingLearnedSkill> pending_learned_skills_;
  bool leaving_combat_{false};
  bool preserve_terminal_encounter_{false};
  void cleanup_encounter();
  void commit_tower_victory();
  bool request_tower_destination(std::string_view biome_id, int floor, bool repeat);
  bool commit_tower_destination(std::string_view biome_id, int floor, bool repeat);
  void apply_pending_tower_destination();
  void commit_farming_plan(FarmingPlan plan);
  void apply_pending_farming_plan();
  void clear_combat_visuals();
  void restore_observe_to_open_first_slots();

  void resolve_pending_learned_skill(std::list<PendingLearnedSkill>::iterator it, bool keep_skill);
};

} // namespace fl::primitives
