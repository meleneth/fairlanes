#pragma once
#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <entt/entt.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

/// FancyLog: styled append-only log for FTXUI with lightweight markup.
///
/// Markup example: "[name](Snail) uses [error](Slime Blast) 🔥"
namespace fl::widgets {

class FancyLog : public ftxui::ComponentBase {
public:
  using Element = ftxui::Element;
  using Decorator = ftxui::Decorator;
  using StyleMap = std::unordered_map<std::string, Decorator>;

  struct Options {
    int max_rows = 3;         // visual height (rows)
    size_t max_entries = 512; // ring-buffer size
    bool autoscroll = true;   // keep view pinned to end on append
  };

  // ---- construction --------------------------------------------------------
  FancyLog();
  explicit FancyLog(Options opt);

  // ---- append --------------------------------------------------------------
  // Parses "[tag](text)" markup; unknown tags become plain text.
  void append_markup(std::string_view utf8_line);

  // Adds plain text with optional decorators (folded left->right).
  template <typename... Ds> void append_plain(std::string_view utf8, Ds... ds) {
    using namespace ftxui;
    Element el = text(std::string{utf8});
    (void)std::initializer_list<int>{(el = el | ds, 0)...};
    push(std::move(el));
  }

  // Adds a prebuilt FTXUI element.
  void append(Element el);

  // ---- styles --------------------------------------------------------------
  void style(std::string tag, Decorator deco); // upsert a single style
  void styles(StyleMap map);                   // replace all styles
  void clear_styles();                         // remove all custom styles

  // ---- housekeeping --------------------------------------------------------
  void clear(); // remove all entries

  // ---- sizing / scrolling --------------------------------------------------
  void set_max_rows(int rows);
  int max_rows() const;

  void set_max_entries(size_t n);
  size_t max_entries() const;

  void set_autoscroll(bool v);
  bool autoscroll() const;

  size_t size() const; // current #entries
  bool empty() const;

  std::string name_tag_for(entt::handle target);

  // ---- ComponentBase -------------------------------------------------------
  bool OnEvent(ftxui::Event event) override;
  ftxui::Element Render() override;
  void set_focused(bool focused) noexcept;

private:
  // parsing
  ftxui::Element parse_markup(std::string_view utf8_line) const;

  // storage helpers
  void push(Element el);

  // data
  std::deque<Element> log;
  Options opts;
  StyleMap style_map;
  int cursor_{0};
  bool focused_{false};
};
} // namespace fl::widgets
