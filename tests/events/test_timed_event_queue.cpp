// timed_event_queue.test.cpp
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <vector>

#include "fl/events/timed_event_queue.hpp"

using namespace std::chrono_literals;
using fl::events::TimedEventQueue;

struct FakeClock {
  using Clock = TimedEventQueue::Clock;
  TimedEventQueue::TimePoint t = TimedEventQueue::TimePoint{}; // epoch-like

  TimedEventQueue::TimePoint now() const { return t; }
  void set(TimedEventQueue::TimePoint tp) { t = tp; }
  void advance(TimedEventQueue::Duration d) { t += d; }
};

TEST_CASE("TimedEventQueue: does not fire events scheduled after the window",
          "[timers][eventpp]") {
  FakeClock fc;

  TimedEventQueue q([&] { return fc.now(); });

  int fired = 0;
  q.schedule_in(10ms, [&] { ++fired; });

  q.process_events(9ms);
  REQUIRE(fired == 0);

  q.process_events(0ms);
  REQUIRE(fired == 0);
}

TEST_CASE("TimedEventQueue: fires events whose time falls within the window",
          "[timers][eventpp]") {
  FakeClock fc;

  TimedEventQueue q([&] { return fc.now(); });

  int fired = 0;
  q.schedule_in(10ms, [&] { ++fired; });

  q.process_events(10ms);
  REQUIRE(fired == 1);

  // processed events should be removed; extra processing shouldn't re-fire
  q.process_events(100ms);
  REQUIRE(fired == 1);
}

TEST_CASE("TimedEventQueue: fires multiple events in chronological order",
          "[timers][eventpp]") {
  FakeClock fc;

  TimedEventQueue q([&] { return fc.now(); });

  std::vector<int> order;
  q.schedule_in(30ms, [&] { order.push_back(3); });
  q.schedule_in(10ms, [&] { order.push_back(1); });
  q.schedule_in(20ms, [&] { order.push_back(2); });

  q.process_events(30ms);

  REQUIRE(order.size() == 3);
  REQUIRE(order[0] == 1);
  REQUIRE(order[1] == 2);
  REQUIRE(order[2] == 3);
}

TEST_CASE("TimedEventQueue: boundary condition fires when when == window_end",
          "[timers][eventpp]") {
  FakeClock fc;
  TimedEventQueue q([&] { return fc.now(); });

  int fired = 0;
  q.schedule_in(10ms, [&] { ++fired; });

  q.process_events(10ms);
  REQUIRE(fired == 1);
}

TEST_CASE("TimedEventQueue: schedule_at uses absolute timepoints correctly",
          "[timers][eventpp]") {
  FakeClock fc;
  TimedEventQueue q([&] { return fc.now(); });

  int fired = 0;

  const auto t0 = fc.now();
  const auto t_fire = t0 + 25ms;

  q.schedule_at(t_fire, [&] { ++fired; });

  q.process_events(24ms);
  REQUIRE(fired == 0);

  q.process_events(1ms);
  REQUIRE(fired == 1);
}
