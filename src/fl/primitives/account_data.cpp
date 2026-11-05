#include "fl/primitives/account_data.hpp"
#include <fmt/format.h>

#include <algorithm>

#include "fl/ecs/components/is_party.hpp"
#include "fl/generated/monster_content.hpp"
#include "fl/generated/recruitment_content.hpp"
#include "fl/primitives/hero_names.hpp"
#include "fl/primitives/party_names.hpp"

namespace fl::primitives {

void AccountData::hook_to_beat(fl::context::AccountCtx ctx, seerin::BeatBus& beats,
                               WorldClock& world_clock, bool enable_visitors) {
  progression_sub_ = beats.subscribe<seerin::Beat>(
      [this, ctx, &world_clock, enable_visitors](const seerin::Beat&) {
        calendar_->set_beat_rate_multiplier(world_clock.beat_rate_multiplier());
        advance_beat(ctx, enable_visitors);
      });
  // Day zero is a real arrival, before ordinary party progression starts.
  if (enable_visitors) try_start_visitor(ctx);
}

bool AccountData::try_start_visitor(fl::context::AccountCtx ctx) {
  if (in_raid() || calendar_->elapsed_beats() < next_visitor_beat_) return false;
  if (!spawn_visitor(ctx)) return false;
  // Consume this occurrence; elapsed arrivals never become banked attempts.
  next_visitor_beat_ = (calendar_->elapsed_beats() / kVisitorPeriodBeats + 1) * kVisitorPeriodBeats;
  return true;
}

bool AccountData::spawn_visitor(fl::context::AccountCtx ctx) {
  return start_raid(ctx, fl::monster::generated_content::raid_bosses());
}

void AccountData::advance_beat(fl::context::AccountCtx ctx, bool enable_visitors) {
  if (in_raid()) {
    raid_->tick();
    return;
  }
  if (enable_visitors && try_start_visitor(ctx)) return;
  calendar_->advance_beat();
  if (enable_visitors && try_start_visitor(ctx)) return;
  for (auto& party : parties_) party.advance_beat();
  resolve_recruitment_events(ctx);
}

fl::events::PartyBus& AccountData::progression_bus() {
  return parties_.front().party_bus();
}

void AccountData::enable_origin_recruitment() {
  if (recruitment_enabled_ || parties_.empty()) return;
  recruitment_enabled_ = true;
  recruitment_opportunities_.reserve(fl::recruitment::generated_content::kOpportunities.size());
  for (const auto& definition : fl::recruitment::generated_content::kOpportunities) {
    recruitment_opportunities_.push_back(
        {std::string(definition.id), std::string(definition.biome_id), definition.floor,
         static_cast<std::size_t>(definition.destination_party - 1),
         RecruitmentOpportunityState::Locked});
  }
  for (std::size_t index = 0; index < parties_.size(); ++index) bind_recruitment_party(index);
}

void AccountData::bind_recruitment_party(std::size_t party_index) {
  auto& bus = parties_.at(party_index).party_bus();
  recruitment_subscriptions_.emplace_back(bus, std::in_place_type<fl::events::TowerFloorCleared>,
                                          [this](const fl::events::TowerFloorCleared& event) {
                                            pending_floor_clears_.push_back(event);
                                          });
}

void AccountData::resolve_recruitment_events(fl::context::AccountCtx ctx) {
  auto pending = std::move(pending_floor_clears_);
  pending_floor_clears_.clear();
  for (const auto& event : pending)
    committed_floor_clears_.insert(fmt::format("{}#{}", event.biome_id, event.floor));
  unlock_floor_opportunities(ctx);
}

bool AccountData::ensure_party_unlocked(fl::context::AccountCtx ctx, std::size_t party_index) {
  if (party_index < parties_.size()) return true;
  // Party storage is sequential. Production floor progression reaches these
  // tiers in order, so a gap means the prerequisite tier has not committed yet.
  if (party_index != parties_.size() || party_index >= 5 || party_index >= party_names.size())
    return false;

  const auto party_id = ctx.reg().create();
  auto& party = parties_.emplace_back(party_id, ctx, party_names.at(party_index));
  ctx.reg().emplace<fl::ecs::components::IsParty>(party_id, party_id, party);
  bind_recruitment_party(party_index);
  log().append_plain(fmt::format("Party {} unlocked: {}.", party_index + 1, party.name()));
  return true;
}

void AccountData::unlock_floor_opportunities(fl::context::AccountCtx ctx) {
  using fl::recruitment::generated_content::Trigger;
  const auto& definitions = fl::recruitment::generated_content::kOpportunities;
  for (std::size_t index = 0; index < definitions.size(); ++index) {
    const auto& definition = definitions[index];
    auto& opportunity = recruitment_opportunities_[index];
    const auto key = fmt::format("{}#{}", definition.biome_id, definition.floor);
    if (definition.trigger != Trigger::TowerMilestone || !committed_floor_clears_.contains(key) ||
        opportunity.state != RecruitmentOpportunityState::Locked)
      continue;
    const bool party_created = opportunity.destination_party == parties_.size();
    if (!ensure_party_unlocked(ctx, opportunity.destination_party)) continue;
    opportunity.state = RecruitmentOpportunityState::Available;
    progression_bus().emit(fl::events::PartyEvent{fl::events::RecruitmentOpportunityUnlocked{
        opportunity.id, opportunity.biome_id, opportunity.floor, opportunity.destination_party}});
    if (party_created)
      progression_bus().emit(
          fl::events::PartyEvent{fl::events::PartyUnlocked{opportunity.destination_party}});
  }
}

bool AccountData::claim_recruitment(fl::context::AccountCtx ctx, std::string_view opportunity_id) {
  const auto found =
      std::ranges::find(recruitment_opportunities_, opportunity_id, &RecruitmentOpportunity::id);
  if (found == recruitment_opportunities_.end() ||
      found->state != RecruitmentOpportunityState::Available ||
      found->destination_party >= parties_.size())
    return false;
  auto& party = parties_[found->destination_party];
  if (party.members().size() >= 5) return false;
  const auto name_index = found->destination_party * 5 + party.members().size();
  if (name_index >= hero_names.size()) return false;
  const auto member = party.recruit_origin_member(hero_names[name_index]);
  if (member == entt::null) return false;
  found->state = RecruitmentOpportunityState::Claimed;
  progression_bus().emit(fl::events::PartyEvent{
      fl::events::HeroRecruited{found->id, found->destination_party, member}});
  if (party.members().size() == 5) complete_roster(ctx, found->destination_party);
  return true;
}

bool AccountData::recruit_story_hero(fl::context::AccountCtx ctx, std::string_view opportunity_id,
                                     std::string_view name) {
  if (parties_.empty() || claimed_story_recruits_.contains(std::string(opportunity_id)))
    return false;
  auto& party = parties_.front();
  if (party.members().size() >= 5) return false;
  const auto member = party.recruit_origin_member(std::string(name));
  if (member == entt::null) return false;
  claimed_story_recruits_.emplace(opportunity_id);
  progression_bus().emit(
      fl::events::PartyEvent{fl::events::HeroRecruited{std::string(opportunity_id), 0, member}});
  if (party.members().size() == 5) complete_roster(ctx, 0);
  return true;
}

void AccountData::complete_roster(fl::context::AccountCtx ctx, std::size_t party_index) {
  if (party_index >= completed_rosters_.size() || completed_rosters_[party_index]) return;
  completed_rosters_[party_index] = true;
  progression_bus().emit(fl::events::PartyEvent{fl::events::PartyRosterCompleted{party_index}});
  unlock_completion_opportunities(ctx, party_index);
}

void AccountData::unlock_completion_opportunities(fl::context::AccountCtx ctx,
                                                  std::size_t party_index) {
  using fl::recruitment::generated_content::Trigger;
  const auto& definitions = fl::recruitment::generated_content::kOpportunities;
  std::vector<std::size_t> matching;
  for (std::size_t index = 0; index < definitions.size(); ++index) {
    const auto& definition = definitions[index];
    auto& opportunity = recruitment_opportunities_[index];
    if (definition.trigger != Trigger::PartyCompleted ||
        definition.completed_party != static_cast<int>(party_index + 1) ||
        opportunity.state != RecruitmentOpportunityState::Locked)
      continue;
    matching.push_back(index);
  }
  if (matching.empty()) return;
  const auto destination = recruitment_opportunities_[matching.front()].destination_party;
  const bool party_created = destination == parties_.size();
  if (!ensure_party_unlocked(ctx, destination)) return;
  for (const auto index : matching)
    recruitment_opportunities_[index].state = RecruitmentOpportunityState::Available;
  if (party_created)
    progression_bus().emit(fl::events::PartyEvent{fl::events::PartyUnlocked{destination}});
  for (const auto index : matching) {
    const auto& opportunity = recruitment_opportunities_[index];
    progression_bus().emit(fl::events::PartyEvent{fl::events::RecruitmentOpportunityUnlocked{
        opportunity.id, opportunity.biome_id, opportunity.floor, opportunity.destination_party}});
  }
}

} // namespace fl::primitives
