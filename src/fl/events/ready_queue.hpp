#include <deque>

#include <entt/entt.hpp>

namespace fl::events {

struct Decision {
  entt::entity actor{entt::null};  // who is acting
  entt::entity skill{entt::null};  // or SkillId, whatever your model is
  entt::entity target{entt::null}; // or a target-set handle
  std::uint64_t nonce{0};          // optional: correlates start/finish safely
};

class ReadyQueue {
public:
  void push(Decision d) { q_.push_back(std::move(d)); }

  bool empty() const { return q_.empty(); }
  Decision &front() { return q_.front(); }
  const Decision &front() const { return q_.front(); }
  std::size_t size() const { return q_.size(); }

  void pop_front() { q_.pop_front(); }

private:
  std::deque<Decision> q_;
};

} // namespace fl::events
