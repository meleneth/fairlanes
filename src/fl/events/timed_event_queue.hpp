// timed_event_queue.hpp
#pragma once

#include <chrono>
#include <functional>
#include <utility>

#include <eventpp/eventqueue.h>
#include <eventpp/utilities/orderedqueuelist.h>

namespace fl::events {
struct TimerEvent {
  using Clock = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;

  TimePoint when;
  std::function<void()> fn;
};

struct TimerCompare {
  template <typename Item> bool operator()(const Item &a, const Item &b) const {
    const auto &ea = a.template getArgument<0>();
    const auto &eb = b.template getArgument<0>();
    return ea.when < eb.when;
  }
};

struct TimerPolicy {
  template <typename Item>
  using QueueList = eventpp::OrderedQueueList<Item, TimerCompare>;

  static int getEvent(const TimerEvent &) { return 0; }
};

using TimerQueueImpl =
    eventpp::EventQueue<int, void(const TimerEvent &), TimerPolicy>;

class TimedEventQueue {
public:
  using Clock = TimerEvent::Clock;
  using Duration = Clock::duration;
  using TimePoint = Clock::time_point;
  using NowFn = std::function<TimePoint()>;

  explicit TimedEventQueue(NowFn now = [] { return Clock::now(); })
      : now_(std::move(now)), last_time_(now_()) {
    queue_.appendListener(0, [](const TimerEvent &ev) {
      if (ev.fn)
        ev.fn();
    });
  }

  void schedule_in(Duration delay, std::function<void()> fn) {
    queue_.enqueue(TimerEvent{now_() + delay, std::move(fn)});
  }

  void schedule_at(TimePoint when, std::function<void()> fn) {
    queue_.enqueue(TimerEvent{when, std::move(fn)});
  }

  void process_events(Duration dt) {
    const TimePoint window_end = last_time_ + dt;
    last_time_ = window_end;

    queue_.processIf(
        [&](const TimerEvent &ev) { return ev.when <= window_end; });
  }

  bool empty() const { return queue_.emptyQueue(); }

private:
  NowFn now_;
  TimerQueueImpl queue_;
  TimePoint last_time_;
};
} // namespace fl::events
