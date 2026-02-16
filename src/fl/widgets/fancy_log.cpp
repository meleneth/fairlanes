#include <algorithm>
#include <cstddef>
#include <utility>

#include "fancy_log.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fmt/format.h"

namespace fl::widgets {
using namespace ftxui;

// ---- construction ----------------------------------------------------------

FancyLog::FancyLog() : FancyLog(Options{}) {}

ftxui::Decorator on_not_black(uint8_t r, uint8_t g, uint8_t b) {
  return color(Color(r, g, b)) | color(Color(16, 18, 28));
}

std::string FancyLog::name_tag_for(entt::handle target) {
  using fl::ecs::components::PartyMember;
  using fl::ecs::components::Stats;
  auto &target_stats = target.get<Stats>();
  if (target.any_of<PartyMember>()) {
    return fmt::format("[player_name]({})", target_stats.name_);
  } else {
    return fmt::format("[enemy_name]({})", target_stats.name_);
  }
}

FancyLog::FancyLog(Options opt) : opts(opt) {
  // sane defaults you can override with styles(...)

  style_map = {{"player_name", on_not_black(98, 164, 119)},
               {"enemy_name", on_not_black(154, 77, 118)},
               {"xp", on_not_black(109, 234, 214)},
               {"level", on_not_black(247, 243, 183)},
               {"error", on_not_black(236, 39, 63)},
               {"warn", on_not_black(222, 93, 58)},
               {"ability", on_not_black(0, 139, 139)},
               {"ok", on_not_black(90, 181, 82)},
               {"orange", on_not_black(243, 168, 51)},
               {"green", on_not_black(0, 139, 139)},
               {"blue", on_not_black(109, 234, 214)},
               {"yellow", on_not_black(247, 243, 183)},
               {"red", on_not_black(250, 110, 121)},
               {"bravo", on_not_black(109, 234, 214)},
               {"hint", dim},
               {"black", on_not_black(16, 18, 28)}};
}
// ---- append ---------------------------------------------------------------

void FancyLog::append_markup(std::string_view utf8_line) {
  // spdlog::info(utf8_line);
  push(parse_markup(utf8_line));
}

void FancyLog::append(Element el) { push(std::move(el)); }

// ---- styles ---------------------------------------------------------------

void FancyLog::style(std::string tag, Decorator deco) {
  style_map[std::move(tag)] = deco;
}

void FancyLog::styles(StyleMap map) { style_map = std::move(map); }

void FancyLog::clear_styles() { style_map.clear(); }

// ---- housekeeping ---------------------------------------------------------

void FancyLog::clear() { log.clear(); }

// ---- sizing / scrolling ---------------------------------------------------

void FancyLog::set_max_rows(int rows) { opts.max_rows = std::max(0, rows); }

int FancyLog::max_rows() const { return opts.max_rows; }

void FancyLog::set_max_entries(size_t n) {
  opts.max_entries = n == 0 ? 1 : n;
  // downsize immediately if needed
  while (log.size() > opts.max_entries)
    log.pop_front();
}

size_t FancyLog::max_entries() const { return opts.max_entries; }

void FancyLog::set_autoscroll(bool v) { opts.autoscroll = v; }
bool FancyLog::autoscroll() const { return opts.autoscroll; }

size_t FancyLog::size() const { return log.size(); }
bool FancyLog::empty() const { return log.empty(); }

// ---- ComponentBase --------------------------------------------------------
ftxui::Element FancyLog::Render() {
  using namespace ftxui;

  // Build full content (or you can still cap entries via max_entries, which you
  // already do).
  Element content = vbox(Elements(log.begin(), log.end()));

  // Make it scrollable and pin view to bottom (tail-follow).
  // focusPositionRelative(1.0f) biases the frame toward the end.
  content =
      content | yframe | focusPositionRelative(0.0f, 1.0f) | vscroll_indicator;

  // Now enforce "max_rows" as a MAX height, not a baked-in height.
  // if (opts.max_rows > 0) {
  //  content = content | ftxui::size(HEIGHT, LESS_THAN, opts.max_rows);
  //}

  // Let parent allocate space.
  return content | flex;
}

// ---- internals ------------------------------------------------------------

// Parse "[tag](content)" segments; degrade gracefully on malformed input.
Element FancyLog::parse_markup(std::string_view s) const {
  std::vector<Element> parts;
  std::string plain;
  const size_t n = s.size();
  size_t i = 0;

  auto flush_plain = [&](std::string &buf) {
    if (!buf.empty()) {
      parts.push_back(text(buf));
      buf.clear();
    }
  };

  while (i < n) {
    size_t lb = s.find('[', i);
    if (lb == std::string_view::npos) {
      plain.append(s.substr(i));
      break;
    }

    // prefix plain
    plain.append(s.substr(i, lb - i));

    size_t rb = s.find(']', lb + 1);
    if (rb == std::string_view::npos) {
      // treat as literal
      plain.push_back(s[lb]);
      i = lb + 1;
      continue;
    }

    if (rb + 1 >= n || s[rb + 1] != '(') {
      plain.append(s.substr(lb, rb - lb + 1));
      i = rb + 1;
      continue;
    }

    size_t rp = s.find(')', rb + 2);
    if (rp == std::string_view::npos) {
      plain.append(s.substr(lb));
      i = n;
      break;
    }

    std::string tag{s.substr(lb + 1, rb - (lb + 1))};
    std::string content{s.substr(rb + 2, rp - (rb + 2))};

    flush_plain(plain);

    if (auto it = style_map.find(tag); it != style_map.end()) {
      parts.push_back(text(content) | it->second);
    } else {
      parts.push_back(text(content));
    }

    i = rp + 1;
  }

  flush_plain(plain);

  if (parts.empty())
    return text("");
  if (parts.size() == 1)
    return std::move(parts.front());
  return hbox(std::move(parts));
}

void FancyLog::push(Element el) {
  // append, enforce capacity
  log.push_back(std::move(el));
  if (log.size() > opts.max_entries)
    log.pop_front();

  // autoscroll is a no-op visually for now because we render only tail.
  // If you later add manual scrolling state, pin the cursor/index here.
}
} // namespace fl::widgets
