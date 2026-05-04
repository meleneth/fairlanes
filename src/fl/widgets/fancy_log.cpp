#include <algorithm>
#include <cstddef>
#include <utility>

#include "fancy_log.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/lospec500.hpp"
#include "fmt/format.h"

namespace fl::widgets {
using namespace ftxui;

// ---- construction ----------------------------------------------------------

FancyLog::FancyLog() : FancyLog(Options{}) {}

std::string FancyLog::name_tag_for(entt::handle target) {
  using fl::ecs::components::PartyMember;
  using fl::ecs::components::Stats;

  auto &target_stats = target.get<Stats>();

  if (target.any_of<PartyMember>()) {
    return fmt::format("[player_name]({})", target_stats.name_);
  }

  return fmt::format("[enemy_name]({})", target_stats.name_);
}

FancyLog::FancyLog(Options opt) : opts(opt) {
  style_map = {{"player_name", fl::lospec500::at(22)},
               {"enemy_name", fl::lospec500::at(34)},
               {"xp", fl::lospec500::at(29)},
               {"level", fl::lospec500::at(15)},
               {"error", fl::lospec500::at(4)},
               {"warn", fl::lospec500::at(6)},
               {"ability", fl::lospec500::at(21)},
               {"ok", fl::lospec500::at(19)},
               {"orange", fl::lospec500::at(8)},
               {"green", fl::lospec500::at(21)},
               {"blue", fl::lospec500::at(29)},
               {"yellow", fl::lospec500::at(15)},
               {"red", fl::lospec500::at(37)},
               {"bravo", fl::lospec500::at(29)},
               {"hint", dim},
               {"black", fl::lospec500::at(0)}};
}

// ---- append ---------------------------------------------------------------

void FancyLog::append_markup(std::string_view utf8_line) {
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
  Element content = vbox(Elements(log.begin(), log.end()));

  content =
      content | yframe | focusPositionRelative(0.0f, 1.0f) | vscroll_indicator;

  return content | flex;
}

// ---- internals ------------------------------------------------------------

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

    plain.append(s.substr(i, lb - i));

    size_t rb = s.find(']', lb + 1);
    if (rb == std::string_view::npos) {
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
  log.push_back(std::move(el));

  if (log.size() > opts.max_entries)
    log.pop_front();
}

} // namespace fl::widgets
