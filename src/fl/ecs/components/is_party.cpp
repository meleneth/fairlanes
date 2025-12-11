#include "is_party.hpp"
#include "fl/context.hpp"
#include "fl/ecs/components/encounter.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/fsm/party_loop.hpp"

namespace fl::ecs::components {
namespace sml = boost::sml;
using fl::fsm::NextEvent;
using fl::fsm::PartyLoop;

IsParty::IsParty(std::string name, entt::entity account)
    : account_{account}, name_{std::move(name)} {}

void IsParty::next() { sm_.process_event(NextEvent{}); }

bool IsParty::needs_town(fl::context::PartyCtx &ctx) {

  bool does_need_town = false;
  for_each_member([&](entt::entity member) {
    // auto &party_member = ctx_.reg_.get<PartyMember>(member);
    auto &stats = ctx.reg_.get<fl::ecs::components::Stats>(member);
    if (!stats.is_alive()) {
      does_need_town = true;
    }
  });
  return does_need_town;
}

bool IsParty::in_combat(fl::context::PartyCtx ctx) {
  using fl::ecs::components::Encounter;

  auto encounter = ctx.reg_.try_get<Encounter>(ctx.self_);
  if (!encounter) {
    return false;
  }
  // "in combat" is just "the encounter is not over yet"
  return !encounter->is_over();
}

} // namespace fl::ecs::components
