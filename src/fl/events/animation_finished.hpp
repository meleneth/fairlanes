#pragma once
#include <chrono>

#include <entt/entt.hpp>
#include <eventpp/eventdispatcher.h>

namespace fl::events {

// Event: animation completion
struct AnimationFinished {
  entt::entity entity;
  std::string animation_id; // optional but very useful for composition/debug
};

// Your bus could be eventpp::EventDispatcher<...> or your own wrapper.
// Sketch:
using AnimBus = eventpp::EventDispatcher<int, void(const AnimationFinished &)>;

} // namespace fl::events
