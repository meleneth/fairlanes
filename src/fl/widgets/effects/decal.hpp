#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

#include <ftxui/screen/color.hpp>

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
  Starfire,
  HitpointNumber,
  Impact,
  Slash,
  Bite,
  Projectile,
  Sweep,
  Burst,
  Beam,
  Heal,
  Cleanse,
  Glitch,
  Aura,
  Field,
  Observe,
};

inline constexpr std::array<DecalAnimationKind, 23>
    kAvailableDecalAnimationKinds{
        DecalAnimationKind::FlameWave,  DecalAnimationKind::Shock,
        DecalAnimationKind::RocksFall,  DecalAnimationKind::PoisonCloud,
        DecalAnimationKind::HolyNova,   DecalAnimationKind::BloodBloom,
        DecalAnimationKind::FrostCrack, DecalAnimationKind::VoidRipple,
        DecalAnimationKind::Starfire,   DecalAnimationKind::HitpointNumber,
        DecalAnimationKind::Impact,     DecalAnimationKind::Slash,
        DecalAnimationKind::Bite,       DecalAnimationKind::Projectile,
        DecalAnimationKind::Sweep,      DecalAnimationKind::Burst,
        DecalAnimationKind::Beam,       DecalAnimationKind::Heal,
        DecalAnimationKind::Cleanse,    DecalAnimationKind::Glitch,
        DecalAnimationKind::Aura,       DecalAnimationKind::Field,
        DecalAnimationKind::Observe,
    };

[[nodiscard]] constexpr const std::array<DecalAnimationKind, 23> &
available_decal_animation_kinds() noexcept {
  return kAvailableDecalAnimationKinds;
}

struct DecalConfig {
  float duration_seconds = 1.0F;
  bool use_background_glow = true;
  bool use_foreground_sparks = true;
  std::uint32_t seed = 0xA17CE5EDu;
  std::optional<ftxui::Color> color;
  std::optional<char> glyph;
  int hitpoints = 0;
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
  case DecalAnimationKind::Starfire:
    return "Starfire";
  case DecalAnimationKind::HitpointNumber:
    return "HitpointNumber";
  case DecalAnimationKind::Impact:
    return "Impact";
  case DecalAnimationKind::Slash:
    return "Slash";
  case DecalAnimationKind::Bite:
    return "Bite";
  case DecalAnimationKind::Projectile:
    return "Projectile";
  case DecalAnimationKind::Sweep:
    return "Sweep";
  case DecalAnimationKind::Burst:
    return "Burst";
  case DecalAnimationKind::Beam:
    return "Beam";
  case DecalAnimationKind::Heal:
    return "Heal";
  case DecalAnimationKind::Cleanse:
    return "Cleanse";
  case DecalAnimationKind::Glitch:
    return "Glitch";
  case DecalAnimationKind::Aura:
    return "Aura";
  case DecalAnimationKind::Field:
    return "Field";
  case DecalAnimationKind::Observe:
    return "Observe";
  }

  return "Unknown";
}

} // namespace fl::widgets::effects
