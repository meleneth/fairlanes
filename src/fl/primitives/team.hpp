// team.hpp
#pragma once

#include <optional>
#include <vector>

#include <entt/entt.hpp>

#include "fl/context.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/move_only.hpp"
#include "fl/primitives/random_hub.hpp"

namespace fl::primitives {

struct Team {
public:
  Team() = default;

  auto begin() { return members_.begin(); }
  auto end() { return members_.end(); }
  auto begin() const { return members_.begin(); }
  auto end() const { return members_.end(); }

  // ---- accessors ----
  std::vector<entt::entity> &members() noexcept { return members_; }
  const std::vector<entt::entity> &members() const noexcept { return members_; }

  bool contains(entt::entity e) const {
    return std::find(members_.begin(), members_.end(), e) != members_.end();
  }

  // MA RK_CLASS_MOVEONLY(Team);

  template <fl::context::WorldCoreCtx Ctx, typename Fn>
  void for_each_alive_member(Ctx &ctx, Fn &&fn) const {
    using fl::ecs::components::Stats;

    for (auto e : members_) {
      if (auto *stats = ctx.reg().template try_get<Stats>(e)) {
        if (stats->is_alive()) {
          std::forward<Fn>(fn)(e);
        }
      }
    }
  }

  template <fl::context::WorldCoreCtx Ctx, typename Fn>
  void for_each_member(Ctx &, Fn &&fn) const {
    for (auto e : members_) {
      std::forward<Fn>(fn)(e);
    }
  }

  template <fl::context::WorldCoreCtx Ctx>
  bool has_alive_members(Ctx &ctx) const {
    using fl::ecs::components::Stats;

    for (auto e : members_) {
      if (auto *stats = ctx.reg().template try_get<Stats>(e)) {
        if (stats->hp_ > 0) {
          return true;
        }
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
      if (auto *stats = ctx.reg().template try_get<Stats>(e)) {
        if (stats->hp_ > 0) {
          out.push_back(e);
        }
      }
    }
    return out;
  }

  template <fl::context::WorldCoreCtx Ctx>
  std::optional<entt::entity> random_alive_member(Ctx &ctx) const {
    auto rs = ctx.rng().stream("encounter/team");

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

private:
  std::vector<entt::entity> members_;
};

} // namespace fl::primitives
