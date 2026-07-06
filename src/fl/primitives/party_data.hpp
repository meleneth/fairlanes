// party_data.hpp
#pragma once

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

#include <entt/entt.hpp>

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

  fl::primitives::EncounterData &encounter_data() { return *encounter_data_; }
  const fl::primitives::EncounterData &encounter_data() const {
    return *encounter_data_;
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
  bool has_encounter() const noexcept { return encounter_data_ != nullptr; }
  bool in_combat() const noexcept {
    return encounter_data_ != nullptr && !encounter_data_->is_over();
  }
  // ---- behavior ----
  void hook_to_beat(seerin::BeatBus &gc_beat_bus);

  bool needs_town();
  bool all_members_dead() const;
  void revitalize_members();
  void start_town_penalty();
  void leave_combat();
  void watch_skill_learned_this_combat(entt::entity member,
                                       fl::skills::SkillKey skill);

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

  seerin::BeatBus party_beat_bus_{};
  seerin::BeatSub gc_forward_sub_{};
  fl::events::ScopedPartyListener town_revitalize_sub_{};
  fl::events::ScopedPartyListener loot_drop_sub_{};
  int town_penalty_beats_remaining_{0};

  struct PendingLearnedSkill {
    entt::entity member{entt::null};
    fl::skills::SkillKey skill{fl::skills::SkillId::Observe};
    fl::events::ScopedPartyListener wipe_sub{};
    fl::events::ScopedPartyListener victory_sub{};
  };

  std::list<PendingLearnedSkill> pending_learned_skills_;

  void
  resolve_pending_learned_skill(std::list<PendingLearnedSkill>::iterator it,
                                bool keep_skill);
};

} // namespace fl::primitives
