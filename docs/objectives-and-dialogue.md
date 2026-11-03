# Objectives and dialogue

Normal client play starts with one empty party. The day-zero Visitor has no
defenders and devastates the village. The game-wide Objective System observes
the raid result and reveals **Recruit a hero**. Press `o` to read objectives,
or `v` to visit the village. Enter the pub, speak to the angry fellow, and
choose **Then fight beside me**. Alderion joins through the ordinary ECS
member construction path with Thump ready to use.

After Enter leaves attract mode, a start screen offers **Manual story** and
**Autoplay** and explains that `a` toggles autoplay at any time during play.
Enabling Autoplay from either the start screen or `a` requires confirmation;
disabling it takes effect immediately. The enable request and resulting state
change travel through typed story events.
`--skip-chaos-attract` bypasses the choice screen; `--story-autoplay` starts
that route when attract mode is skipped. `--replay` takes its existing early
path and opens only the requested regression recording.

His first two encounters are one-on-one Field Mouse fights. Each clear earns
five Darrin-marks. After the second clear, combat pauses for the town visit.
Press `r` to return, then `v` to buy **Village Guard Hide** for ten marks.
The shop grants and equips the authored chest item through the normal inventory
and equipment events. This reveals **Meet the next volunteer**; speak to the
volunteer in the village to recruit Bramblethorn. The first two objectives are
the opening progression slice. A wider village economy and later objectives
remain to be authored.

`scripts/fairlanes_story_declarations.rb` defines stable story event IDs,
objectives, dialogue nodes, and choices. The existing content generator validates
references and renders ordinary C++ tables in `fl/generated/story_content.hpp`.
Generated [story events](tour/generated/story-events.md) list every declared
event and every dialogue event without a handwritten handler. The Ruby
`handled` flag is checked against the C++ dispatcher by a test. Choices emit
typed `StoryFact` bus events; the handwritten Objective System owns recruitment,
shop effects, and objective state. The currently unhandled `ask_about_visitor`
event is recorded as a fact but has no gameplay effect.

The DSL also declares an `origin_opening` autoplay route. Its generated table
annotates each required fact and the next dialogue choice, town return, or
gear purchase. Run `fairlanes --skip-chaos-attract --story-autoplay` to execute
that route on production beats. The same generated
[story events](tour/generated/story-events.md) report includes the route so
an autoplay author can see exactly what is needed to advance. Autoplay is
opt-in; ordinary play waits for the player's choices.
While autoplay is enabled, achievement-style notices auto-acknowledge after
60 seconds of wall time each. Space or Enter can still acknowledge one sooner.

The previous 8-account, 5-party, 5-hero setup is available through the
`sandbox` console command. It asks for confirmation and warns that the current
story session cannot be recovered. The switch creates a fresh full-roster game.
Story state is session-only; saving it is future work.
