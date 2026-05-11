#include <catch2/catch_test_macros.hpp>

#include <string>
#include <variant>
#include <vector>

#include "sr/timed_scheduler.hpp" // adjust include path
#include "sr/uWu.hpp"

namespace {

// A tiny event variant for tests.
struct EvtA {
  int v;
};
struct EvtB {
  std::string s;
};
using Ev = std::variant<EvtA, EvtB>;

template <typename T> int count_type(const std::vector<Ev> &out) {
  int n = 0;
  for (const auto &e : out) {
    if (std::holds_alternative<T>(e))
      ++n;
  }
  return n;
}

} // namespace

TEST_CASE("TimedScheduler: starts at 0 and has no pending work") {
  std::vector<Ev> out;

  seerin::TimedScheduler<Ev> sch([&](const Ev &e) { out.push_back(e); });

  REQUIRE(sch.now().v == 0);
  REQUIRE(sch.pending() == 0);
  REQUIRE(out.empty());
}

TEST_CASE("TimedScheduler: schedule_in does not emit until due") {
  std::vector<Ev> out;
  seerin::TimedScheduler<Ev> sch([&](const Ev &e) { out.push_back(e); });

  sch.schedule_in(seerin::uWu{10}, Ev{EvtA{1}});
  REQUIRE(sch.pending() == 1);
  REQUIRE(out.empty());

  sch.advance(seerin::uWu{9});
  REQUIRE(out.empty());
  REQUIRE(sch.pending() == 1);

  sch.advance(seerin::uWu{1});
  REQUIRE(sch.pending() == 0);
  REQUIRE(out.size() == 1);
  REQUIRE(std::get<EvtA>(out[0]).v == 1);
}

TEST_CASE("TimedScheduler: schedule_at emits when now >= when") {
  std::vector<Ev> out;
  seerin::TimedScheduler<Ev> sch([&](const Ev &e) { out.push_back(e); });

  sch.schedule_at(seerin::uWu{0}, Ev{EvtA{7}});
  REQUIRE(sch.pending() == 1);

  // Nothing runs until time advances or a beat happens.
  REQUIRE(out.empty());

  sch.advance(seerin::uWu{1});
  REQUIRE(out.size() == 1);
  REQUIRE(std::get<EvtA>(out[0]).v == 7);
  REQUIRE(sch.pending() == 0);
}

TEST_CASE("TimedScheduler: actions run in ascending time order regardless of "
          "insertion order") {
  std::vector<Ev> out;
  seerin::TimedScheduler<Ev> sch([&](const Ev &e) { out.push_back(e); });

  // Insert out of order.
  sch.schedule_at(seerin::uWu{30}, Ev{EvtA{3}});
  sch.schedule_at(seerin::uWu{10}, Ev{EvtA{1}});
  sch.schedule_at(seerin::uWu{20}, Ev{EvtA{2}});

  REQUIRE(sch.pending() == 3);

  sch.advance(seerin::uWu{100});
  REQUIRE(sch.pending() == 0);
  REQUIRE(out.size() == 3);
  REQUIRE(std::get<EvtA>(out[0]).v == 1);
  REQUIRE(std::get<EvtA>(out[1]).v == 2);
  REQUIRE(std::get<EvtA>(out[2]).v == 3);
}

TEST_CASE(
    "TimedScheduler: multiple items at same timestamp all fire when due") {
  std::vector<Ev> out;
  seerin::TimedScheduler<Ev> sch([&](const Ev &e) { out.push_back(e); });

  // Same "when" value.
  sch.schedule_at(seerin::uWu{50}, Ev{EvtA{1}});
  sch.schedule_at(seerin::uWu{50}, Ev{EvtA{2}});
  sch.schedule_at(seerin::uWu{50}, Ev{EvtB{"x"}});

  sch.advance(seerin::uWu{49});
  REQUIRE(out.empty());
  REQUIRE(sch.pending() == 3);

  sch.advance(seerin::uWu{1});
  REQUIRE(sch.pending() == 0);
  REQUIRE(out.size() == 3);
  REQUIRE(count_type<EvtA>(out) == 2);
  REQUIRE(count_type<EvtB>(out) == 1);
}

TEST_CASE(
    "TimedScheduler: on_beat advances by UWU_PER_BEAT and runs due work") {
  std::vector<Ev> out;
  seerin::TimedScheduler<Ev> sch([&](const Ev &e) { out.push_back(e); });

  // Due at 2 beats.
  sch.schedule_in_beats(2, Ev{EvtA{42}});

  REQUIRE(out.empty());
  sch.on_beat(); // now = 1 beat
  REQUIRE(out.empty());
  REQUIRE(sch.pending() == 1);

  sch.on_beat(); // now = 2 beats
  REQUIRE(out.size() == 1);
  REQUIRE(std::get<EvtA>(out[0]).v == 42);
  REQUIRE(sch.pending() == 0);
}

TEST_CASE("TimedScheduler: advance(<=0) is a no-op") {
  std::vector<Ev> out;
  seerin::TimedScheduler<Ev> sch([&](const Ev &e) { out.push_back(e); });

  sch.schedule_in(seerin::uWu{1}, Ev{EvtA{9}});

  sch.advance(seerin::uWu{0});
  REQUIRE(out.empty());
  REQUIRE(sch.pending() == 1);

  sch.advance(seerin::uWu{-5});
  REQUIRE(out.empty());
  REQUIRE(sch.pending() == 1);

  sch.advance(seerin::uWu{1});
  REQUIRE(out.size() == 1);
  REQUIRE(std::get<EvtA>(out[0]).v == 9);
}

TEST_CASE(
    "TimedScheduler: smelly callbacks fire when due and do not emit events") {
  std::vector<Ev> out;
  int called = 0;

  seerin::TimedScheduler<Ev> sch([&](const Ev &e) { out.push_back(e); });

  sch.schedule_smelly_in(seerin::uWu{10}, "test:smell", [&] { ++called; });
  sch.schedule_in(seerin::uWu{10}, Ev{EvtA{1}});

  sch.advance(seerin::uWu{10});
  REQUIRE(called == 1);
  REQUIRE(out.size() == 1);
  REQUIRE(std::get<EvtA>(out[0]).v == 1);
  REQUIRE(sch.pending() == 0);
}

TEST_CASE("TimedScheduler: null smelly callback is safe (does nothing)") {
  std::vector<Ev> out;
  seerin::TimedScheduler<Ev> sch([&](const Ev &e) { out.push_back(e); });

  sch.schedule_smelly_in(seerin::uWu{1}, "test:null-cb",
                         std::function<void()>{});
  sch.advance(seerin::uWu{1});

  REQUIRE(out.empty());
  REQUIRE(sch.pending() == 0);
}

TEST_CASE("TimedScheduler: can schedule more work from inside a callback") {
  std::vector<Ev> out;
  seerin::TimedScheduler<Ev> sch([&](const Ev &e) { out.push_back(e); });

  // At t=10, callback schedules an event at t=12.
  sch.schedule_smelly_at(seerin::uWu{10}, "test:schedule-inside", [&] {
    sch.schedule_at(seerin::uWu{12}, Ev{EvtA{99}});
  });

  sch.advance(seerin::uWu{10});
  REQUIRE(out.empty());
  REQUIRE(sch.pending() == 1); // the newly scheduled event

  sch.advance(seerin::uWu{2});
  REQUIRE(out.size() == 1);
  REQUIRE(std::get<EvtA>(out[0]).v == 99);
  REQUIRE(sch.pending() == 0);
}
