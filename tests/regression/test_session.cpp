#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <ranges>
#include <vector>

#include "fl/generated/regression_content.hpp"
#include "fl/grand_central.hpp"
#include "fl/primitives/world_clock.hpp"
#include "fl/regression/session.hpp"
#include "fl/widgets/party_battle_screen.hpp"

namespace {

nlohmann::json completed_recording(int overdrive, bool render_view = false) {
  fl::regression::Session session{{"woodland_mouse_duel", 42, 1200}};
  session.game().world_clock().set_beat_rate_multiplier(overdrive);
  fl::widgets::PartyBattleScreen view{session.game().account_context(0), 0};
  while (!session.finished()) {
    session.advance_beat();
    if (session.beats() == 80) {
      session.game().world_clock().set_beat_rate_multiplier(8);
    }
    if (render_view && session.beats() % 12 == 0) {
      (void)view.Render();
    }
  }
  REQUIRE(session.outcome() == "hero_victory");
  return session.recording();
}

} // namespace

TEST_CASE("Seeded regression fights repeat their ordered events", "[regression][determinism]") {
  fl::regression::Session compact{{"woodland_mouse_duel", 42, 1200}};
  while (!compact.finished()) compact.advance_beat();
  REQUIRE(compact.summary().at("schema") == "fairlanes-regression-run-summary");
  REQUIRE(compact.summary().at("version") == 1);
  REQUIRE(compact.summary().at("seed") == 42);
  REQUIRE(compact.summary().at("outcome") == "hero_victory");
  REQUIRE(compact.recording().at("version") == 1);

  const auto first = compact.recording();
  const auto second = completed_recording(1);
  REQUIRE(first.at("events") == second.at("events"));
  REQUIRE(first.at("result") == second.at("result"));
  REQUIRE(first.at("events").back().at("type") == "end");
  REQUIRE(first.at("events").back().at("outcome") == "hero_victory");
}

TEST_CASE("Overdrive changes pacing without changing gameplay events", "[regression][overdrive]") {
  REQUIRE(completed_recording(1).at("events") == completed_recording(8).at("events"));
}

TEST_CASE("Rendering the combat view does not consume gameplay randomness",
          "[regression][presentation]") {
  REQUIRE(completed_recording(1).at("events") == completed_recording(1, true).at("events"));
}

TEST_CASE("Beat cap is recorded separately from a loss and divergence is located",
          "[regression][recording]") {
  fl::regression::Session capped{{"woodland_mouse_duel", 42, 1}};
  capped.advance_beat();
  REQUIRE(capped.finished());
  REQUIRE(capped.outcome() == "beat_cap");
  REQUIRE(capped.recording().at("result").at("outcome") == "beat_cap");

  auto changed = capped.recording();
  changed["events"][0]["outcome"] = "mouse_victory";
  const auto divergence = capped.first_divergence(changed);
  REQUIRE(divergence.has_value());
  REQUIRE(divergence->find("event 0") != std::string::npos);
}

TEST_CASE("Ruby floor ten consolidates early physical combat for the opening party",
          "[regression][generated]") {
  const auto* scenario = fl::regression::generated_content::find("origin_woodland_floor10");
  REQUIRE(scenario != nullptr);
  REQUIRE(scenario->seeds.size() == 8);
  fl::regression::Options offense{"origin_woodland_floor10", 1, 2400, 0, "offensive"};
  auto defense = offense;
  defense.variant = "defensive";
  fl::regression::Session first{offense};
  fl::regression::Session second{defense};
  const auto& base = first.recording().at("initial_state");
  const auto& defensive = second.recording().at("initial_state");
  REQUIRE(base.size() == 5);
  REQUIRE(base.at(2).at("name") == "Antler Stag");
  REQUIRE(base.at(3).at("name") == "Bark Smack");
  REQUIRE(base.at(4).at("name") == "Bumpkin Hare");
  REQUIRE(base.at(0).at("stats").at("armor") == 1);
  REQUIRE(base.at(0).at("gear") == nlohmann::json::array({"village_guard_hide"}));
  REQUIRE(base.at(0).at("loadout") == nlohmann::json::array({"thump/1"}));
  REQUIRE(defensive.at(0).at("loadout") == nlohmann::json::array({"brace/1"}));
  while (!first.finished()) first.advance_beat();
  while (!second.finished()) second.advance_beat();
  REQUIRE(first.outcome() == "hero_victory");
  REQUIRE(second.outcome() == "enemy_victory");
  REQUIRE(first.recording().at("result").at("beats") < second.recording().at("result").at("beats"));
  REQUIRE(second.recording().at("result").at("brace_consumptions") > 0);
  fl::regression::Session repeated{defense};
  while (!repeated.finished()) repeated.advance_beat();
  REQUIRE(repeated.recording().at("events") == second.recording().at("events"));
}

TEST_CASE("Brace learning persists into the next authored encounter and is selected normally",
          "[regression][generated][brace][learning][progression]") {
  fl::regression::Options learned{"origin_woodland_brace_progression", 1, 3200, 0, "learn_brace"};
  auto control = learned;
  control.variant = "no_brace";
  fl::regression::Session progression{learned};
  fl::regression::Session without_brace{control};
  while (!progression.finished()) progression.advance_beat();
  while (!without_brace.finished()) without_brace.advance_beat();

  const auto& recording = progression.recording();
  REQUIRE(recording.at("version") == 2);
  REQUIRE(recording.at("encounters").size() == 2);
  REQUIRE(recording.at("encounter_results").size() == 2);
  REQUIRE(recording.at("encounter_results").at(0).at("outcome") == "hero_victory");
  REQUIRE(recording.at("encounter_results").at(1).at("outcome") == "hero_victory");
  REQUIRE(std::ranges::any_of(recording.at("events"), [](const auto& event) {
    return event.value("type", "") == "skill_witnessed" && event.value("skill", "") == "brace/1";
  }));
  REQUIRE(recording.at("external_inputs").at(0).at("skill") == "brace/1");
  REQUIRE(
      std::ranges::find(recording.at("final_party_state").at(0).at("known_skills"), "brace/1") !=
      recording.at("final_party_state").at(0).at("known_skills").end());
  REQUIRE(recording.at("final_party_state").at(0).at("loadout") ==
          nlohmann::json::array({"brace/1"}));
  REQUIRE(recording.at("encounter_results").at(1).at("brace_applications") > 0);
  REQUIRE(recording.at("encounter_results").at(1).at("brace_consumptions") > 0);
  REQUIRE(recording.at("encounter_results").at(1).at("brace_mitigated_damage") > 0);
  REQUIRE(without_brace.recording().at("encounter_results").at(1).at("brace_applications") == 0);
  REQUIRE(without_brace.recording().at("encounter_results").at(1).at("brace_mitigated_damage") ==
          0);
  REQUIRE(without_brace.recording().at("external_inputs").at(0).at("skill") == "thump/1");
  REQUIRE(recording.at("result").at("beats") > without_brace.recording().at("result").at("beats"));
  REQUIRE(progression.summary().at("encounters").size() == 2);
  REQUIRE(progression.summary().at("encounters").at(1).at("brace_mitigated_hits") > 0);
  REQUIRE(progression.summary().at("encounters").at(1).at("brace_mitigated_damage") > 0);
  REQUIRE(progression.summary().at("party_state_before_encounters").size() == 2);

  fl::regression::Session repeated{learned};
  while (!repeated.finished()) repeated.advance_beat();
  REQUIRE(repeated.recording().at("events") == recording.at("events"));
  REQUIRE(repeated.recording().at("encounter_results") == recording.at("encounter_results"));
}

TEST_CASE("Floor fifty records Honey Badger Dire Bleed without changing its danger",
          "[regression][generated][dire-bleed]") {
  fl::regression::Session baseline{{"origin_woodland_honey_badger", 1, 4000, 0, "offensive"}};
  while (!baseline.finished()) baseline.advance_beat();
  const auto& initial = baseline.recording().at("initial_state");
  const auto& result = baseline.recording().at("result");
  REQUIRE(initial.size() == 3);
  REQUIRE(initial.at(2).at("name") == "Honey Badger");
  REQUIRE(result.at("eviscerate_casts") == 5);
  REQUIRE(result.at("bleed_applications") == 5);
  REQUIRE(result.at("bleed_ticks") == 12);
  REQUIRE(result.at("bleed_tick_damage") == 24);
  REQUIRE(result.at("bleed_deaths") == 1);
  REQUIRE(result.at("bleed_clears") == 4);
  REQUIRE(result.at("bleed_active_at_end") == 1);
  REQUIRE(result.at("longest_bleed_seconds") > 11.0);
  REQUIRE(result.at("survivors") == 1);
  REQUIRE(baseline.outcome() == "hero_victory");
}

TEST_CASE("Stag lesson requires equipped Observe and retains learned Brace",
          "[regression][generated][brace][learning]") {
  fl::regression::Options observed{"origin_woodland_stag_lesson", 1, 1200, 0, "observe_equipped"};
  auto control = observed;
  control.variant = "observe_unequipped";
  fl::regression::Session lesson{observed};
  fl::regression::Session without_observe{control};
  while (!lesson.finished()) lesson.advance_beat();
  while (!without_observe.finished()) without_observe.advance_beat();

  const auto stag_braces = [](const nlohmann::json& recording) {
    return std::ranges::count_if(recording.at("events"), [](const auto& event) {
      return event.value("type", "") == "action" && event.value("actor", "") == "enemy1" &&
             event.value("skill", "") == "brace/1";
    });
  };
  REQUIRE(stag_braces(lesson.recording()) == 1);
  REQUIRE(stag_braces(without_observe.recording()) == 1);

  const auto& learned_party = lesson.game().accounts().front().parties().front();
  const auto& control_party = without_observe.game().accounts().front().parties().front();
  REQUIRE(learned_party.members().front().grimoire().knows(fl::skills::SkillId::Brace));
  REQUIRE_FALSE(control_party.members().front().grimoire().knows(fl::skills::SkillId::Brace));
  REQUIRE(std::ranges::any_of(lesson.recording().at("events"), [](const auto& event) {
    return event.value("type", "") == "action" && event.value("actor", "") == "hero1" &&
           event.value("skill", "") == "brace/1";
  }));
}

TEST_CASE("Brace practice records application, spending, and ordinary follow-up",
          "[regression][generated][brace]") {
  fl::regression::Options braced{"origin_woodland_brace_practice", 1, 1600, 0, "braced"};
  auto unbraced_options = braced;
  unbraced_options.variant = "unbraced";
  fl::regression::Session with_brace{braced};
  fl::regression::Session without_brace{unbraced_options};
  while (!with_brace.finished()) with_brace.advance_beat();
  while (!without_brace.finished()) without_brace.advance_beat();

  const auto& events = with_brace.recording().at("events");
  REQUIRE(std::ranges::any_of(events, [](const auto& event) {
    return event.value("type", "") == "action" && event.value("actor", "") == "hero1" &&
           event.value("skill", "") == "brace/1";
  }));
  REQUIRE(std::ranges::any_of(events, [](const auto& event) {
    return event.value("type", "") == "state" && event.value("actor", "") == "hero1" &&
           event.at("after").at("statuses") == nlohmann::json::array({"Brace"});
  }));

  std::vector<int> braced_hits;
  std::vector<int> ordinary_hits;
  for (const auto& event : events)
    if (event.value("type", "") == "hit" && event.value("target", "") == "hero1")
      braced_hits.push_back(event.at("damage"));
  for (const auto& event : without_brace.recording().at("events"))
    if (event.value("type", "") == "hit" && event.value("target", "") == "hero1")
      ordinary_hits.push_back(event.at("damage"));
  REQUIRE(braced_hits.size() >= 2);
  REQUIRE_FALSE(ordinary_hits.empty());
  REQUIRE(braced_hits[0] < ordinary_hits[0]);
  REQUIRE(braced_hits[1] > braced_hits[0]);
}

TEST_CASE("Plains Cover recording preserves aim and resolves both hit targets",
          "[regression][generated][cover]") {
  fl::regression::Session session{{"origin_plains_cover_lesson", 1, 1600, 0, "cover"}};
  while (!session.finished()) session.advance_beat();
  const auto& events = session.recording().at("events");
  const auto charge = std::ranges::find_if(events, [](const auto& event) {
    return event.value("type", "") == "action" && event.value("actor", "") == "enemy2" &&
           event.value("skill", "") == "bumper_rush/1";
  });
  REQUIRE(charge != events.end());
  const auto charge_seq = charge->at("seq").get<std::size_t>();
  const auto before = [charge_seq](const auto& event) {
    return event.value("type", "") == "hit" && event.value("skill", "") == "pebble_spit/1" &&
           event.value("actor", "").starts_with("hero") && event.value("target", "") == "enemy2" &&
           event.at("seq").template get<std::size_t>() < charge_seq;
  };
  const auto after = [charge_seq](const auto& event) {
    return event.value("type", "") == "hit" && event.value("skill", "") == "pebble_spit/1" &&
           event.value("actor", "").starts_with("hero") && event.value("target", "") == "enemy1" &&
           event.at("seq").template get<std::size_t>() > charge_seq;
  };
  REQUIRE(std::ranges::any_of(events, before));
  REQUIRE(std::ranges::any_of(events, after));
  REQUIRE(std::ranges::any_of(events, [](const auto& event) {
    return event.value("type", "") == "action" && event.value("actor", "").starts_with("hero") &&
           event.value("target", "") == "enemy1" && event.value("skill", "") == "pebble_spit/1";
  }));
}
