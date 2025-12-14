#include "is_party.hpp"
#include "fl/context.hpp"
#include "fl/ecs/components/encounter.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/fsm/party_loop.hpp"
#include "fl/fsm/party_loop_ctx.hpp"

namespace fl::ecs::components {
namespace sml = boost::sml;
using fl::fsm::NextEvent;
using fl::fsm::PartyLoop;

IsParty::IsParty(fl::context::PartyCtx context, std::string name,
                 entt::entity account)
    : sm_{context.party_loop_context(), std::move(context)}, account_{account},
      name_{std::move(name)} {}

void IsParty::next() { sm_.process_event(NextEvent{}); }

bool IsParty::needs_town() {
  /*
    bool does_need_town = false;
    for_each_member([&](entt::entity member) {
      // auto &party_member = ctx_.reg_.get<PartyMember>(member);
      auto &stats = ctx.reg().get<fl::ecs::components::Stats>(member);
      if (!stats.is_alive()) {
        does_need_town = true;
      }
    });
    return does_need_town;
    */
  return false;
}

bool IsParty::in_combat() {
  /* using fl::ecs::components::Encounter;

   auto encounter = ctx.reg().try_get<Encounter>(ctx.self_);
   if (!encounter) {
     return false;
   }
   // "in combat" is just "the encounter is not over yet"
   return !encounter->is_over();
   // TODO FIXME
   */
  return true;
}

} // namespace fl::ecs::components
