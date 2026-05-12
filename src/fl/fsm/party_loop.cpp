#include "party_loop.hpp"

#include "fl/ecs/components/encounter.hpp"
#include "fl/ecs/components/is_party.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/grant_xp_to_party.hpp"
#include "fl/ecs/systems/party_gearing.hpp"
// #include "fl/ecs/systems/replenish_party.hpp"
#include "fl/context.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/skills/thump.hpp"
#include "fl/widgets/fancy_log.hpp"
#include <spdlog/spdlog.h>
#include <tracy/Tracy.hpp>

namespace fl::fsm {
void PartyLoop::Ops::enter_idle(fl::context::PartyCtx &ctx) {
  ZoneScopedN("PartyLoop::enter_idle");
  (void)ctx;
  // Mark the party attached to this FSM as idle.
  // ctx.log().append_markup("[cyan](PartyLoop) Entering idle state.");
};

void PartyLoop::Ops::enter_farming(fl::context::PartyCtx &ctx) {
  ZoneScopedN("PartyLoop::enter_farming");
  (void)ctx;
  // Also set the label for the party tied to this FSM (nice for local UI)
  // ctx.log().append_markup("[cyan](PartyLoop) Entering farming state.");

  fl::primitives::EncounterBuilder{ctx}.thump_it_out();
};

void PartyLoop::Ops::exit_farming(fl::context::PartyCtx &ctx) {
  ZoneScopedN("PartyLoop::exit_farming");
  // Always leave combat when exiting Farming, even when enemies died (not a
  // party wipe). This emits PartyLeftCombat so that subscriptions like
  // DireBleed's left_combat_sub are cleaned up before EncounterData is
  // destroyed by the next create_encounter() call.
  ctx.party_data().leave_combat();

  if (ctx.reg().all_of<fl::ecs::components::Encounter>(ctx.self())) {
    ctx.reg().remove<fl::ecs::components::Encounter>(ctx.self());
  }
  ctx.log().append_plain("Returned to town.");
  entt::handle h{ctx.reg(), ctx.self()};
  // TODO FIXME
  // using fl::systems::ReplenishParty;
  // ReplenishParty::commit(h);
};

void PartyLoop::Ops::enter_dead(fl::context::PartyCtx &ctx) {
  ZoneScopedN("PartyLoop::enter_dead");
  ctx.log().append_plain(
      "A passing caravan drags the fallen party back to town.");
  ctx.party_data().leave_combat();
  ctx.party_data().start_town_penalty();
}

void PartyLoop::Ops::enter_fixing(fl::context::PartyCtx &ctx) {
  ZoneScopedN("PartyLoop::enter_fixing");
  (void)ctx;
}

void PartyLoop::Ops::exit_fixing(fl::context::PartyCtx &ctx) {
  ZoneScopedN("PartyLoop::exit_fixing");
  ctx.party_data().revitalize_members();
}

void PartyLoop::Ops::enter_gearing(fl::context::PartyCtx &ctx) {
  ZoneScopedN("PartyLoop::enter_gearing");
  fl::ecs::systems::PartyGearing::commit(ctx);
}

void PartyLoop::Ops::fixing_tick(fl::context::PartyCtx &ctx) {
  ZoneScopedN("PartyLoop::fixing_tick");
  ctx.party_data().tick_town_penalty();
}

void PartyLoop::Ops::combat_tick(fl::context::PartyCtx &ctx) {
  ZoneScopedN("PartyLoop::combat_tick");
  ctx.bus().emit(fl::events::PartyEvent{fl::events::PartyTick{}});
};
bool PartyLoop::Ops::in_combat(fl::context::PartyCtx &ctx) {
  return ctx.party_data().in_combat();
}

bool PartyLoop::Ops::fixing_done(fl::context::PartyCtx &ctx) {
  return !ctx.party_data().town_penalty_active();
}
} // namespace fl::fsm
