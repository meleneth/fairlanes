#include "is_party.hpp"
#include "fl/context.hpp"
#include "fl/ecs/components/encounter.hpp"
#include "fl/ecs/components/is_account.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/fsm/party_loop.hpp"
#include "fl/fsm/party_loop_ctx.hpp"
#include "fl/fsm/party_loop_machine.hpp"

namespace fl::ecs::components {
namespace sml = boost::sml;
using fl::fsm::NextEvent;
using fl::fsm::PartyLoop;

IsParty::IsParty(fl::fsm::PartyLoopCtx ctx, std::string name,
                 entt::entity account)
    : party_loop_machine_{std::make_unique<fl::fsm::PartyLoopMachine>(ctx)},
      name_{std::move(name)}, account_{account} {}

void IsParty::next() { party_loop_machine_->start(name_); }

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
void IsParty::add_party_member(entt::entity member) {
  // auto &account = ctx_.reg().get<fl::ecs::components::IsAccount>(account_);
  (void)member;
  // ctx_.party_->log().append_markup(
  //    "[cyan](IsParty) adding member ID " +
  //   std::to_string(
  //      static_cast<std::underlying_type_t<entt::entity>>(member)) +
  //  " to party [party_name](" + name_ + ")");
  // party_members_.push_back(member);
};

} // namespace fl::ecs::components
