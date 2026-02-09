#include "party_loop.hpp"

#include "fl/ecs/components/encounter.hpp"
#include "fl/ecs/components/is_party.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/grant_xp_to_party.hpp"
// #include "fl/ecs/systems/replenish_party.hpp"
#include "fl/context.hpp"
#include "fl/fsm/party_loop_ctx.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "fl/skills/thump.hpp"
#include "fl/widgets/fancy_log.hpp"
#include <spdlog/spdlog.h>

namespace fl::fsm {
void PartyLoop::Ops::enter_idle(fl::fsm::PartyLoopCtx &ctx) {
  // Mark the party attached to this FSM as idle.
  (void)ctx;
};

void PartyLoop::Ops::enter_farming(fl::fsm::PartyLoopCtx &ctx) {
  // Also set the label for the party tied to this FSM (nice for local UI)

  auto party_ctx = ctx.party_context();
  fl::primitives::EncounterBuilder{party_ctx}.thump_it_out();
};

void PartyLoop::Ops::exit_farming(fl::fsm::PartyLoopCtx &ctx) {
  ctx.reg().remove<fl::ecs::components::Encounter>(ctx.self());
  // ctx.log_.append_plain("Returned to town.");
  entt::handle h{ctx.reg(), ctx.self()};
  // TODO FIXME
  // using fl::systems::ReplenishParty;
  // ReplenishParty::commit(h);
};

void PartyLoop::Ops::enter_fixing(fl::fsm::PartyLoopCtx &ctx) { (void)ctx; };

void PartyLoop::Ops::combat_tick(fl::fsm::PartyLoopCtx &ctx) {
  using fl::skills::Thump;
  Thump in_the_night;

  using fl::ecs::components::Encounter;
  using fl::ecs::components::Stats;
  auto &encounter = ctx.reg().get<Encounter>(ctx.self());
  encounter.attackers_->for_each_alive_member(ctx, [&](entt::entity attacker) {
    auto defender = encounter.defenders_->random_alive_member(ctx);
    if (defender) {
      auto party_context = ctx.party_context();
      in_the_night.thump(fl::context::AttackCtx::make_attack(
          party_context, attacker, *defender));
    }
  });

  encounter.defenders_->for_each_alive_member(ctx, [&](entt::entity attacker) {
    auto defender = encounter.attackers_->random_alive_member(ctx);
    if (defender) {
      auto party_context = ctx.party_context();
      in_the_night.thump(fl::context::AttackCtx::make_attack(
          party_context, attacker, *defender));
    }
  });
};
bool PartyLoop::Ops::in_combat(fl::fsm::PartyLoopCtx &ctx) {
  auto &is_party = ctx.reg().get<fl::ecs::components::IsParty>(ctx.self());
  return is_party.in_combat();
}

bool PartyLoop::Ops::needs_town(fl::fsm::PartyLoopCtx &ctx) {
  auto &is_party = ctx.reg().get<fl::ecs::components::IsParty>(ctx.self());
  return is_party.needs_town();
}
} // namespace fl::fsm
