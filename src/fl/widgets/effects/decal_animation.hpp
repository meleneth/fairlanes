#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

#include "fl/widgets/effects/flame_wave.hpp"

namespace fl::widgets::effects {

enum class DecalAnimationKind {
  FlameWave,
  Shock,
  RocksFall,
  PoisonCloud,
  HolyNova,
  BloodBloom,
  FrostCrack,
  VoidRipple,
};

struct DecalConfig {
  float duration_seconds = 1.0F;
  bool use_background_glow = true;
  bool use_foreground_sparks = true;
  std::uint32_t seed = 0xA17CE5EDu;
};

class DecalAnimation {
public:
  virtual ~DecalAnimation() = default;

  [[nodiscard]] virtual Frame render(float progress) const = 0;
  [[nodiscard]] virtual float duration_seconds() const = 0;
  [[nodiscard]] virtual std::string_view name() const = 0;
  [[nodiscard]] virtual DecalAnimationKind kind() const = 0;
};

[[nodiscard]] std::shared_ptr<const DecalAnimation>
make_decal_animation(DecalAnimationKind kind, int width, int height,
                     DecalConfig config = {});

[[nodiscard]] constexpr std::string_view
name(DecalAnimationKind kind) noexcept {
  switch (kind) {
  case DecalAnimationKind::FlameWave:
    return "FlameWave";
  case DecalAnimationKind::Shock:
    return "Shock";
  case DecalAnimationKind::RocksFall:
    return "RocksFall";
  case DecalAnimationKind::PoisonCloud:
    return "PoisonCloud";
  case DecalAnimationKind::HolyNova:
    return "HolyNova";
  case DecalAnimationKind::BloodBloom:
    return "BloodBloom";
  case DecalAnimationKind::FrostCrack:
    return "FrostCrack";
  case DecalAnimationKind::VoidRipple:
    return "VoidRipple";
  }

  return "Unknown";
}

} // namespace fl::widgets::effects
