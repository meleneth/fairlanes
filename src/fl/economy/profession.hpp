#pragma once

#include <array>
#include <string_view>

namespace fl::economy {

enum class Profession {
  Herbalism,
  Mining,
  Scavenging,
  Alchemy,
  Smithing,
  Leatherworking,
  Tailoring,
  Study,
  Count,
};

inline constexpr std::array<Profession, 8> kProfessions{
    Profession::Herbalism, Profession::Mining, Profession::Scavenging, Profession::Alchemy,
    Profession::Smithing, Profession::Leatherworking, Profession::Tailoring, Profession::Study};

inline constexpr std::string_view display_name(Profession profession) noexcept {
  switch (profession) {
  case Profession::Herbalism: return "Herbalism";
  case Profession::Mining: return "Mining";
  case Profession::Scavenging: return "Scavenging";
  case Profession::Alchemy: return "Alchemy";
  case Profession::Smithing: return "Smithing";
  case Profession::Leatherworking: return "Leatherworking";
  case Profession::Tailoring: return "Tailoring";
  case Profession::Study: return "Study";
  case Profession::Count: return "Unknown";
  }
  return "Unknown";
}

} // namespace fl::economy
