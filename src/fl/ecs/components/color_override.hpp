#include <entt/entt.hpp>
#include <ftxui/component/component.hpp>

namespace fl::ecs::components {

struct ColorOverride {
  ftxui::Color color;
};

inline void safe_add_color(entt::registry &reg, entt::entity e,
                           ftxui::Color c) {
  if (!reg.valid(e))
    return;
  reg.emplace_or_replace<ColorOverride>(e, ColorOverride{c});
}

inline void safe_clear_color(entt::registry &reg, entt::entity e) {
  if (!reg.valid(e))
    return;
  reg.remove<ColorOverride>(e);
}

} // namespace fl::ecs::components
