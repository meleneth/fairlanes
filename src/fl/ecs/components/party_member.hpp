#pragma once
#include <entt/entt.hpp>
#include <ranges>
#include <string>

#include "fl/context.hpp"
#include "fl/ecs/fwd.hpp"


namespace fl::ecs::components {

struct PartyMember {
  fl::ecs::Entity party_;
  fl::context::EntityCtx ctx_;

  PartyMember(fl::context::EntityCtx ctx, std::string /*name*/,
              entt::entity party);
};

// Call `fn(entt::handle)` for each member of `party_e`
template <typename PM = PartyMember, typename Fn>
inline void for_each_member(entt::registry &reg, entt::entity party_e,
                            Fn &&fn) {
  auto view = reg.view<PM>();            // entities with PartyMember
  for (auto e : view) {                  // iterate entities
    if (view.get(e).party_ == party_e) { // match party
      fn(entt::handle{reg, e});          // yield handle
    }
  }
}

template <typename PM = PartyMember, typename Fn>
inline void for_each_member(entt::registry *reg, entt::entity party_e,
                            Fn &&fn) {
  for_each_member<PM>(*reg, party_e, std::forward<Fn>(fn));
}

template <typename Ctx, typename PM = PartyMember, typename Fn>
inline void for_each_member(Ctx &ctx, entt::entity party_e, Fn &&fn) {
  for_each_member<PM>(*ctx.reg_, party_e, std::forward<Fn>(fn));
}

} // namespace fl::ecs::components
