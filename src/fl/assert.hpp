#pragma once

#include <cstdlib>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <fmt/core.h>
#include <fmt/format.h>

namespace fl {

// -----------------------------------------------------------------------------
// Build switch
//   - Define FL_ENABLE_ASSERTS=1 in dev/debug
//   - Define FL_ENABLE_ASSERTS=0 in release
// If you don't define it, we default to !NDEBUG.
// -----------------------------------------------------------------------------
#ifndef FL_ENABLE_ASSERTS
#if !defined(NDEBUG)
#define FL_ENABLE_ASSERTS 1
#else
#define FL_ENABLE_ASSERTS 0
#endif
#endif

inline constexpr bool kEnableAsserts = (FL_ENABLE_ASSERTS != 0);

// -----------------------------------------------------------------------------
// Sink (swap to spdlog/Tracy later if you want).
// -----------------------------------------------------------------------------
[[noreturn]] inline void
fail(std::string_view msg,
     std::source_location loc = std::source_location::current()) {

  fmt::print(stderr, "ASSERTION FAILED\n  {}:{}\n  {}\n  {}\n", loc.file_name(),
             loc.line(), loc.function_name(), msg);

  std::fflush(stderr);
  std::abort();
}

// -----------------------------------------------------------------------------
// Lane 1: cheap + pretty
// - fmt format string is compile-time checked.
// - BUT args are evaluated at the call site (so keep them cheap).
// -----------------------------------------------------------------------------
template <typename... Args>
inline void assert_fmt(bool condition, std::source_location loc,
                       fmt::format_string<Args...> fmt_str, Args &&...args) {

  if constexpr (kEnableAsserts) {
    if (!condition) {
      fl::fail(fmt::format(fmt_str, std::forward<Args>(args)...), loc);
    }
  } else {
    (void)condition;
    (void)loc;
    (void)fmt_str;
  }
}

// Message-only helper
inline void
assert_msg(bool condition, std::string_view msg = "assertion failed",
           std::source_location loc = std::source_location::current()) {

  if constexpr (kEnableAsserts) {
    if (!condition)
      fl::fail(msg, loc);
  } else {
    (void)condition;
    (void)msg;
    (void)loc;
  }
}

// -----------------------------------------------------------------------------
// Lane 2: truly zero work in release
// - The callable is NOT invoked unless asserts are enabled and condition fails.
// - Use this when message/args are expensive or have side effects.
// -----------------------------------------------------------------------------
template <class MsgFn>
inline void
assert_lazy(bool condition, MsgFn &&make_msg,
            std::source_location loc = std::source_location::current()) {

  if constexpr (kEnableAsserts) {
    if (!condition) {
      // Accept return types: std::string, std::string_view, const char*, etc.
      using R = decltype(std::forward<MsgFn>(make_msg)());
      if constexpr (std::is_convertible_v<R, std::string_view>) {
        fl::fail(std::string_view(std::forward<MsgFn>(make_msg)()), loc);
      } else {
        fl::fail(std::string(std::forward<MsgFn>(make_msg)()), loc);
      }
    }
  } else {
    (void)condition;
    (void)make_msg;
    (void)loc;
  }
}

} // namespace fl
