#include "party_member.hpp"

#include <entt/entt.hpp>
#include <ranges>
#include <string>

#include "closet.hpp"
#include "fl/ecs/fwd.hpp"

namespace fl::ecs::components {

PartyMember::PartyMember(fl::context::EntityCtx context, std::string name,
                         entt::entity party)
    : party_(party), ctx_(std::move(context)), closet_(ctx_.reg().create()) {
  (void)name;
  ctx_.reg().emplace<Closet>(closet_);
}
} // namespace fl::ecs::components
