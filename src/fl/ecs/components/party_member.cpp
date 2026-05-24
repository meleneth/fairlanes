#include "party_member.hpp"

#include <entt/entt.hpp>
#include <ranges>
#include <string>

#include "closet.hpp"
#include "fl/ecs/fwd.hpp"
#include "fl/primitives/member_data.hpp"

namespace fl::ecs::components {

PartyMember::PartyMember(fl::context::EntityCtx context, std::string name,
                         entt::entity party,
                         fl::primitives::MemberData &member_data)
    : party_(party), ctx_(std::move(context)), closet_(ctx_.reg().create()),
      member_data_(&member_data) {
  (void)name;
  ctx_.reg().emplace<Closet>(closet_);
}
} // namespace fl::ecs::components
