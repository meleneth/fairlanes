# Story events

Generated from the Ruby story DSL. `Handled` entries must match the handwritten dispatcher.

| Event ID | Handler | Dialogue emitter |
| --- | --- | --- |
| `visitor_destroyed_village` | Handled | — |
| `recruit_angry_fellow` | Handled | `angry_fellow` |
| `ask_about_visitor` | **Unimplemented** | `angry_fellow` |
| `first_combat_won` | Handled | — |
| `two_starter_wins` | Handled | — |
| `village_gear_purchased` | Handled | — |
| `recruit_second_hero` | Handled | `second_volunteer` |

## Dialogue events without handlers

- `ask_about_visitor`

## Autoplay enabled routes

### `origin_opening`

| Wait for fact | Action | Target | Choice |
| --- | --- | --- | --- |
| `visitor_destroyed_village` | `choose_dialogue` | `angry_fellow` | `invite` |
| `two_starter_wins` | `return_to_town` | — | — |
| `two_starter_wins` | `buy_gear` | `village_guard_hide` | — |
| `village_gear_purchased` | `choose_dialogue` | `second_volunteer` | `accept` |
