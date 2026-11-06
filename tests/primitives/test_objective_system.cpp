#include <catch2/catch_test_macros.hpp>

#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/effective_combat_stats.hpp"
#include "fl/events/story_bus.hpp"
#include "fl/generated/story_content.hpp"
#include "fl/grand_central.hpp"
#include "fl/primitives/objective_system.hpp"

TEST_CASE("First Visitor defeat reveals the pub recruitment objective", "[story][objective]") {
  fl::GrandCentral game{1, 1, 0, true, true, 42, true};
  auto* story = game.objectives();
  REQUIRE(story != nullptr);
  auto& party = game.accounts().front().parties().front();
  REQUIRE(party.members().empty());
  REQUIRE_FALSE(story->active("recruit_first_hero"));
  story->emit("recruit_angry_fellow");
  REQUIRE(party.members().empty());

  game.innervate_event_system();
  game.advance_beat();
  REQUIRE(story->active("recruit_first_hero"));
  REQUIRE_FALSE(story->completed("recruit_first_hero"));
  REQUIRE(story->can_talk("angry_fellow"));
  story->emit("ask_about_visitor");
  REQUIRE(story->unhandled_events() == std::vector<std::string>{"ask_about_visitor"});

  story->emit("recruit_angry_fellow");
  REQUIRE(story->completed("recruit_first_hero"));
  REQUIRE_FALSE(story->can_talk("angry_fellow"));
  REQUIRE(party.members().size() == 1);
  const auto hero = party.members().front().member_id();
  REQUIRE(game.reg().all_of<fl::ecs::components::Stats, fl::ecs::components::PartyMember>(hero));
  story->emit("recruit_angry_fellow");
  REQUIRE(party.members().size() == 1);

  for (int beat = 0; beat < 12 && !party.has_encounter(); ++beat) game.advance_beat();
  REQUIRE(party.has_encounter());
  REQUIRE(party.tower_route()->floor == 1);
  REQUIRE(party.encounter_data().attackers().members().size() == 1);
}

TEST_CASE("Ruby story event status agrees with handwritten handlers", "[story][content]") {
  for (const auto& event : fl::story::generated_content::kEvents)
    REQUIRE(event.handled == fl::primitives::ObjectiveSystem::supports(event.id));
  REQUIRE(fl::story::generated_content::find_dialogue("angry_fellow") != nullptr);
  REQUIRE(fl::story::generated_content::find_dialogue("second_volunteer") != nullptr);
}

TEST_CASE("Two starter wins lead to a village purchase and second volunteer",
          "[story][progression]") {
  fl::GrandCentral game{1, 1, 0, true, true, 7, true};
  auto* story = game.objectives();
  auto& party = game.accounts().front().parties().front();
  game.innervate_event_system();
  game.advance_beat();
  story->emit("recruit_angry_fellow");
  REQUIRE(party.members().size() == 1);
  int beats = 0;
  while (!party.story_shop_pending() && beats < 5000) {
    game.advance_beat();
    ++beats;
  }
  REQUIRE(party.tower_route()->floor == 3);
  REQUIRE(party.story_shop_pending());
  REQUIRE(party.darrin_marks() == 10);
  for (int beat = 0; beat < 10 && !party.has_encounter(); ++beat) game.advance_beat();
  REQUIRE(party.has_encounter());
  REQUIRE(party.tower_route()->floor == 3);
  REQUIRE(game.accounts().front().normal_party_battle_mode());
  party.return_to_town();
  REQUIRE(story->can_buy_village_guard_hide());
  story->request_purchase("village_guard_hide");
  REQUIRE(party.darrin_marks() == 0);
  REQUIRE(fl::ecs::systems::effective_combat_stats(game.reg(), party.members().front().member_id())
              .armor >= 1);
  story->request_purchase("village_guard_hide");
  REQUIRE(party.darrin_marks() == 0);
  REQUIRE(story->active("recruit_second_hero"));
  REQUIRE(story->can_talk("second_volunteer"));
  story->emit("recruit_second_hero");
  REQUIRE(party.members().size() == 2);
  REQUIRE(story->completed("recruit_second_hero"));
}

TEST_CASE("Autoplay requests publish the accepted state", "[story][autoplay]") {
  fl::GrandCentral game{1, 1, 0, true, true, 7, true};
  auto* story = game.objectives();
  std::vector<bool> changed;
  fl::events::ScopedStoryListener listener{
      story->bus(), std::in_place_type<fl::events::AutoplayChanged>,
      [&changed](const fl::events::AutoplayChanged& event) { changed.push_back(event.enabled); }};
  story->request_autoplay(true);
  REQUIRE(story->autoplay_enabled());
  story->request_autoplay(false);
  REQUIRE_FALSE(story->autoplay_enabled());
  REQUIRE(changed == std::vector<bool>{true, false});
}

TEST_CASE("Autoplay follows the Ruby-authored opening path on production beats",
          "[story][autoplay]") {
  fl::GrandCentral game{1, 1, 0, true, true, 7, true, true};
  auto* story = game.objectives();
  REQUIRE(story->autoplay_enabled());
  game.innervate_event_system();
  int beats = 0;
  while (!story->completed("recruit_second_hero") && beats < 5000) {
    game.advance_beat();
    ++beats;
  }
  REQUIRE(story->autoplay_step_index() ==
          fl::story::generated_content::kAutoplayRoutes.front().steps.size());
  REQUIRE(story->completed("recruit_first_hero"));
  REQUIRE(story->completed("recruit_second_hero"));
  REQUIRE(game.accounts().front().parties().front().members().size() == 2);
}

TEST_CASE("The story party clears the physical slice and can return to Woodland ten",
          "[story][tower][physical-slice]") {
  fl::GrandCentral game{1, 1, 0, true, true, 7, true, true};
  auto* story = game.objectives();
  auto& party = game.accounts().front().parties().front();
  REQUIRE_FALSE(party.select_tower("origin_plains_tower"));
  game.innervate_event_system();

  int beats = 0;
  while (party.latest_tower_floor("origin_woodland_tower") < 7 && beats < 40000) {
    game.advance_beat();
    ++beats;
  }
  REQUIRE(party.latest_tower_floor("origin_woodland_tower") == 7);
  REQUIRE_FALSE(std::ranges::any_of(party.members(), [](const auto& member) {
    return member.grimoire().knows(fl::skills::SkillId::Brace);
  }));

  while (party.latest_tower_floor("origin_woodland_tower") < 10 && beats < 40000) {
    game.advance_beat();
    ++beats;
  }
  REQUIRE(story->completed("recruit_second_hero"));
  REQUIRE(party.members().size() == 3);
  REQUIRE(std::ranges::any_of(party.members(), [](const auto& member) {
    return member.grimoire().knows(fl::skills::SkillId::Brace);
  }));
  REQUIRE(party.latest_tower_floor("origin_woodland_tower") == 10);
  REQUIRE_FALSE(party.tower_choice_pending());
  REQUIRE(party.select_tower("origin_plains_tower"));

  while (party.latest_tower_floor("origin_plains_tower") < 4 && beats < 80000) {
    game.advance_beat();
    ++beats;
  }
  REQUIRE(party.latest_tower_floor("origin_plains_tower") == 4);
  REQUIRE(party.select_tower("origin_woodland_tower", 10));
  REQUIRE(party.tower_route()->biome_id == "origin_woodland_tower");
  REQUIRE(party.tower_route()->floor == 10);
}
