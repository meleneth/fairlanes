#pragma once

#include <chrono>
#include <cstddef>
#include <deque>
#include <string>
#include <vector>

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
  enum class ClearReason { Manual, Automatic };
  struct ClearedNotice {
    Notice notice;
    Clock::time_point enqueued_at;
    Clock::time_point shown_at;
    Clock::time_point cleared_at;
    ClearReason reason{ClearReason::Manual};
  };

  void enqueue(Notice notice, Clock::time_point now = Clock::now());
  void set_auto_acknowledge(bool enabled, Clock::time_point now = Clock::now()) noexcept {
    if (enabled && !auto_acknowledge_ && !queue_.empty()) shown_at_ = now;
    auto_acknowledge_ = enabled;
  }
  bool active() const noexcept {
    return !queue_.empty();
  }
  const std::vector<ClearedNotice>& cleared_notices() const noexcept {
    return cleared_notices_;
  }
  void dismiss(Clock::time_point now = Clock::now());
  void tick(Clock::time_point now = Clock::now());
  ftxui::Element render(int viewport_height, Clock::time_point now = Clock::now()) const;

private:
  struct PendingNotice {
    Notice notice;
    Clock::time_point enqueued_at;
  };
  void clear_front(ClearReason reason, Clock::time_point now);
  static constexpr auto kRiseDuration = std::chrono::milliseconds{900};
  static constexpr auto kAutoAcknowledgeDelay = std::chrono::seconds{30};
  std::deque<PendingNotice> queue_;
  std::vector<ClearedNotice> cleared_notices_;
  Clock::time_point shown_at_{};
  bool auto_acknowledge_{false};
};

} // namespace fl::widgets
