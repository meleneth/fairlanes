#include <fmt/format.h>

#include <algorithm>
#include <iterator>

#include "fl/context.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/loot_drop.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/fsm/party_loop_machine.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/skills/skill.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "party_data.hpp"
#include "sr/beat_bus.hpp"

namespace fl::primitives {

PartyData::PartyData(entt::entity party_id,
                     fl::context::AccountCtx &account_ctx, std::string name)
    : party_id_(party_id), name_(std::move(name)),
      account_id_(account_ctx.account_data().account_id()),
      log_(std::make_unique<fl::widgets::FancyLog>()),
      party_ctx_(account_ctx.party_context(*this)),
      party_loop_machine_(
          std::make_unique<fl::fsm::PartyLoopMachine>(party_ctx_)) {
  // Optional: early log breadcrumb
  log_->append_markup(
      fmt::format("Party created: id={} name={}", (int)party_id_, name_));
  town_revitalize_sub_ = fl::events::ScopedPartyListener{
      party_bus_, std::in_place_type<fl::events::PartyRevitalizeRequested>,
      [this](const fl::events::PartyRevitalizeRequested &) {
        revitalize_members();
      }};
  loot_drop_sub_ = fl::ecs::systems::LootDropSystem::bind_listener(party_ctx_);
}

void PartyData::hook_to_beat(seerin::BeatBus &gc_beat_bus) {
  gc_forward_sub_ =
      gc_beat_bus.subscribe<seerin::Beat>([this](const seerin::Beat &) {
        // Beat{} on both sides, as requested:
        // log_->append_markup("PartyData received beat");
        party_beat_bus_.emit(seerin::Beat{});
        party_loop_machine_->beat_event();
      });
}

bool PartyData::needs_town() {
  return all_members_dead() || town_penalty_beats_remaining_ > 0;
}

bool PartyData::all_members_dead() const {
  using fl::ecs::components::Stats;

  bool has_members = false;
  for (const auto &member : members_) {
    has_members = true;
    if (const auto *stats =
            party_ctx_.reg().try_get<Stats>(member.member_id())) {
      if (stats->hp_ > 0) {
        return false;
      }
    }
  }

  return has_members;
}

void PartyData::revitalize_members() {
  using fl::ecs::components::Stats;

  for (const auto &member : members_) {
    if (auto *stats = party_ctx_.reg().try_get<Stats>(member.member_id())) {
      const auto old_hp = stats->hp_;
      stats->hp_ = std::max(1, stats->max_hp_);
      stats->mp_ = std::max(0, stats->max_mp_);
      party_bus_.emit(fl::events::PartyEvent{
          fl::events::PartyHealed{member.member_id(), stats->hp_ - old_hp}});
    }
  }
}

void PartyData::start_town_penalty() {
  town_penalty_beats_remaining_ = kTownPenaltyBeats;
}

void PartyData::leave_combat() {
  if (!encounter_data_) {
    return;
  }

  if (!all_members_dead()) {
    party_bus_.emit(fl::events::PartyEvent{fl::events::PartyVictory{}});
  }

  party_bus_.emit(fl::events::PartyEvent{fl::events::PartyLeftCombat{}});
  encounter_data_->clear_pending_events();
  encounter_data_->finalize();
  encounter_data_.reset();
}

void PartyData::watch_skill_learned_this_combat(entt::entity member,
                                                fl::skills::SkillKey skill) {
  if (!in_combat()) {
    return;
  }

  pending_learned_skills_.push_back(
      PendingLearnedSkill{.member = member, .skill = skill});
  auto it = std::prev(pending_learned_skills_.end());

  it->wipe_sub = fl::events::ScopedPartyListener{
      party_bus_, std::in_place_type<fl::events::PartyWiped>,
      [this, it](const fl::events::PartyWiped &) {
        resolve_pending_learned_skill(it, false);
      }};

  it->victory_sub = fl::events::ScopedPartyListener{
      party_bus_, std::in_place_type<fl::events::PartyVictory>,
      [this, it](const fl::events::PartyVictory &) {
        resolve_pending_learned_skill(it, true);
      }};
}

void PartyData::resolve_pending_learned_skill(
    std::list<PendingLearnedSkill>::iterator it, bool keep_skill) {
  if (it == pending_learned_skills_.end()) {
    return;
  }

  auto member = it->member;
  auto skill = it->skill;

  it->wipe_sub.reset();
  it->victory_sub.reset();
  pending_learned_skills_.erase(it);

  if (keep_skill) {
    return;
  }

  auto *party_member =
      party_ctx_.reg().try_get<fl::ecs::components::PartyMember>(member);
  if (party_member != nullptr) {
    party_member->member_data().grimoire().unlearn(skill);
    party_member->closet().unequip_skill(skill);
  }

  if (auto *stats =
          party_ctx_.reg().try_get<fl::ecs::components::Stats>(member)) {
    log_->append_markup(
        fmt::format("[player_name]({}) couldn't quite figure out [ability]({})",
                    stats->name_, fl::skills::display_name(skill)));
  }
}

void PartyData::tick_town_penalty() {
  if (town_penalty_beats_remaining_ > 0) {
    --town_penalty_beats_remaining_;
  }
}

EncounterData &PartyData::create_encounter() {
  /*log_->append_markup(fmt::format("Party id={} encounter_data={}",
                                   (int)party_id_,
                                   static_cast<void *>(encounter_data_.get())));
                                   */
  encounter_data_ = std::make_unique<EncounterData>(&party_ctx_);
  encounter_data_->innervate_event_system();

  return *encounter_data_;
}

void PartyData::add_item(entt::entity item) { inventory_.push_back(item); }

std::span<const entt::entity> PartyData::items() const noexcept {
  return inventory_;
}

void PartyData::replace_items(std::vector<entt::entity> items) {
  inventory_ = std::move(items);
}
} // namespace fl::primitives
