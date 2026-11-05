#pragma once

#include <chrono>
#include <cstddef>
#include <deque>
#include <string>

#include <ftxui/dom/elements.hpp>

namespace fl::widgets {

// Presentation-only queue. Gameplay events enqueue notices; acknowledging one
// reveals the next without changing gameplay time or progression state.
class ImportantNoticeOverlay {
public:
  using Clock = std::chrono::steady_clock;

  struct Notice {
    std::string label;
    std::string message;
    std::size_t color_index{20};
  };

  void enqueue(Notice notice, Clock::time_point now = Clock::now());
  void set_auto_acknowledge(bool enabled, Clock::time_point now = Clock::now()) noexcept {
    if (enabled && !auto_acknowledge_ && !queue_.empty()) shown_at_ = now;
    auto_acknowledge_ = enabled;
  }
  bool active() const noexcept {
    return !queue_.empty();
  }
  void dismiss(Clock::time_point now = Clock::now());
  void tick(Clock::time_point now = Clock::now());
  ftxui::Element render(int viewport_height, Clock::time_point now = Clock::now()) const;

private:
  static constexpr auto kRiseDuration = std::chrono::milliseconds{900};
  static constexpr auto kAutoAcknowledgeDelay = std::chrono::seconds{60};
  std::deque<Notice> queue_;
  Clock::time_point shown_at_{};
  bool auto_acknowledge_{false};
};

} // namespace fl::widgets
