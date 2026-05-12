#pragma once

#include <entt/entt.hpp>
#include <utility>

#include "fl/context.hpp"
#include "fl/ecs/components/monster_identity.hpp"
#include "fl/ecs/components/skill_slots.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/monsters/monster_kind.hpp"
#include "fl/monsters/monster_registry.hpp"
#include "fl/monsters/monster_skills.hpp"
#include "fl/primitives/component_builder.hpp"

namespace fl::primitives {
class EntityBuilder {
  fl::context::BuildCtx &ctx_;
  entt::entity ent_{entt::null};

public:
  explicit EntityBuilder(fl::context::BuildCtx &context)
      : ctx_(context), ent_(ctx_.reg().create()) {}

  template <typename C> EntityBuilder &with(C c) {
    ctx_.reg().emplace_or_replace<C>(ent_, std::move(c));
    return *this;
  }

  template <typename C> EntityBuilder &with_default() {
    auto entity_ctx = ctx_.entity_context(ent_);
    ctx_.reg().emplace_or_replace<C>(ent_,
                                     ComponentBuilder<C>::defaults(entity_ctx));
    return *this;
  }

  // ---- New: accessors so archetype functions can poke internals ----
  fl::context::BuildCtx &ctx() { return ctx_; }
  const fl::context::BuildCtx &ctx() const { return ctx_; }

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
    with(fl::ecs::components::MonsterIdentity{kind});
    auto &reg = monster_registry();
    auto it = reg.find(kind);
    if (it == reg.end()) {
      // For now, crash loudly if you forgot to register.
      throw std::runtime_error("MonsterKind not registered");
    }

    it->second(*this);
    with(fl::ecs::components::SkillSlots::with_known(
        fl::monster::known_skill_for(kind)));
    return *this;
  }
};
} // namespace fl::primitives
