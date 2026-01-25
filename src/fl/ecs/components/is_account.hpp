// fl/ecs/components/is_account.hpp
#pragma once
#include <entt/entity/entity.hpp>

namespace fl::widgets {
class FancyLog;
}

namespace fl::ecs::components {

struct IsAccount {
  entt::entity self_{entt::null};
  fl::widgets::FancyLog *log_{nullptr};

  IsAccount(entt::entity self, fl::widgets::FancyLog *log)
      : self_(self), log_(log) {}
};

} // namespace fl::ecs::components
