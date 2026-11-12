#include "fl/ecs/systems/change_combat_affinity.hpp"

#include <algorithm>

#include "fl/ecs/components/combat_affinity.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/systems/party_gearing.hpp"
#include "fl/primitives/party_data.hpp"

namespace fl::ecs::systems {

AffinityChangeResult ChangeCombatAffinity::commit(fl::context::PartyCtx& ctx, entt::entity member,
                                                  fl::primitives::CombatAffinity affinity) {
  if (affinity == fl::primitives::CombatAffinity::Neutral || !ctx.reg().valid(member) ||
      ctx.reg().try_get<fl::ecs::components::PartyMember>(member) == nullptr)
    return AffinityChangeResult::NotAMember;
  const auto found = std::ranges::find_if(ctx.party_data().members(), [member](const auto& entry) {
    return entry.member_id() == member;
  });
  if (found == ctx.party_data().members().end()) return AffinityChangeResult::NotAMember;
  if (!ctx.party_data().at_encounter_boundary()) return AffinityChangeResult::NotInTown;

  auto& active = ctx.reg().get_or_emplace<fl::ecs::components::ActiveCombatAffinity>(member);
  if (active.value == affinity) return AffinityChangeResult::Unchanged;
  active.value = affinity;
  PartyGearing::commit(ctx);
  ctx.log().append_plain(std::string(found->name()) + " changed armor affinity to " +
                         std::string(fl::primitives::to_string(affinity)) + ".");
  return AffinityChangeResult::Changed;
}

} // namespace fl::ecs::systems
