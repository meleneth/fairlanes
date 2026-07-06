#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <string_view>

#include <ftxui/component/event.hpp>

#include "fl/widgets/effect_gallery_view.hpp"
#include "fl/widgets/effects/decal.hpp"

namespace {

std::size_t effect_count() {
  return fl::widgets::effects::available_decal_animation_kinds().size();
}

std::string_view effect_name(std::size_t index) {
  const auto &effects = fl::widgets::effects::available_decal_animation_kinds();
  return fl::widgets::effects::name(effects[index % effects.size()]);
}

} // namespace

TEST_CASE("Effect gallery effect list is populated from decal catalog",
          "[widgets][effect-gallery]") {
  REQUIRE_FALSE(
      fl::widgets::effects::available_decal_animation_kinds().empty());
}

TEST_CASE("Effect gallery navigation wraps in both directions",
          "[widgets][effect-gallery]") {
  auto state = fl::widgets::EffectGalleryState{};
  const auto count = effect_count();
  REQUIRE(count > 1);

  state.current_effect_index = count - 1;
  state.next_effect(count);
  CHECK(state.current_effect_index == 0);

  state.previous_effect(count);
  CHECK(state.current_effect_index == count - 1);
}

TEST_CASE("Effect gallery labels expose previous current and next names",
          "[widgets][effect-gallery]") {
  const auto count = effect_count();
  REQUIRE(count > 2);

  auto state = fl::widgets::EffectGalleryState{};
  auto labels = state.labels();
  CHECK(labels.previous == effect_name(count - 1));
  CHECK(labels.current == effect_name(0));
  CHECK(labels.next == effect_name(1));

  state.current_effect_index = 1;
  labels = state.labels();
  CHECK(labels.previous == effect_name(0));
  CHECK(labels.current == effect_name(1));
  CHECK(labels.next == effect_name(2));

  state.current_effect_index = count - 1;
  labels = state.labels();
  CHECK(labels.previous == effect_name(count - 2));
  CHECK(labels.current == effect_name(count - 1));
  CHECK(labels.next == effect_name(0));
}

TEST_CASE("Effect gallery background navigation wraps in both directions",
          "[widgets][effect-gallery]") {
  auto state = fl::widgets::EffectGalleryState{};

  auto labels = state.background_labels();
  CHECK(labels.previous == "Bog");
  CHECK(labels.current == "Forest");
  CHECK(labels.next == "Savannah");

  state.previous_background(3);
  labels = state.background_labels();
  CHECK(labels.previous == "Savannah");
  CHECK(labels.current == "Bog");
  CHECK(labels.next == "Forest");

  state.next_background(3);
  labels = state.background_labels();
  CHECK(labels.previous == "Bog");
  CHECK(labels.current == "Forest");
  CHECK(labels.next == "Savannah");
}

TEST_CASE(
    "Effect gallery combatant count controls accept digits one through six",
    "[widgets][effect-gallery]") {
  auto state = fl::widgets::EffectGalleryState{};

  REQUIRE(state.set_combatant_count_from_digit('1'));
  CHECK(state.combatant_count == 1);

  REQUIRE(state.set_combatant_count_from_digit('6'));
  CHECK(state.combatant_count == 6);

  CHECK_FALSE(state.set_combatant_count_from_digit('x'));
  CHECK(state.combatant_count == 6);

  CHECK_FALSE(state.set_combatant_count_from_digit('0'));
  CHECK(state.combatant_count == 6);
}

TEST_CASE("Effect gallery widget handles navigation and count events",
          "[widgets][effect-gallery]") {
  auto gallery = fl::widgets::EffectGalleryView{};
  const auto count = effect_count();
  REQUIRE(count > 1);

  CHECK(gallery.state().current_effect_index == 0);
  REQUIRE(gallery.OnEvent(ftxui::Event::ArrowLeft));
  CHECK(gallery.state().current_effect_index == count - 1);

  REQUIRE(gallery.OnEvent(ftxui::Event::Character("l")));
  CHECK(gallery.state().current_effect_index == 0);

  REQUIRE(gallery.OnEvent(ftxui::Event::ArrowDown));
  CHECK(gallery.state().current_background_index == 1);

  REQUIRE(gallery.OnEvent(ftxui::Event::Character("k")));
  CHECK(gallery.state().current_background_index == 0);

  REQUIRE(gallery.OnEvent(ftxui::Event::Character("6")));
  CHECK(gallery.state().combatant_count == 6);

  CHECK_FALSE(gallery.OnEvent(ftxui::Event::Character("x")));
  CHECK(gallery.state().combatant_count == 6);
}
