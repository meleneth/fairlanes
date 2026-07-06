#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <string_view>

#include "fl/lospec500.hpp"
#include "fl/widgets/effects/archetype_decal.hpp"
#include "fl/widgets/effects/decal.hpp"
#include "fl/widgets/effects/starfire.hpp"

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

int visible_glyphs(const fl::widgets::effects::Frame &frame) {
  int count = 0;
  for (const auto &cell : frame.cells) {
    if (cell.glyph != ' ') {
      ++count;
    }
  }
  return count;
}

int leftmost_visible_x(const fl::widgets::effects::Frame &frame) {
  int left = frame.width;
  for (int y = 0; y < frame.height; ++y) {
    for (int x = 0; x < frame.width; ++x) {
      if (frame.at(x, y).glyph != ' ') {
        left = std::min(left, x);
      }
    }
  }
  return left == frame.width ? -1 : left;
}

int rightmost_visible_x(const fl::widgets::effects::Frame &frame) {
  int right = -1;
  for (int y = 0; y < frame.height; ++y) {
    for (int x = 0; x < frame.width; ++x) {
      if (frame.at(x, y).glyph != ' ') {
        right = std::max(right, x);
      }
    }
  }
  return right;
}

bool same_frame(const fl::widgets::effects::Frame &lhs,
                const fl::widgets::effects::Frame &rhs) {
  if (lhs.width != rhs.width || lhs.height != rhs.height ||
      lhs.cells.size() != rhs.cells.size()) {
    return false;
  }

  for (std::size_t i = 0; i < lhs.cells.size(); ++i) {
    if (lhs.cells[i].glyph != rhs.cells[i].glyph ||
        lhs.cells[i].alpha != rhs.cells[i].alpha ||
        lhs.cells[i].fg != rhs.cells[i].fg ||
        lhs.cells[i].bg != rhs.cells[i].bg) {
      return false;
    }
  }
  return true;
}

int glyph_count(const fl::widgets::effects::Frame &frame, char glyph) {
  int count = 0;
  for (const auto &cell : frame.cells) {
    if (cell.glyph == glyph) {
      ++count;
    }
  }
  return count;
}

bool has_non_lospec_color(const fl::widgets::effects::Frame &frame) {
  for (const auto &cell : frame.cells) {
    if (cell.fg && !is_lospec500_color(*cell.fg)) {
      return true;
    }
    if (cell.bg && !is_lospec500_color(*cell.bg)) {
      return true;
    }
  }
  return false;
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

bool frame_contains_glyph(const fl::widgets::effects::Frame &frame,
                          char glyph) {
  for (const auto &cell : frame.cells) {
    if (cell.glyph == glyph) {
      return true;
    }
  }
  return false;
}

bool frame_contains_any(const fl::widgets::effects::Frame &frame,
                        std::string_view glyphs) {
  for (const auto &cell : frame.cells) {
    if (glyphs.find(cell.glyph) != std::string_view::npos) {
      return true;
    }
  }
  return false;
}

} // namespace

TEST_CASE("Skill visual archetype decals render distinct shape languages",
          "[widgets][effects][decal][archetype]") {
  using fl::widgets::effects::DecalAnimationKind;

  struct ExpectedArchetypeRender {
    DecalAnimationKind kind;
    std::string_view characteristic_glyphs;
  };

  constexpr std::array<ExpectedArchetypeRender, 13> expected{{
      {DecalAnimationKind::Impact, "X#<>/\\"},
      {DecalAnimationKind::Slash, "/"},
      {DecalAnimationKind::Bite, "V^"},
      {DecalAnimationKind::Projectile, ">-"},
      {DecalAnimationKind::Sweep, "~="},
      {DecalAnimationKind::Burst, "*o"},
      {DecalAnimationKind::Beam, "=#"},
      {DecalAnimationKind::Heal, "+"},
      {DecalAnimationKind::Cleanse, "|o"},
      {DecalAnimationKind::Glitch, "#%@?/\\|!"},
      {DecalAnimationKind::Aura, "o^"},
      {DecalAnimationKind::Field, "~:"},
      {DecalAnimationKind::Observe, "@*+~=^v/\\"},
  }};

  for (const auto &entry : expected) {
    CAPTURE(fl::widgets::effects::name(entry.kind));
    const auto animation =
        fl::widgets::effects::make_decal_animation(entry.kind, 42, 9);
    REQUIRE(animation != nullptr);
    REQUIRE(animation->kind() == entry.kind);
    REQUIRE(animation->name() == fl::widgets::effects::name(entry.kind));

    const auto frame = animation->render(0.5F);
    REQUIRE(frame.width == 42);
    REQUIRE(frame.height == 9);
    REQUIRE(active_cells(frame) > 0);
    REQUIRE(visible_glyphs(frame) > 0);
    REQUIRE(frame_contains_any(frame, entry.characteristic_glyphs));
    require_lospec500_colors(frame);
  }
}

TEST_CASE("Observe decal renders a large bloodshot eyeball",
          "[widgets][effects][decal][observe]") {
  using fl::widgets::effects::DecalAnimationKind;

  const auto animation = fl::widgets::effects::make_decal_animation(
      DecalAnimationKind::Observe, 48, 11);

  REQUIRE(animation != nullptr);
  REQUIRE(animation->kind() == DecalAnimationKind::Observe);
  REQUIRE(animation->name() ==
          fl::widgets::effects::name(DecalAnimationKind::Observe));

  const auto frame = animation->render(0.5F);
  REQUIRE(frame.width == 48);
  REQUIRE(frame.height == 11);
  REQUIRE(active_cells(frame) > 45);
  REQUIRE(glyph_count(frame, '@') >= 2);
  REQUIRE(frame_contains_any(frame, "~=^v"));
  require_lospec500_colors(frame);
}

TEST_CASE("Impact renders compact physical hit phases without reticle shapes",
          "[widgets][effects][decal][archetype]") {
  using fl::widgets::effects::ArchetypeDecal;
  using fl::widgets::effects::DecalAnimationKind;

  const ArchetypeDecal impact{DecalAnimationKind::Impact, 40, 9};
  const auto contact = impact.render(0.1F);
  const auto compression = impact.render(0.38F);
  const auto aftermark = impact.render(0.82F);

  CHECK(frame_contains_glyph(contact, 'X'));
  CHECK(frame_contains_glyph(contact, '#'));
  CHECK(frame_contains_glyph(compression, '<'));
  CHECK(frame_contains_glyph(compression, '>'));
  CHECK(frame_contains_glyph(aftermark, '#'));

  CHECK_FALSE(frame_contains_glyph(contact, 'o'));
  CHECK_FALSE(frame_contains_glyph(contact, 'O'));
  CHECK_FALSE(frame_contains_glyph(contact, '+'));
  CHECK_FALSE(frame_contains_glyph(compression, 'o'));
  CHECK_FALSE(frame_contains_glyph(aftermark, 'o'));

  CHECK(active_cells(contact) < 12);
  CHECK(active_cells(aftermark) < active_cells(compression));
}

TEST_CASE("Extended archetype animations report longer durations",
          "[widgets][effects][decal][archetype]") {
  using fl::widgets::effects::ArchetypeDecal;
  using fl::widgets::effects::DecalAnimationKind;

  CHECK(ArchetypeDecal{DecalAnimationKind::Sweep, 32, 8}.duration_seconds() ==
        2.0F);
  CHECK(ArchetypeDecal{DecalAnimationKind::Cleanse, 32, 8}.duration_seconds() ==
        2.0F);
  CHECK(ArchetypeDecal{DecalAnimationKind::Beam, 32, 8}.duration_seconds() ==
        1.0F);
}

TEST_CASE("Beam pass helper computes half-width moving beam spans",
          "[widgets][effects][decal][archetype]") {
  using fl::widgets::effects::beam_span_for_progress;
  using fl::widgets::effects::beam_visual_length;

  CHECK(beam_visual_length(40) == 20);
  CHECK(beam_visual_length(1) == 1);

  const auto start = beam_span_for_progress(40, 0.0F);
  CHECK(start.start == -19);
  CHECK(start.end == 0);

  const auto middle = beam_span_for_progress(40, 0.5F);
  CHECK(middle.end - middle.start + 1 == 20);
  CHECK(middle.start > start.start);
  CHECK(middle.end < 40);

  const auto done = beam_span_for_progress(40, 1.0F);
  CHECK(done.start == 40);
  CHECK(done.end == 59);
}

TEST_CASE("Projectile embeds at the right edge and holds before ending",
          "[widgets][effects][decal][archetype]") {
  using fl::widgets::effects::projectile_embed_x;
  using fl::widgets::effects::projectile_head_x_for_progress;
  using fl::widgets::effects::projectile_is_holding;

  CHECK(projectile_embed_x(24) == 23);
  CHECK(projectile_head_x_for_progress(24, 0.0F) < 0);
  CHECK(projectile_head_x_for_progress(24, 0.5F) < 23);
  CHECK(projectile_head_x_for_progress(24, 0.78F) == 23);
  CHECK(projectile_head_x_for_progress(24, 1.0F) == 23);
  CHECK_FALSE(projectile_is_holding(0.5F));
  CHECK(projectile_is_holding(0.9F));
}

TEST_CASE("Beam and projectile frames move through their render bounds",
          "[widgets][effects][decal][archetype]") {
  using fl::widgets::effects::ArchetypeDecal;
  using fl::widgets::effects::DecalAnimationKind;

  const ArchetypeDecal beam{DecalAnimationKind::Beam, 40, 7};
  CHECK(leftmost_visible_x(beam.render(0.0F)) == 0);
  CHECK(leftmost_visible_x(beam.render(0.5F)) > 0);
  CHECK(active_cells(beam.render(1.0F)) == 0);

  const ArchetypeDecal projectile{DecalAnimationKind::Projectile, 24, 7};
  CHECK(rightmost_visible_x(projectile.render(0.5F)) < 23);
  CHECK(rightmost_visible_x(projectile.render(0.9F)) == 23);
  CHECK(rightmost_visible_x(projectile.render(1.0F)) == 23);
}

TEST_CASE(
    "New non-FlameWave decal animations render Lospec500-colored active frames",
    "[widgets][effects][decal]") {
  using fl::widgets::effects::DecalAnimationKind;

  constexpr std::array<DecalAnimationKind, 6> kinds{
      DecalAnimationKind::Shock,      DecalAnimationKind::RocksFall,
      DecalAnimationKind::HolyNova,   DecalAnimationKind::BloodBloom,
      DecalAnimationKind::FrostCrack, DecalAnimationKind::VoidRipple};

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

TEST_CASE("Starfire decal converts rotating flame pixels into ASCII cells",
          "[widgets][effects][decal][starfire]") {
  using fl::widgets::effects::DecalAnimationKind;

  const auto animation = fl::widgets::effects::make_decal_animation(
      DecalAnimationKind::Starfire, 48, 12);

  REQUIRE(animation != nullptr);
  REQUIRE(animation->kind() == DecalAnimationKind::Starfire);
  REQUIRE(animation->name() ==
          fl::widgets::effects::name(DecalAnimationKind::Starfire));

  const auto early = animation->render(0.15F);
  const auto late = animation->render(0.75F);
  const auto repeat = animation->render(0.75F);

  REQUIRE(early.width == 48);
  REQUIRE(early.height == 12);
  REQUIRE(active_cells(early) > 20);
  REQUIRE(visible_glyphs(early) > 10);
  REQUIRE(glyph_count(early, '@') < active_cells(early) / 3);
  REQUIRE(has_non_lospec_color(early));
  REQUIRE(has_non_lospec_color(late));

  REQUIRE(late.cells.size() == repeat.cells.size());
  bool saw_motion = false;
  for (std::size_t i = 0; i < late.cells.size(); ++i) {
    CAPTURE(i);
    REQUIRE(late.cells[i].glyph == repeat.cells[i].glyph);
    REQUIRE(late.cells[i].alpha == repeat.cells[i].alpha);
    REQUIRE(late.cells[i].fg.has_value() == repeat.cells[i].fg.has_value());
    REQUIRE(late.cells[i].bg.has_value() == repeat.cells[i].bg.has_value());
    if (early.cells[i].glyph != late.cells[i].glyph ||
        early.cells[i].alpha != late.cells[i].alpha) {
      saw_motion = true;
    }
  }
  REQUIRE(saw_motion);
}

TEST_CASE("Starfire progress is quantized over an integer frame count",
          "[widgets][effects][decal][starfire]") {
  fl::widgets::effects::Starfire animation(32, 10, 5);

  const auto frame_zero = animation.render(0.0F);
  const auto still_zero = animation.render(0.124F);
  const auto frame_one = animation.render(0.126F);

  REQUIRE(same_frame(frame_zero, still_zero));
  REQUIRE_FALSE(same_frame(frame_zero, frame_one));
}

TEST_CASE("Starfire final progress renders the loop's last frame",
          "[widgets][effects][decal][starfire]") {
  fl::widgets::effects::Starfire animation(32, 10, 9);

  const auto final_frame = animation.render(1.0F);
  const auto final_repeat = animation.render(1.2F);
  const auto before_final = animation.render(0.93F);

  REQUIRE(same_frame(final_frame, final_repeat));
  REQUIRE_FALSE(same_frame(final_frame, before_final));
  REQUIRE(active_cells(final_frame) > 20);
}

TEST_CASE("PoisonCloud decal renders active RGB fog",
          "[widgets][effects][decal][poison_cloud]") {
  using fl::widgets::effects::DecalAnimationKind;

  const auto animation = fl::widgets::effects::make_decal_animation(
      DecalAnimationKind::PoisonCloud, 40, 8);

  REQUIRE(animation != nullptr);
  REQUIRE(animation->kind() == DecalAnimationKind::PoisonCloud);
  REQUIRE(animation->name() ==
          fl::widgets::effects::name(DecalAnimationKind::PoisonCloud));

  const auto frame = animation->render(0.5F);
  REQUIRE(frame.width == 40);
  REQUIRE(frame.height == 8);
  REQUIRE(active_cells(frame) > 0);
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
