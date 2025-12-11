#pragma once

#define MARK_CLASS_MOVEONLY(T)                                                 \
  T(const T &) = delete;                                                       \
  T &operator=(const T &) = delete;                                            \
  T(T &&) = default;                                                           \
  T &operator=(T &&) = default;
