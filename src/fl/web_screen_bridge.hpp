#pragma once

#include <ftxui/component/screen_interactive.hpp>

namespace fl {

class WebScreenRegistration {
public:
  explicit WebScreenRegistration(ftxui::ScreenInteractive &screen);
  ~WebScreenRegistration();

  WebScreenRegistration(const WebScreenRegistration &) = delete;
  WebScreenRegistration &
  operator=(const WebScreenRegistration &) = delete;

private:
  ftxui::ScreenInteractive *screen_{nullptr};
};

} // namespace fl
