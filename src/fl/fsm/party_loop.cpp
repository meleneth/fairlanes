#include "party_loop.hpp"

#include "fl/ecs/components/encounter.hpp"
#include "fl/ecs/components/is_party.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/grant_xp_to_party.hpp"
// #include "fl/ecs/systems/replenish_party.hpp"
#include "fl/primitives/encounter_builder.hpp"
#include "fl/skills/thump.hpp"
#include "fl/widgets/fancy_log.hpp"
#include <spdlog/spdlog.h>

namespace fl::fsm {
void PartyLoop::enter_idle(fl::context::PartyCtx &ctx) {
  // Mark the party attached to this FSM as idle.
  (void)ctx;
};

void PartyLoop::enter_farming(fl::context::PartyCtx &ctx) {
  // Also set the label for the party tied to this FSM (nice for local UI)

  using fl::concepts::EncounterBuilder;
  using fl::ecs::components::Encounter;
  EncounterBuilder{ctx}.thump_it_out();
};

void PartyLoop::exit_farming(fl::context::PartyCtx &ctx) {
  using fl::ecs::components::Encounter;
  ctx.reg_.remove<Encounter>(ctx.self_());
  // ctx.log_.append_plain("Returned to town.");
  entt::handle h{ctx.reg_, ctx.self_()};
  // TODO FIXME
  // using fl::systems::ReplenishParty;
  // ReplenishParty::commit(h);
};

void PartyLoop::enter_fixing(fl::context::PartyCtx &ctx) { (void)ctx; };

void PartyLoop::combat_tick(fl::context::PartyCtx &ctx) {
  using fl::skills::Thump;
  Thump in_the_night;

  using fl::ecs::components::Encounter;
  using fl::ecs::components::Stats;
  auto &encounter = ctx.reg_.get<Encounter>(ctx.self_());
  encounter.attackers_->for_each_alive_member(ctx, [&](entt::entity attacker) {
    auto defender = encounter.defenders_->random_alive_member(ctx);
    if (defender) {
      in_the_night.thump(
          fl::context::AttackCtx::make_attack(ctx, attacker, *defender));
    }
  });

  encounter.defenders_->for_each_alive_member(ctx, [&](entt::entity attacker) {
    auto defender = encounter.attackers_->random_alive_member(ctx);
    if (defender) {
      in_the_night.thump(
          fl::context::AttackCtx::make_attack(ctx, attacker, *defender));
    }
  });
};
bool PartyLoop::in_combat(fl::context::PartyCtx &ctx) {
  auto &is_party = ctx.reg_.get<fl::ecs::components::IsParty>(ctx.self_());
  return is_party.in_combat();
}

bool PartyLoop::needs_town(fl::context::PartyCtx &ctx) {
  auto &is_party = ctx.reg_.get<fl::ecs::components::IsParty>(ctx.self_());
  return is_party.needs_town();
}
} // namespace fl::fsm
