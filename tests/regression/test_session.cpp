#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

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

TEST_CASE("Seeded regression fights repeat their ordered events",
          "[regression][determinism]") {
  const auto first = completed_recording(1);
  const auto second = completed_recording(1);
  REQUIRE(first.at("events") == second.at("events"));
  REQUIRE(first.at("result") == second.at("result"));
  REQUIRE(first.at("events").back().at("type") == "end");
  REQUIRE(first.at("events").back().at("outcome") == "hero_victory");
}

TEST_CASE("Overdrive changes pacing without changing gameplay events",
          "[regression][overdrive]") {
  REQUIRE(completed_recording(1).at("events") ==
          completed_recording(8).at("events"));
}

TEST_CASE("Rendering the combat view does not consume gameplay randomness",
          "[regression][presentation]") {
  REQUIRE(completed_recording(1).at("events") ==
          completed_recording(1, true).at("events"));
}

TEST_CASE(
    "Beat cap is recorded separately from a loss and divergence is located",
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
