#include "party_member.hpp"

#include <entt/entt.hpp>
#include <ranges>
#include <string>

#include "closet.hpp"
#include "fl/ecs/fwd.hpp"

namespace fl::ecs::components {

PartyMember::PartyMember(fl::context::EntityCtx context, std::string name,
                         entt::entity party)
    : party_(party), ctx_(std::move(context)), closet_() {
  (void)name;
}
} // namespace fl::ecs::components
