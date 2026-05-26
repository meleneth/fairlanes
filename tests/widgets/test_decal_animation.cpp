#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>

#include "fl/lospec500.hpp"
#include "fl/widgets/effects/decal.hpp"

namespace {

bool is_lospec500_color(ftxui::Color color) {
  const auto palette = fl::lospec500::raw_colors();
  return std::find(palette.begin(), palette.end(), color) != palette.end();
}

int active_cells(const fl::widgets::effects::Frame &frame) {
  int count = 0;
  for (const auto &cell : frame.cells) {
    if (cell.active()) {
      ++count;
    }
  }
  return count;
}

void require_lospec500_colors(const fl::widgets::effects::Frame &frame) {
  for (const auto &cell : frame.cells) {
    if (cell.fg) {
      REQUIRE(is_lospec500_color(*cell.fg));
    }
    if (cell.bg) {
      REQUIRE(is_lospec500_color(*cell.bg));
    }
  }
}

} // namespace

TEST_CASE(
    "New non-FlameWave decal animations render Lospec500-colored active frames",
    "[widgets][effects][decal]") {
  using fl::widgets::effects::DecalAnimationKind;

  constexpr std::array<DecalAnimationKind, 7> kinds{
      DecalAnimationKind::Shock,       DecalAnimationKind::RocksFall,
      DecalAnimationKind::PoisonCloud, DecalAnimationKind::HolyNova,
      DecalAnimationKind::BloodBloom,  DecalAnimationKind::FrostCrack,
      DecalAnimationKind::VoidRipple};

  for (const auto kind : kinds) {
    CAPTURE(fl::widgets::effects::name(kind));
    const auto animation =
        fl::widgets::effects::make_decal_animation(kind, 40, 8);
    REQUIRE(animation != nullptr);
    REQUIRE(animation->kind() == kind);
    REQUIRE(animation->name() == fl::widgets::effects::name(kind));

    const auto frame = animation->render(0.5F);
    REQUIRE(frame.width == 40);
    REQUIRE(frame.height == 8);
    REQUIRE(active_cells(frame) > 0);
    require_lospec500_colors(frame);
  }
}

TEST_CASE("Decal animation render is scrubbable for an existing object",
          "[widgets][effects][decal][determinism]") {
  using fl::widgets::effects::DecalAnimationKind;

  const auto animation = fl::widgets::effects::make_decal_animation(
      DecalAnimationKind::VoidRipple, 40, 8);

  const auto first = animation->render(0.42F);
  const auto second = animation->render(0.42F);

  REQUIRE(first.width == second.width);
  REQUIRE(first.height == second.height);
  REQUIRE(first.cells.size() == second.cells.size());
  for (std::size_t i = 0; i < first.cells.size(); ++i) {
    CAPTURE(i);
    REQUIRE(first.cells[i].glyph == second.cells[i].glyph);
    REQUIRE(first.cells[i].alpha == second.cells[i].alpha);
    REQUIRE(first.cells[i].fg.has_value() == second.cells[i].fg.has_value());
    REQUIRE(first.cells[i].bg.has_value() == second.cells[i].bg.has_value());
  }
}

TEST_CASE("Hitpoint number decal drops a colored value",
          "[widgets][effects][decal][hitpoints]") {
  fl::widgets::effects::DecalConfig config;
  config.color = fl::lospec500::color_at(15);
  config.hitpoints = 42;

  const auto animation = fl::widgets::effects::make_decal_animation(
      fl::widgets::effects::DecalAnimationKind::HitpointNumber, 20, 6, config);

  REQUIRE(animation != nullptr);
  REQUIRE(animation->kind() ==
          fl::widgets::effects::DecalAnimationKind::HitpointNumber);

  const auto start = animation->render(0.0F);
  const auto bottom = animation->render(1.0F);

  REQUIRE(start.width == 20);
  REQUIRE(start.height == 6);
  REQUIRE(active_cells(start) == 2);
  REQUIRE(active_cells(bottom) == 2);
  require_lospec500_colors(start);
  require_lospec500_colors(bottom);

  bool saw_bottom_number = false;
  const int landing_row = bottom.height - 2;
  for (int x = 0; x < bottom.width; ++x) {
    if (bottom.at(x, landing_row).glyph == '4' ||
        bottom.at(x, landing_row).glyph == '2') {
      saw_bottom_number = true;
    }
    REQUIRE(bottom.at(x, bottom.height - 1).glyph == ' ');
  }
  REQUIRE(saw_bottom_number);
}
