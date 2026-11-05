#include "important_notice_overlay.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include "fl/lospec500.hpp"

namespace fl::widgets {

void ImportantNoticeOverlay::enqueue(Notice notice, Clock::time_point now) {
  if (queue_.empty()) shown_at_ = now;
  queue_.push_back(std::move(notice));
}

void ImportantNoticeOverlay::dismiss(Clock::time_point now) {
  if (queue_.empty()) return;
  queue_.pop_front();
  if (!queue_.empty()) shown_at_ = now;
}

void ImportantNoticeOverlay::tick(Clock::time_point now) {
  if (auto_acknowledge_ && !queue_.empty() && now - shown_at_ >= kAutoAcknowledgeDelay)
    dismiss(now);
}

ftxui::Element ImportantNoticeOverlay::render(int viewport_height, Clock::time_point now) const {
  using namespace ftxui;
  if (queue_.empty()) return text("");

  const auto& notice = queue_.front();
  const auto elapsed = std::max(Clock::duration::zero(), now - shown_at_);
  const double progress = std::clamp(std::chrono::duration<double>(elapsed).count() /
                                         std::chrono::duration<double>(kRiseDuration).count(),
                                     0.0, 1.0);
  // Five rows contain the window, its text, and the acknowledgement hint.
  const int travel = std::max(0, viewport_height - 5);
  const int top_padding = static_cast<int>(std::lround((1.0 - progress) * travel));
  auto panel = window(text(" " + notice.label + " ") | bold,
                      vbox({paragraph(notice.message), separator(),
                            text("Space / Enter to continue") | dim})) |
               size(WIDTH, EQUAL, 46) | bgcolor(fl::lospec500::color_at(notice.color_index)) |
               color(fl::lospec500::color_at(0)) | clear_under;
  return vbox(
      {filler() | size(HEIGHT, EQUAL, top_padding), hbox({filler(), std::move(panel)}), filler()});
}

} // namespace fl::widgets
