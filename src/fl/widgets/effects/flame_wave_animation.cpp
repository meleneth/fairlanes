#include "fl/widgets/effects/flame_wave_animation.hpp"

namespace fl::widgets::effects {

FlameWaveAnimation::FlameWaveAnimation(int width, int height,
                                       DecalConfig config)
    : width_(width), height_(height) {
  FlameWaveConfig flame_config;
  flame_config.duration_seconds = config.duration_seconds;
  flame_config.use_background_glow = config.use_background_glow;
  flame_config.use_foreground_sparks = config.use_foreground_sparks;
  flame_config.seed = config.seed;
  flame_ = FlameWave{flame_config};
}

Frame FlameWaveAnimation::render(float progress) const {
  return flame_.render(progress, width_, height_);
}

float FlameWaveAnimation::duration_seconds() const {
  return flame_.config().duration_seconds;
}

std::string_view FlameWaveAnimation::name() const { return "FlameWave"; }

DecalAnimationKind FlameWaveAnimation::kind() const {
  return DecalAnimationKind::FlameWave;
}

} // namespace fl::widgets::effects
