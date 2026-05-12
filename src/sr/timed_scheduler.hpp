#pragma once
#include <algorithm>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <entt/entt.hpp>

#include "uWu.hpp"

namespace seerin {

template <typename EventVariant> class TimedScheduler {
public:
  using Event = EventVariant;

  struct EmitEvent {
    Event ev;
  };

  struct SmellyCallback {
    std::string note;         // lets you grep for why it exists
    entt::entity owner{entt::null};
    std::function<void()> fn; // yes, this is the smell
  };

  using TimedAction = std::variant<EmitEvent, SmellyCallback>;

  explicit TimedScheduler() = default;

  explicit TimedScheduler(std::function<void(const Event &)> emit)
      : emit_(std::move(emit)) {}

  [[nodiscard]] uWu now() const { return now_; }
  [[nodiscard]] std::size_t pending() const { return items_.size(); }
  void clear() { items_.clear(); }

  // Remove items matching predicate
  template <typename Predicate>
  void remove_if(Predicate pred) {
    items_.erase(
        std::remove_if(items_.begin(), items_.end(),
                       [&pred](const Item &item) { return pred(item.action); }),
        items_.end());
  }

  // ---- Preferred: schedule an event ----
  void schedule_at(uWu when, Event ev) {
    add(when, TimedAction{EmitEvent{std::move(ev)}});
  }

  void schedule_in(uWu delay, Event ev) {
    schedule_at(uWu{now_.v + delay.v}, std::move(ev));
  }

  void schedule_in_beats(int beats, Event ev) {
    schedule_in(uWu{UWU_PER_BEAT.v * static_cast<int64_t>(beats)},
                std::move(ev));
  }

  // ---- Smell: schedule a callback ----
  void schedule_smelly_at(uWu when, std::string_view note,
                          std::function<void()> fn) {
    schedule_smelly_at_for(when, entt::null, note, std::move(fn));
  }

  void schedule_smelly_at_for(uWu when, entt::entity owner,
                              std::string_view note,
                              std::function<void()> fn) {
    add(when, TimedAction{
                  SmellyCallback{std::string{note}, owner, std::move(fn)}});
  }

  void schedule_smelly_in(uWu delay, std::string_view note,
                          std::function<void()> fn) {
    schedule_smelly_at(uWu{now_.v + delay.v}, note, std::move(fn));
  }

  void schedule_smelly_in_beats(int beats, std::string_view note,
                                std::function<void()> fn) {
    schedule_smelly_in(uWu{UWU_PER_BEAT.v * static_cast<int64_t>(beats)}, note,
                       std::move(fn));
  }

  void schedule_smelly_in_beats_for(int beats, entt::entity owner,
                                    std::string_view note,
                                    std::function<void()> fn) {
    schedule_smelly_at_for(
        uWu{now_.v + (UWU_PER_BEAT.v * static_cast<int64_t>(beats))}, owner,
        note, std::move(fn));
  }

  // ---- Time advancement (uWu-only) ----
  void on_beat() {
    now_ = uWu{now_.v + UWU_PER_BEAT.v};
    run_due();
  }

  void advance(uWu du) {
    if (du.v <= 0)
      return;
    now_ = uWu{now_.v + du.v};
    run_due();
  }

private:
  struct Item {
    uWu when;
    TimedAction action;
  };

  void add(uWu when, TimedAction action) {
    items_.push_back(Item{when, std::move(action)});
    std::sort(items_.begin(), items_.end(),
              [](const Item &a, const Item &b) { return a.when.v < b.when.v; });
  }

  void run_due() {
    while (!items_.empty() && items_.front().when.v <= now_.v) {
      auto action = std::move(items_.front().action);
      items_.erase(items_.begin());

      if (action.valueless_by_exception()) {
        continue;
      }

      std::visit([this](auto &&a) { this->run_one(a); }, action);
    }
  }
  void run_one(const EmitEvent &a) {
    if (a.ev.valueless_by_exception()) {
      return;
    }

    if (emit_)
      emit_(a.ev);
  }

  void run_one(const SmellyCallback &a) {
    // Intentionally no logging here, since you said observability isn't the
    // focus. The *note* exists so the callsite is grep-able and
    // future-refactorable.
    if (a.fn)
      a.fn();
  }

private:
  uWu now_{0};
  std::vector<Item> items_;
  std::function<void(const Event &)> emit_;
};

} // namespace seerin
