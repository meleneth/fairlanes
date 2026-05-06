#include "closet.hpp"

namespace fl::ecs::components {

Closet::Closet()
    : chest(entt::null),
      helm(entt::null),
      pants(entt::null),
      belt(entt::null),
      boots(entt::null),
      gloves(entt::null),
      sleeves(entt::null),
      cape(entt::null),
      necklace(entt::null),
      ring_1(entt::null),
      ring_2(entt::null),
      mainhand(entt::null),
      offhand(entt::null),
      knife(entt::null) {}

bool Closet::empty() const {
  return chest == entt::null && helm == entt::null &&
         pants == entt::null && belt == entt::null &&
         boots == entt::null && gloves == entt::null &&
         sleeves == entt::null && cape == entt::null &&
         necklace == entt::null &&
         ring_1 == entt::null && ring_2 == entt::null &&
         mainhand == entt::null && offhand == entt::null &&
         knife == entt::null;
}

}  // namespace fl::ecs::components
