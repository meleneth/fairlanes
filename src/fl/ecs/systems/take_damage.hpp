#pragma once
#include <entt/entt.hpp>
#include <ftxui/screen/color.hpp>

namespace fl::context {
struct AttackCtx;
}
namespace fl::ecs::systems {

class TakeDamage {
public:
  static int commit(fl::context::AttackCtx &ctx);
  static int commit(fl::context::AttackCtx &ctx,
                    ftxui::Color damage_number_color);
};

} // namespace fl::ecs::systems
