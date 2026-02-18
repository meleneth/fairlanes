#pragma once
#include <entt/entt.hpp>

namespace fl::context {
struct AttackCtx;
}
namespace fl::ecs::systems {

class TakeDamage {
public:
  static void commit(fl::context::AttackCtx &ctx);
};

} // namespace fl::ecs::systems
