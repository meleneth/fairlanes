#pragma once
// bus.hpp

#include <eventpp/callbacklist.h>

namespace seerin {

template <typename EventVariant> class Bus {
public:
  using Callback = void(const EventVariant &);

  template <typename Fn> auto on(Fn &&fn) {
    return list_.append(std::forward<Fn>(fn));
  }

  void emit(const EventVariant &ev) { list_(ev); }

private:
  eventpp::CallbackList<Callback> list_{};
};

} // namespace seerin
