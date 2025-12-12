// team.hpp
#pragma once
#include <optional>
#include <vector>

#include <entt/entt.hpp>

#include "fl/context.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/move_only.hpp"

namespace fl::primitives {

struct Team {
  std::vector<entt::entity> members_;

  // MA RK_CLASS_MOVEONLY(Team);

  template <fl::context::WorldCoreCtx Ctx, typename Fn>
  void for_each_alive_member(Ctx &ctx, Fn &&fn) {
    using fl::ecs::components::Stats;

    for (auto e : members_) {
      if (auto *stats = ctx.reg_.template try_get<Stats>(e)) {
        if (stats->is_alive()) {
          std::forward<Fn>(fn)(e);
        }
      }
    }
  }

  template <fl::context::WorldCoreCtx Ctx, typename Fn>
  inline void for_each_member(Ctx &ctx, Fn &&fn) {
    using fl::ecs::components::Stats;
    for (auto e : members_) {
      std::forward<Fn>(fn)(e);
    }
  }
  template <fl::context::WorldCoreCtx Ctx>
  bool has_alive_members(Ctx &ctx) const {
    using fl::ecs::components::Stats;

    for (auto e : members_) {
      if (auto stats = ctx->reg_.template try_get<Stats>(e);
          stats && stats->hp_ > 0) {
        return true;
      }
    }
    return false;
  }

  template <fl::context::WorldCoreCtx Ctx>
  std::vector<entt::entity> alive_members(Ctx &ctx) const {
    using fl::ecs::components::Stats;
    std::vector<entt::entity> out;
    out.reserve(members_.size());
    for (auto e : members_) {
      if (auto stats = ctx.reg_.template try_get<Stats>(e);
          stats && stats->hp_ > 0) {
        out.push_back(e);
      }
    }
    return out;
  }
  template <fl::context::WorldCoreCtx Ctx>
  std::optional<entt::entity> random_alive_member(Ctx &ctx) {
    using fl::ecs::components::Stats;

    auto rs = ctx.rng_.stream("encounter/players");

    std::optional<entt::entity> selected;
    int seen = 0;

    for_each_alive_member(ctx, [&](entt::entity candidate) {
      ++seen;
      if (rs.random_index(seen) == 0) {
        selected = candidate;
      }
    });
    return selected;
  }
};

} // namespace fl::primitives
