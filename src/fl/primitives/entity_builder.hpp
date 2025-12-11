#pragma once

#include <entt/entt.hpp>
#include <utility>

#include "fl/context.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/monsters/monster_registry.hpp"
#include "fl/primitives/component_builder.hpp"

class EntityBuilder {
  fl::context::PartyCtx ctx_;
  entt::entity ent_{entt::null};

public:
  explicit EntityBuilder(fl::context::PartyCtx context)
      : ctx_(context), ent_(ctx_.reg_.create()) {}

  template <typename C> EntityBuilder &with(C c) {
    ctx_.reg_.emplace_or_replace<C>(ent_, std::move(c));
    return *this;
  }

  template <typename C> EntityBuilder &with_default() {
    ctx_.reg_.emplace_or_replace<C>(
        ent_, ComponentBuilder<C>::defaults(ctx_.entity_context(ent_)));
    return *this;
  }

  // ---- New: accessors so archetype functions can poke internals ----
  fl::context::PartyCtx &ctx() { return ctx_; }
  const fl::context::PartyCtx &ctx() const { return ctx_; }

  entt::entity entity() const { return ent_; }
  // ------------------------------------------------------------------

  entt::entity build() const { return ent_; }

  EntityBuilder &with_monster_defaults() {
    using fl::ecs::components::Stats;
    // using fl::ecs::components::Tags;
    using fl::ecs::components::TrackXP;

    with_default<Stats>();
    // with_default<Tags>();
    with_default<TrackXP>();
    // Add any other “every monster has this” components here.

    return *this;
  }

  EntityBuilder &monster(fl::monster::MonsterKind kind) {
    using fl::monster::monster_registry;
    with_monster_defaults();
    auto &reg = monster_registry();
    auto it = reg.find(kind);
    if (it == reg.end()) {
      // For now, crash loudly if you forgot to register.
      throw std::runtime_error("MonsterKind not registered");
    }

    it->second(*this);
    return *this;
  }
};
