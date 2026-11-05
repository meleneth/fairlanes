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
  const auto first = completed_recording(1);
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

TEST_CASE("Ruby floor ten variants use the generated lineup and gear in combat",
          "[regression][generated]") {
  const auto* scenario = fl::regression::generated_content::find("origin_woodland_floor10");
  REQUIRE(scenario != nullptr);
  REQUIRE(scenario->seeds.size() == 8);
  fl::regression::Options arrival{"origin_woodland_floor10", 1, 2400, 0, "arrival"};
  fl::regression::Options detour = arrival;
  detour.variant = "detour";
  fl::regression::Session first{arrival};
  fl::regression::Session second{detour};
  const auto& base = first.recording().at("initial_state");
  const auto& upgraded = second.recording().at("initial_state");
  REQUIRE(base.size() == 10);
  REQUIRE(base.at(5).at("name") == "Honey Badger");
  REQUIRE(base.at(5).at("stats").at("max_hp") == 10);
  REQUIRE(base.at(5).at("stats").at("attack") == 2);
  REQUIRE(base.at(0).at("stats").at("armor") == 1);
  REQUIRE(upgraded.at(0).at("stats").at("armor") == 3);
  REQUIRE(upgraded.at(0).at("stats").at("max_hp") == 25);
  while (!first.finished()) first.advance_beat();
  while (!second.finished()) second.advance_beat();
  REQUIRE(first.outcome() == "hero_victory");
  REQUIRE(second.outcome() == "hero_victory");
  fl::regression::Session repeated{detour};
  while (!repeated.finished()) repeated.advance_beat();
  REQUIRE(repeated.recording().at("events") == second.recording().at("events"));
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
