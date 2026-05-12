#include "fl/primitives/encounter_data.hpp"

#include <fmt/format.h>

#include "fl/context.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/skills/skill_selection.hpp"
#include "fl/skills/skill_sequence.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "sr/atb_events.hpp"
#include <tracy/Tracy.hpp>

namespace fl::primitives {

void EncounterData::innervate_event_system() {
  ZoneScopedN("EncounterData::innervate_event_system");
  atb_out().on<seerin::BecameActive>([this](const seerin::BecameActive &ev) {
    ZoneScopedN("EncounterData::BecameActive");
    const entt::entity attacker = ev.id;
    const entt::entity target = target_random_alive_opposition(attacker);

    if (target == entt::null) {
      party_ctx_->log().append_markup(fmt::format(
          "{} did nothing.",
          party_ctx_->reg().get<fl::ecs::components::Stats>(attacker).name_));

      atb_in().emit(seerin::AtbInEvent{seerin::FinishedTurn{attacker}});
      return;
    }

    fl::skills::SkillSequencer sequencer{
        *party_ctx_, rt_.atb_.scheduler(),
        [this](entt::entity entity) {
          atb_in().emit(seerin::AtbInEvent{seerin::FinishedTurn{entity}});
        },
        [this](entt::entity entity) { clear_pending_events_for(entity); }};
    sequencer.schedule(attacker, target, choose_skill(attacker));
    TracyPlot("Encounter.PendingEvents",
              static_cast<double>(rt_.atb_.scheduler().pending()));
  });
}

fl::skills::SkillId EncounterData::choose_skill(entt::entity attacker) {
  ZoneScopedN("EncounterData::choose_skill");
  return fl::skills::choose_skill(party_ctx_->reg(), party_ctx_->rng(),
                                  attacker);
}

void EncounterData::finalize() {
  party_ctx_->log().append_markup(
      fmt::format("Finalizing encounter with {} entities to clean up",
                  life_.entities_to_cleanup_.size()));

  for (auto e_cleanup : life_.entities_to_cleanup_) {
    party_ctx_->reg().destroy(e_cleanup);
  }

  party_ctx_->log().append_markup(
      fmt::format("Encounter {} finalized and cleaned up",
                  int(entt::to_integral(party_ctx_->self()))));
}

void EncounterData::clear_pending_events() { rt_.atb_.clear_pending_events(); }

void EncounterData::clear_pending_events_for(entt::entity id) {
  ZoneScopedN("EncounterData::clear_pending_events_for");
  rt_.atb_.clear_pending_events_for(id);
}

bool EncounterData::has_alive_enemies() {
  using fl::ecs::components::Stats;

  for (auto e : life_.entities_to_cleanup_) {
    if (!party_ctx_->reg().valid(e) || !party_ctx_->reg().all_of<Stats>(e)) {
      continue;
    }

    auto &enemy = party_ctx_->reg().get<Stats>(e);
    if (enemy.is_alive()) {
      return true;
    }
  }

  return false;
}

bool EncounterData::is_over() { return !has_alive_enemies(); }

EncounterData::EncounterData(fl::context::PartyCtx *party_ctx)
    : party_ctx_(party_ctx) {
  rt_.atb_.set_can_charge_fn([this](entt::entity entity) {
    auto *stats = party_ctx_->reg().try_get<fl::ecs::components::Stats>(entity);
    return stats && stats->is_alive();
  });

  wire_.party_beat_ = fl::events::ScopedPartyListener{
      party_ctx_->bus(), std::in_place_type<fl::events::PartyTick>,
      [this](const fl::events::PartyTick &) { atb_in().emit(seerin::Beat{}); }};
}

} // namespace fl::primitives
