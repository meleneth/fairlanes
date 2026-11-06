#include "fl/primitives/origin_auto_progression.hpp"

#include <algorithm>
#include <string>

#include "fl/primitives/account_data.hpp"
#include "fl/primitives/party_data.hpp"

namespace fl::primitives {

OriginAutoProgression::OriginAutoProgression(fl::context::AccountCtx account,
                                             std::size_t party_index,
                                             fl::events::StoryBus& story_bus,
                                             bool initially_enabled)
    : account_(account), party_index_(party_index), story_bus_(&story_bus) {
  autoplay_sub_ = fl::events::ScopedStoryListener{
      story_bus, std::in_place_type<fl::events::AutoplayChanged>,
      [this](const fl::events::AutoplayChanged& event) { set_enabled(event.enabled); }};

  auto& bus = party().party_bus();
  party_subs_.emplace_back(bus, std::in_place_type<fl::events::PartyVictory>, [this](const auto&) {
    if (party().in_raid()) return;
    outcome_ = Outcome::Victory;
    request_evaluation();
  });
  party_subs_.emplace_back(bus, std::in_place_type<fl::events::PartyWiped>, [this](const auto&) {
    if (ignore_next_raid_wipe_) {
      ignore_next_raid_wipe_ = false;
      return;
    }
    outcome_ = Outcome::Defeat;
    request_evaluation();
  });
  party_subs_.emplace_back(bus, std::in_place_type<fl::events::PartyRaidResolved>,
                           [this](const fl::events::PartyRaidResolved& event) {
                             ignore_next_raid_wipe_ = !event.victory;
                             request_evaluation();
                           });
  party_subs_.emplace_back(bus, std::in_place_type<fl::events::PartyEncounterBoundary>,
                           [this](const auto&) { request_evaluation(); });
  party_subs_.emplace_back(bus, std::in_place_type<fl::events::TowerDestinationChanged>,
                           [this](const auto&) { request_evaluation(); });
  party_subs_.emplace_back(bus, std::in_place_type<fl::events::TowerFloorCleared>,
                           [this](const fl::events::TowerFloorCleared& event) {
                             last_clear_ = event;
                             request_evaluation();
                           });
  party_subs_.emplace_back(bus, std::in_place_type<fl::events::BiomeUnlocked>,
                           [this](const auto&) { request_evaluation(); });
  party_subs_.emplace_back(bus, std::in_place_type<fl::events::TowerCompleted>,
                           [this](const fl::events::TowerCompleted& event) {
                             completed_biome_ = event.biome_id;
                             request_evaluation();
                           });
  party_subs_.emplace_back(bus, std::in_place_type<fl::events::MemberJoined>,
                           [this](const auto&) { request_evaluation(); });
  party_subs_.emplace_back(bus, std::in_place_type<fl::events::EquipmentEquipped>,
                           [this](const auto&) { request_evaluation(); });

  auto& progression_bus = account_.account_data().parties().front().party_bus();
  recruitment_subs_.emplace_back(progression_bus,
                                 std::in_place_type<fl::events::RecruitmentOpportunityUnlocked>,
                                 [this](const auto&) { request_evaluation(); });
  recruitment_subs_.emplace_back(progression_bus, std::in_place_type<fl::events::HeroRecruited>,
                                 [this](const auto&) { request_evaluation(); });
  if (initially_enabled) set_enabled(true);
}

PartyData& OriginAutoProgression::party() const {
  return account_.account_data().parties().at(party_index_);
}

void OriginAutoProgression::set_enabled(bool enabled) {
  if (enabled_ == enabled) return;
  enabled_ = enabled;
  party().set_progression_control_mode(enabled ? ProgressionControlMode::Auto
                                               : ProgressionControlMode::Manual);
  party().block_origin_auto_progression(false);
  outcome_ = Outcome::None;
  last_clear_.reset();
  completed_biome_.reset();
  recovery_.reset();
  ignore_next_raid_wipe_ = false;
  evaluation_pending_ = enabled;
  decide(enabled ? "enabled" : "disabled");
}

void OriginAutoProgression::request_evaluation() noexcept {
  if (enabled_) evaluation_pending_ = true;
}

void OriginAutoProgression::evaluate() {
  if (!enabled_ || !evaluation_pending_) return;
  evaluation_pending_ = false;
  auto& controlled = party();
  if (controlled.in_raid() || controlled.in_combat()) return;
  if (!controlled.at_encounter_boundary()) return;
  ignore_next_raid_wipe_ = false;
  controlled.block_origin_auto_progression(false);

  if (!controlled.tower_route()) {
    if (controlled.select_tower(PartyData::kOriginBiomeIds.front())) {
      decide("select_biome", PartyData::kOriginBiomeIds.front(), 1);
      return;
    }
    become_blocked("no accessible Origin tower");
    return;
  }
  if (recruit_if_possible()) return;
  if (completed_biome_) {
    outcome_ = Outcome::None;
    recovery_.reset();
    last_clear_.reset();
    completed_biome_.reset();
    if (select_least_progressed_biome()) return;
    become_blocked("Origin route complete");
    return;
  }
  if (outcome_ == Outcome::Defeat) {
    outcome_ = Outcome::None;
    if (handle_defeat()) return;
  } else if (outcome_ == Outcome::Victory) {
    outcome_ = Outcome::None;
    if (handle_recovery_victory()) {
      last_clear_.reset();
      return;
    }
    last_clear_.reset();
  }

  if (controlled.members().empty()) {
    become_blocked("no party members");
    return;
  }
  if (select_least_progressed_biome()) return;

  const auto route = *controlled.tower_route();
  if (route.floor <= controlled.highest_tower_floor_cleared(route.biome_id)) {
    const int frontier = controlled.latest_tower_floor(route.biome_id);
    if (controlled.select_tower(route.biome_id, frontier))
      decide("return_to_frontier", route.biome_id, frontier);
  } else {
    decide("continue", route.biome_id, route.floor);
  }
}

bool OriginAutoProgression::recruit_if_possible() {
  auto& controlled = party();
  if (controlled.members().size() >= 5) return false;
  const auto& opportunities = account_.account_data().recruitment_opportunities();
  const auto found = std::ranges::find_if(opportunities, [this](const auto& opportunity) {
    return opportunity.destination_party == party_index_ &&
           opportunity.state == RecruitmentOpportunityState::Available;
  });
  if (found == opportunities.end()) return false;
  decide("recruit", found->id);
  story_bus_->emit(
      fl::events::StoryEvent{fl::events::RecruitmentClaimRequested{std::string(found->id)}});
  return true;
}

bool OriginAutoProgression::handle_defeat() {
  auto& controlled = party();
  const auto route = controlled.tower_route();
  if (!route) {
    become_blocked("defeat without an active tower route");
    return true;
  }

  if (controlled.tower_floor_attempts() < kAttemptsBeforeRetreat) {
    decide(recovery_ ? "retry_farming" : "retry_frontier", route->biome_id, route->floor);
    return true;
  }

  if (recovery_) {
    if (route->floor > 1 && controlled.select_tower(route->biome_id, route->floor - 1)) {
      recovery_->farming_victories = 0;
      decide("retreat_further", route->biome_id, route->floor - 1);
      return true;
    }
    become_blocked("unable to win the first accessible floor");
    return true;
  }

  const int cleared = controlled.highest_tower_floor_cleared(route->biome_id);
  if (cleared < 1) {
    become_blocked("no cleared floor available for recovery");
    return true;
  }
  recovery_ = Recovery{std::string(route->biome_id), route->floor, 0};
  if (controlled.select_tower(route->biome_id, cleared)) {
    decide("retreat_to_farm", route->biome_id, cleared);
    return true;
  }
  become_blocked("recovery floor selection rejected");
  return true;
}

bool OriginAutoProgression::handle_recovery_victory() {
  if (!recovery_) return false;
  auto& controlled = party();
  const auto route = controlled.tower_route();
  if (!route || route->biome_id != recovery_->biome_id) {
    recovery_.reset();
    return false;
  }
  if (last_clear_ && last_clear_->biome_id == recovery_->biome_id &&
      last_clear_->floor == recovery_->frontier) {
    decide("frontier_recovered", recovery_->biome_id, recovery_->frontier);
    recovery_.reset();
    return select_least_progressed_biome();
  }
  ++recovery_->farming_victories;
  if (recovery_->farming_victories < kFarmingVictoriesBeforeRetry) {
    decide("farm_for_recovery", route->biome_id, route->floor);
    return true;
  }
  const auto frontier = recovery_->frontier;
  if (controlled.select_tower(recovery_->biome_id, frontier)) {
    decide("retry_after_farming", recovery_->biome_id, frontier);
    return true;
  }
  become_blocked("frontier retry selection rejected");
  return true;
}

bool OriginAutoProgression::select_least_progressed_biome() {
  auto& controlled = party();
  const auto current = controlled.tower_route();
  std::optional<std::string_view> selected;
  int selected_floor = 101;
  for (const auto biome : PartyData::kOriginBiomeIds) {
    if (!controlled.biome_unlocked(biome)) continue;
    const int cleared = controlled.highest_tower_floor_cleared(biome);
    if (cleared >= 100 || cleared >= selected_floor) continue;
    selected = biome;
    selected_floor = cleared;
  }
  if (!selected || (current && current->biome_id == *selected)) return false;
  if (!controlled.select_tower(*selected)) return false;
  decide("select_biome", *selected, controlled.latest_tower_floor(*selected));
  return true;
}

void OriginAutoProgression::decide(std::string_view action, std::string_view target, int floor) {
  story_bus_->emit(fl::events::StoryEvent{fl::events::OriginAutoProgressionDecision{
      party_index_, std::string(action), std::string(target), floor}});
}

void OriginAutoProgression::become_blocked(std::string_view reason) {
  party().block_origin_auto_progression(true);
  decide("blocked", reason);
}

} // namespace fl::primitives
