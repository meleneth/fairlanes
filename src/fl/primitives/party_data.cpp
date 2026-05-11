#include <fmt/core.h>

#include <algorithm>

#include "fl/context.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/fsm/party_loop_machine.hpp"
#include "fl/primitives/account_data.hpp"
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
}

void PartyData::hook_to_beat(seerin::BeatBus &gc_beat_bus) {
  gc_forward_sub_ = gc_beat_bus.on<seerin::Beat>([this](const seerin::Beat &) {
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
    if (const auto *stats = party_ctx_.reg().try_get<Stats>(member.member_id())) {
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
      stats->hp_ = std::max(1, stats->max_hp_);
      stats->mp_ = std::max(0, stats->max_mp_);
    }
  }
}

void PartyData::start_town_penalty() {
  town_penalty_beats_remaining_ = kTownPenaltyBeats;
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
} // namespace fl::primitives
