#pragma once
#include <entt/entt.hpp>
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

template <typename PM = PartyMember, typename Fn>
inline void for_each_member(entt::registry &reg, entt::entity party_e,
                            Fn &&fn) {
  auto view = reg.view<PM>();
  for (auto e : view) {
    if (view.get(e).party_ == party_e) {
      fn(entt::handle{reg, e});
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
  for_each_member<PM>(*ctx.reg(), party_e, std::forward<Fn>(fn));
}

} // namespace fl::ecs::components
