#pragma once
// ready_queue.hpp

#include <algorithm>
#include <cstdint>
#include <deque>
#include <vector>

namespace seerin::enc {

using CombatantId = std::uint32_t;

class ReadyQueue {
public:
  bool contains(CombatantId who) const {
    return std::find(q_.begin(), q_.end(), who) != q_.end();
  }

  void push_back_unique(CombatantId who) {
    if (!contains(who))
      q_.push_back(who);
  }

  bool empty() const { return q_.empty(); }

  CombatantId front() const { return q_.front(); }

  bool pop_front_if(CombatantId who) {
    if (q_.empty())
      return false;
    if (q_.front() != who)
      return false;
    q_.pop_front();
    return true;
  }

  void remove(CombatantId who) {
    q_.erase(std::remove(q_.begin(), q_.end(), who), q_.end());
  }

  std::vector<CombatantId> snapshot() const {
    return std::vector<CombatantId>(q_.begin(), q_.end());
  }

private:
  std::deque<CombatantId> q_{};
};

} // namespace seerin::enc
