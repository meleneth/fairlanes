// variant_bus.hpp

#pragma once
#include <eventpp/callbacklist.h>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace seerin {

template <class T, class... Ts> struct type_index;

template <class T, class... Ts>
struct type_index<T, T, Ts...> : std::integral_constant<std::size_t, 0> {};

template <class T, class U, class... Ts>
struct type_index<T, U, Ts...>
    : std::integral_constant<std::size_t, 1 + type_index<T, Ts...>::value> {};

template <class T, class... Ts>
inline constexpr std::size_t type_index_v = type_index<T, Ts...>::value;

template <class Variant> class VariantBus;

template <class... Ts> class VariantBus<std::variant<Ts...>> {
public:
  using Variant = std::variant<Ts...>;

  template <class T> using List = eventpp::CallbackList<void(const T &)>;

  template <class T> class Subscription {
  public:
    using Handle = typename List<T>::Handle;

    Subscription() = default;
    Subscription(VariantBus &bus, Handle handle)
        : bus_(&bus), handle_(handle), connected_(true) {}

    Subscription(Subscription &&rhs) noexcept { *this = std::move(rhs); }

    Subscription &operator=(Subscription &&rhs) noexcept {
      if (this == &rhs) {
        return *this;
      }

      reset();
      bus_ = rhs.bus_;
      handle_ = rhs.handle_;
      connected_ = rhs.connected_;
      rhs.bus_ = nullptr;
      rhs.connected_ = false;
      return *this;
    }

    ~Subscription() { reset(); }

    void reset() {
      if (connected_ && bus_ != nullptr) {
        bus_->template callbacks<T>().remove(handle_);
      }

      bus_ = nullptr;
      connected_ = false;
    }

    Subscription(const Subscription &) = delete;
    Subscription &operator=(const Subscription &) = delete;

  private:
    VariantBus *bus_{nullptr};
    Handle handle_{};
    bool connected_{false};
  };

  template <class T> List<T> &callbacks() {
    static_assert((std::is_same_v<T, Ts> || ...),
                  "T is not in this VariantBus' variant");
    return std::get<type_index_v<T, Ts...>>(lists_);
  }

  template <class T, class F> auto on(F &&f) {
    return callbacks<T>().append(std::forward<F>(f));
  }

  template <class T, class F> Subscription<T> subscribe(F &&f) {
    return Subscription<T>{*this, on<T>(std::forward<F>(f))};
  }

  void emit(const Variant &v) {
    if (v.valueless_by_exception()) {
      return;
    }

    std::visit(
        [&](const auto &e) {
          using T = std::decay_t<decltype(e)>;
          callbacks<T>()(e); // <- invoke callback list
        },
        v);
  }

private:
  std::tuple<List<Ts>...> lists_;
};

} // namespace seerin
