#pragma once

#include "fl/context.hpp"

namespace fl::skills {

class Eviscerate {
public:
  void eviscerate(fl::context::AttackCtx &&ctx);
};

} // namespace fl::skills
