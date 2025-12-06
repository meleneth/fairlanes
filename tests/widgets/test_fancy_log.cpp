// tests/fairlanes/widgets/fancy_log_tests.cpp
#include <catch2/catch_test_macros.hpp>

#include "fl/widgets/fancy_log.hpp"

using fl::widgets::FancyLog;

TEST_CASE("FancyLog starts empty", "[fancylog]") {
  FancyLog log;

  REQUIRE(log.empty());
  REQUIRE(log.size() == 0);
}

TEST_CASE("FancyLog appends plain lines", "[fancylog]") {
  FancyLog log;

  log.append_plain("hello");
  log.append_plain("world");

  REQUIRE_FALSE(log.empty());
  REQUIRE(log.size() == 2);
}

TEST_CASE("FancyLog respects max_entries as a ring buffer", "[fancylog]") {
  FancyLog::Options opt;
  opt.max_entries = 3;
  FancyLog log{opt};

  log.append_plain("a");
  log.append_plain("b");
  log.append_plain("c");
  REQUIRE(log.size() == 3);

  // This should not grow past max_entries.
  log.append_plain("d");
  REQUIRE(log.size() == 3);

  // We can't easily assert which ones got dropped without exposing internals,
  // but we CAN assert the size cap is obeyed and nothing explodes.
}

TEST_CASE("FancyLog clear empties the log", "[fancylog]") {
  FancyLog log;
  log.append_plain("one");
  log.append_plain("two");
  REQUIRE(log.size() == 2);

  log.clear();
  REQUIRE(log.empty());
  REQUIRE(log.size() == 0);
}

TEST_CASE("FancyLog append_markup parses without throwing", "[fancylog]") {
  FancyLog log;

  // Should accept plain text
  log.append_markup("hello world");

  // And some markup; exact styling is FTXUI business, just don't crash
  log.append_markup("[name](Snail) uses [error](Slime Blast) 🔥");

  REQUIRE(log.size() == 2);
}
