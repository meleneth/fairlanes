#include "web_screen_bridge.hpp"

#include <mutex>
#include <optional>
#include <string>

#include <ftxui/component/event.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

namespace fl {

#ifdef __EMSCRIPTEN__
namespace {

constexpr int kWebKeyEnter = 1000;
constexpr int kWebKeyEscape = 1001;
constexpr int kWebKeyBackspace = 1002;
constexpr int kWebKeyTab = 1003;
constexpr int kWebKeyArrowUp = 1004;
constexpr int kWebKeyArrowDown = 1005;
constexpr int kWebKeyArrowLeft = 1006;
constexpr int kWebKeyArrowRight = 1007;
constexpr int kWebKeyPageUp = 1008;
constexpr int kWebKeyPageDown = 1009;
constexpr int kWebKeyHome = 1010;
constexpr int kWebKeyEnd = 1011;

constexpr int kWebModifierShift = 1 << 0;
constexpr int kWebActionKeyDown = 1;

std::mutex g_web_screen_mutex;
ftxui::ScreenInteractive *g_web_screen = nullptr;

std::optional<ftxui::Event> web_key_to_ftxui_event(int key, int modifiers) {
  if (key >= 0x20 && key <= 0x7e) {
    return ftxui::Event::Character(std::string(1, static_cast<char>(key)));
  }

  switch (key) {
  case kWebKeyEnter:
    return ftxui::Event::Return;
  case kWebKeyEscape:
    return ftxui::Event::Escape;
  case kWebKeyBackspace:
    return ftxui::Event::Backspace;
  case kWebKeyTab:
    if ((modifiers & kWebModifierShift) != 0) {
      return ftxui::Event::TabReverse;
    }
    return ftxui::Event::Tab;
  case kWebKeyArrowUp:
    return ftxui::Event::ArrowUp;
  case kWebKeyArrowDown:
    return ftxui::Event::ArrowDown;
  case kWebKeyArrowLeft:
    return ftxui::Event::ArrowLeft;
  case kWebKeyArrowRight:
    return ftxui::Event::ArrowRight;
  case kWebKeyPageUp:
    return ftxui::Event::PageUp;
  case kWebKeyPageDown:
    return ftxui::Event::PageDown;
  case kWebKeyHome:
    return ftxui::Event::Home;
  case kWebKeyEnd:
    return ftxui::Event::End;
  default:
    return std::nullopt;
  }
}

} // namespace
#endif

WebScreenRegistration::WebScreenRegistration(
    ftxui::ScreenInteractive &screen)
    : screen_(&screen) {
#ifdef __EMSCRIPTEN__
  std::scoped_lock lock(g_web_screen_mutex);
  g_web_screen = screen_;
#endif
}

WebScreenRegistration::~WebScreenRegistration() {
#ifdef __EMSCRIPTEN__
  std::scoped_lock lock(g_web_screen_mutex);
  if (g_web_screen == screen_) {
    g_web_screen = nullptr;
  }
#endif
}

#ifdef __EMSCRIPTEN__
extern "C" EMSCRIPTEN_KEEPALIVE void
fairlanes_web_key_event(int key, int modifiers, int action) {
  if (action != kWebActionKeyDown) {
    return;
  }

  auto event = web_key_to_ftxui_event(key, modifiers);
  if (!event) {
    return;
  }

  ftxui::ScreenInteractive *screen = nullptr;
  {
    std::scoped_lock lock(g_web_screen_mutex);
    screen = g_web_screen;
  }

  if (screen != nullptr) {
    screen->PostEvent(*event);
  }
}
#endif

} // namespace fl
