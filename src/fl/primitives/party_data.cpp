#include <fmt/core.h>

#include "fl/context.hpp"
#include "fl/fsm/party_loop_machine.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "party_data.hpp"

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

EncounterData &PartyData::create_encounter() {
  log_->append_markup(fmt::format("Party id={} encounter_data={}",
                                  (int)party_id_,
                                  static_cast<void *>(encounter_data_.get())));
  encounter_data_ = std::make_unique<EncounterData>();

  return *encounter_data_;
}
} // namespace fl::primitives
