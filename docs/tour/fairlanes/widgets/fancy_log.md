[FancyLog](https://github.com/meleneth/fairlanes/blob/89571fb5f3e6396d5e191a557602c2b25c79a31c/src/fl/widgets/fancy_log.cpp#L35) is the rich-text log used to present game output.

Its main append path is:

```cpp
// Parses "[tag](text)" markup; unknown tags are rendered as plain text.
void append_markup(std::string_view utf8_line);
```

This lets gameplay code write lightweight tagged text without needing to manually assemble styled UI output.

The important behavior is that unknown tags degrade gracefully: if the log does not understand a tag, it still shows the text instead of losing it.
