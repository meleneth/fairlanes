#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/tags.hpp"
#include "fl/ecs/components/track_xp.hpp"

namespace fl::primitives {
template <typename T> struct ComponentBuilder; // primary template

// ---- Components ----

// Shorthand
using json = nlohmann::json;
using fl::ecs::components::Stats;
using fl::ecs::components::TrackXP;

// ---- Specializations ----
template <> struct ComponentBuilder<Stats> {
  static Stats defaults(fl::context::EntityCtx const& ctx) {
    (void)ctx;
    return Stats{};
  } // no designated inits
  static void apply(Stats& s, const json& j) {
    if (auto it = j.find("hp"); it != j.end()) s.hp_ = it->get<int>();
    if (auto it = j.find("max_hp"); it != j.end()) s.max_hp_ = it->get<int>();
    if (auto it = j.find("attack"); it != j.end()) s.attack_ = it->get<int>();
    if (auto it = j.find("armor"); it != j.end()) s.armor_ = it->get<int>();
    if (auto it = j.find("resistances"); it != j.end()) {
      const auto& r = *it;
      if (auto field = r.find("physical"); field != r.end())
        s.resistances_.physical = field->get<int>();
      if (auto field = r.find("magical"); field != r.end())
        s.resistances_.magical = field->get<int>();
      if (auto field = r.find("fire"); field != r.end()) s.resistances_.fire = field->get<int>();
      if (auto field = r.find("ice"); field != r.end()) s.resistances_.ice = field->get<int>();
      if (auto field = r.find("lightning"); field != r.end())
        s.resistances_.lightning = field->get<int>();
    }
    // Old content may still carry mp/max_mp; they are ignored.
  }
};

using fl::ecs::components::Tags;
template <> struct ComponentBuilder<Tags> {
  static Tags defaults(fl::context::EntityCtx& ctx) {
    (void)ctx;
    return Tags{};
  }
  static void apply(Tags& t, const json& j) {
    if (auto it = j.find("values"); it != j.end()) {
      t.values = it->get<std::vector<std::string>>();
    }
  }
};

template <> struct ComponentBuilder<TrackXP> {
  static TrackXP defaults(fl::context::EntityCtx const& ctx) {
    return TrackXP{ctx.entity_context(ctx.self()), 0};
  }
  static void apply(TrackXP& t, const json& j) {
    (void)t;
    (void)j;
  }
};
} // namespace fl::primitives
