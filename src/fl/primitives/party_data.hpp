// party_data.hpp
#pragma once

#include <cstddef>
#include <deque>
#include <functional> // std::invoke
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

struct PartyData {
public:
  static constexpr int kTownPenaltySeconds = 60;
  static constexpr int kTownPenaltyBeats =
      fl::primitives::WorldClock::beats_from_seconds(kTownPenaltySeconds);

  explicit PartyData(entt::entity party_id,
                     fl::context::AccountCtx &account_ctx, std::string name);
  ~PartyData();

  PartyData(PartyData &&) = delete;
  PartyData &operator=(PartyData &&) = delete;

  PartyData(const PartyData &) = delete;
  PartyData &operator=(const PartyData &) = delete;

  // ---- accessors ----
  entt::entity party_id() const noexcept { return party_id_; }
  std::string_view name() const noexcept { return name_; }

  fl::widgets::FancyLog &log() const { return *log_; }
  bool has_log() const noexcept { return static_cast<bool>(log_); }

  std::deque<fl::primitives::MemberData> &members() noexcept {
    return members_;
  }
  const std::deque<fl::primitives::MemberData> &members() const noexcept {
    return members_;
  }

  fl::primitives::EncounterData &encounter_data() { return raid_encounter_ ? *raid_encounter_ : *encounter_data_; }
  const fl::primitives::EncounterData &encounter_data() const {
    return raid_encounter_ ? *raid_encounter_ : *encounter_data_;
  }

  fl::events::PartyBus &party_bus() noexcept { return party_bus_; }
  const fl::events::PartyBus &party_bus() const noexcept { return party_bus_; }

  fl::context::PartyCtx &loop_ctx() noexcept { return party_ctx_; }
  const fl::context::PartyCtx &loop_ctx() const noexcept { return party_ctx_; }

  fl::context::PartyCtx &party_ctx() noexcept { return party_ctx_; }
  const fl::context::PartyCtx &party_ctx() const noexcept { return party_ctx_; }

  fl::fsm::PartyLoopMachine &loop_machine() { return *party_loop_machine_; }
  const fl::fsm::PartyLoopMachine &loop_machine() const {
    return *party_loop_machine_;
  }
  EncounterData &create_encounter();
  bool has_encounter() const noexcept { return raid_encounter_ != nullptr || encounter_data_ != nullptr; }
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
  std::optional<TowerRoute> tower_route() const noexcept;
  bool select_tower(std::string_view biome_id,
                    std::optional<int> floor = std::nullopt);
  int latest_tower_floor(std::string_view biome_id) const noexcept;
  void begin_tower_encounter();
  bool plains_unlocked() const noexcept { return plains_unlocked_; }
  bool tower_choice_pending() const noexcept { return tower_choice_pending_; }
  int tower_floor_attempts() const noexcept { return tower_floor_attempts_; }
  // ---- behavior ----
  void advance_beat();
  entt::entity recruit_member(std::string name);
  bool story_shop_pending() const noexcept { return story_shop_pending_; }
  void begin_story_shop() noexcept { story_shop_pending_ = true; }
  void finish_story_shop() noexcept { story_shop_pending_ = false; }
  int darrin_marks() const noexcept { return darrin_marks_; }
  void award_darrin_marks(int amount) noexcept { darrin_marks_ += amount; }
  bool spend_darrin_marks(int amount) noexcept {
    if (amount < 0 || darrin_marks_ < amount) return false;
    darrin_marks_ -= amount;
    return true;
  }
  void hook_to_beat(seerin::BeatBus &gc_beat_bus);

  bool needs_town();
  bool all_members_dead() const;
  void revitalize_members();
  void start_town_penalty();
  void leave_combat();
  void return_to_town();
  // Call between combat ticks, before enrollment into the account raid.
  void summon_to_raid();
  bool in_raid() const noexcept { return raid_encounter_ != nullptr; }
  void join_raid(EncounterData &encounter);
  void resolve_raid(bool victory);
  void detach_raid() noexcept { raid_encounter_ = nullptr; }
  void watch_skill_learned_this_combat(entt::entity member,
                                       fl::skills::SkillKey skill);
  bool drop_skill(entt::entity member, std::size_t slot_index);

  GrimoireDiscipline grimoire_discipline() const noexcept {
    return grimoire_discipline_;
  }
  void select_grimoire_discipline(GrimoireDiscipline discipline);
  const FarmingPlan &farming_plan() const noexcept { return farming_plan_; }
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
  bool farm_focus_selected() const noexcept { return farm_focus_selected_; }
  bool needs_farm_focus_choice() const noexcept {
    return encounter_mode_ == EncounterMode::HeroesJourney &&
           !farm_focus_selected_;
  }
  EncounterMode encounter_mode() const noexcept { return encounter_mode_; }
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

  void add_party_member(entt::entity member) { (void)member; }

  template <class Fn> void for_each_member(Fn &&fn) const {
    for (const auto &member : members_) {
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
  EncounterData *raid_encounter_{nullptr}; // borrowed from account RaidData
  std::deque<fl::primitives::MemberData> members_;
  seerin::AtbEngine atb_;
  std::vector<entt::entity> inventory_;
  GrimoireDiscipline grimoire_discipline_{GrimoireDiscipline::Brawn};
  FarmingPlan farming_plan_{
      make_farming_plan(GrimoireDiscipline::Brawn, FarmFocus::Brawn)};
  ProgressionControlMode progression_control_mode_{
      ProgressionControlMode::Manual};
  std::optional<FarmFocus> previous_farm_focus_;
  bool farm_focus_selected_{false};
  EncounterMode encounter_mode_{EncounterMode::ChaosAttractor};
  std::string tower_biome_{};
  std::string encounter_tower_biome_{};
  int encounter_tower_floor_{0};
  int woodland_floor_{1};
  int plains_floor_{1};
  int woodland_latest_floor_{1};
  int plains_latest_floor_{1};
  int tower_floor_attempts_{0};
  bool plains_unlocked_{false};
  bool tower_choice_pending_{false};
  bool story_shop_pending_{false};
  int darrin_marks_{0};
  bool plains_reward_awarded_{false};

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
  void clear_combat_visuals();
  void restore_observe_to_open_first_slots();

  void
  resolve_pending_learned_skill(std::list<PendingLearnedSkill>::iterator it,
                                bool keep_skill);
};

} // namespace fl::primitives
