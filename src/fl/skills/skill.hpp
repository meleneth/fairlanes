#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace fl::skills {

enum class SkillId {
  Observe,
  Flee,
  Thump,
  Eviscerate,
  Poison,
  ColdSnap,
  FlameStrike,
  FlameWave,
  Joltspasm,
  RocksFall,
  SourBreath,
  Mercyburst,
  BloodBloom,
  IceSplitter,
  GravitySigh,
  Bump,
  Squish,
  Smack,
};

class SkillRank {
public:
  static constexpr int kMin = 1;
  static constexpr int kMax = 9;

  constexpr SkillRank() noexcept = default;

  [[nodiscard]] static constexpr bool is_valid(int rank) noexcept {
    return rank >= kMin && rank <= kMax;
  }

  [[nodiscard]] static constexpr std::optional<SkillRank>
  from_int(int rank) noexcept {
    if (!is_valid(rank)) {
      return std::nullopt;
    }
    return SkillRank{rank};
  }

  [[nodiscard]] static constexpr SkillRank require(int rank) {
    if (!is_valid(rank)) {
      throw std::out_of_range{"skill rank must be between 1 and 9"};
    }
    return SkillRank{rank};
  }

  [[nodiscard]] constexpr int value() const noexcept { return value_; }

  [[nodiscard]] constexpr bool operator==(const SkillRank &rhs) const noexcept {
    return value_ == rhs.value_;
  }

  [[nodiscard]] constexpr bool operator!=(const SkillRank &rhs) const noexcept {
    return !(*this == rhs);
  }

  [[nodiscard]] constexpr bool operator<(const SkillRank &rhs) const noexcept {
    return value_ < rhs.value_;
  }

  [[nodiscard]] constexpr bool operator<=(const SkillRank &rhs) const noexcept {
    return value_ <= rhs.value_;
  }

private:
  explicit constexpr SkillRank(int rank) noexcept
      : value_{static_cast<std::uint8_t>(rank)} {}

  std::uint8_t value_{1};
};

[[nodiscard]] constexpr std::string_view roman(SkillRank rank) noexcept {
  constexpr std::array<std::string_view, SkillRank::kMax + 1> kRoman{
      "", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX"};
  return kRoman[static_cast<std::size_t>(rank.value())];
}

struct SkillKey {
  SkillId base;
  SkillRank rank;

  constexpr SkillKey(SkillId base_skill,
                     SkillRank skill_rank = SkillRank{}) noexcept
      : base{base_skill}, rank{skill_rank} {}

  [[nodiscard]] constexpr bool operator==(const SkillKey &rhs) const noexcept {
    return base == rhs.base && rank == rhs.rank;
  }

  [[nodiscard]] constexpr bool operator!=(const SkillKey &rhs) const noexcept {
    return !(*this == rhs);
  }
};

struct SkillKeyHash {
  [[nodiscard]] std::size_t operator()(SkillKey skill) const noexcept {
    const auto base_hash = std::hash<int>{}(static_cast<int>(skill.base));
    const auto rank_hash = std::hash<int>{}(skill.rank.value());
    return base_hash ^
           (rank_hash + 0x9e3779b9U + (base_hash << 6U) + (base_hash >> 2U));
  }
};

inline constexpr std::array<SkillKey, 17> kRandomCombatSkills{
    SkillKey{SkillId::Thump},       SkillKey{SkillId::Eviscerate},
    SkillKey{SkillId::Poison},      SkillKey{SkillId::ColdSnap},
    SkillKey{SkillId::FlameStrike}, SkillKey{SkillId::FlameWave},
    SkillKey{SkillId::Bump},        SkillKey{SkillId::Squish},
    SkillKey{SkillId::Smack},       SkillKey{SkillId::Joltspasm},
    SkillKey{SkillId::RocksFall},   SkillKey{SkillId::SourBreath},
    SkillKey{SkillId::Mercyburst},  SkillKey{SkillId::BloodBloom},
    SkillKey{SkillId::IceSplitter}, SkillKey{SkillId::GravitySigh},
    SkillKey{SkillId::Flee}};

enum class SkillTag {
  Physical,
  Blunt,
  Piercing,
  Slashing,
  Bleed,
  Poison,
  Disease,
  Acid,
  Fire,
  Cold,
  Lightning,
  Earth,
  Gravity,
  Sonic,
  Healing,
  Holy,
  Control,
  Area,
  Projectile,
  Melee,
  Observe,
  Utility,
  Escape,
};

enum class SkillExecutionKind {
  ThumpLike,
  Eviscerate,
  Poison,
  ColdSnap,
  FlameStrike,
  FlameWave,
  DecalStrike,
  Flee,
  Observe,
};

struct SkillDefinition;

const SkillDefinition &definition(SkillKey skill) noexcept;

std::string_view name(SkillKey skill) noexcept;
std::string display_name(SkillKey skill);
int learn_chance_percent(SkillKey skill) noexcept;
std::span<const SkillTag> tags(SkillKey skill) noexcept;
bool has_tag(SkillKey skill, SkillTag tag) noexcept;

} // namespace fl::skills
