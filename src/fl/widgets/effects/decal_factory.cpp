#include "fl/widgets/effects/decal.hpp"

#include <memory>

#include "fl/widgets/effects/blood_bloom.hpp"
#include "fl/widgets/effects/flame_wave_animation.hpp"
#include "fl/widgets/effects/frost_crack.hpp"
#include "fl/widgets/effects/holy_nova.hpp"
#include "fl/widgets/effects/poison_cloud.hpp"
#include "fl/widgets/effects/rocks_fall.hpp"
#include "fl/widgets/effects/shock.hpp"
#include "fl/widgets/effects/void_ripple.hpp"

namespace fl::widgets::effects {

std::shared_ptr<const DecalAnimation>
make_decal_animation(DecalAnimationKind kind, int width, int height,
                     DecalConfig config) {
  switch (kind) {
  case DecalAnimationKind::FlameWave:
    return std::make_shared<FlameWaveAnimation>(width, height, config);
  case DecalAnimationKind::Shock:
    return std::make_shared<Shock>(width, height, config);
  case DecalAnimationKind::RocksFall:
    return std::make_shared<RocksFall>(width, height, config);
  case DecalAnimationKind::PoisonCloud:
    return std::make_shared<PoisonCloud>(width, height, config);
  case DecalAnimationKind::HolyNova:
    return std::make_shared<HolyNova>(width, height, config);
  case DecalAnimationKind::BloodBloom:
    return std::make_shared<BloodBloom>(width, height, config);
  case DecalAnimationKind::FrostCrack:
    return std::make_shared<FrostCrack>(width, height, config);
  case DecalAnimationKind::VoidRipple:
    return std::make_shared<VoidRipple>(width, height, config);
  }

  return std::make_shared<FlameWaveAnimation>(width, height, config);
}

} // namespace fl::widgets::effects
