# Animation Decals

Back to [Tour Home](../../index.md)

Animation decals are transient combat visuals drawn over a `Combatant` widget. They show skill effects, status visuals, and hitpoint numbers without becoming gameplay authority.

## Where They Live

Core files:

- `src/fl/ecs/components/visual_effects.hpp`: `DecalEffect` and `CombatantDecals` ECS components.
- `src/fl/widgets/effects/decal.hpp`: decal animation interface, kinds, and config.
- `src/fl/widgets/effects/decal_factory.cpp`: maps `DecalAnimationKind` to concrete animation classes.
- `src/fl/widgets/combatant.cpp`: `AttackDecalNode`, the FTXUI overlay node that renders decals over a combatant.
- `src/fl/ecs/systems/visual_resolver.cpp`: removes finished decal effects.

Common producers:

- `src/fl/skills/skill_sequence.cpp`: skill visuals and healing numbers.
- `src/fl/ecs/systems/take_damage.cpp`: hitpoint-number decals for damage.

## The Timing Contract

Every decal animation renders from normalized progress:

```cpp
Frame render(float progress) const;
```

`progress` is always conceptually in the range `0.0` to `1.0`:

- `0.0` means the animation has just started.
- `1.0` means the animation is complete.
- Intermediate values are the only timing input the animation should need.

`DecalEffect::progress_at(now)` maps wall-clock elapsed time into that normalized range using the effect's `duration`, then clamps the result to `[0.0, 1.0]`. Helpers such as `clamp_progress(...)` also clamp and treat NaN as `0.0`.

This is important: concrete decal classes should not invent their own external clocks. They receive normalized progress and render the correct frame for that progress.

## Size Awareness

Decals are auto-size aware. They are constructed from the actual box assigned to the combatant during FTXUI layout:

```text
Combatant Render()
  -> AttackDecalNode::ComputeRequirement()
  -> preserve child requirement
  -> AttackDecalNode::SetBox(box)
  -> create animations with width and decal height
```

`AttackDecalNode::ComputeRequirement()` copies the child combatant's requirement. That means adding a decal must not change the fitted widget size.

`AttackDecalNode::SetBox(...)` then reads the real assigned width and height, builds each animation with that size, and caches the prepared animations until the size/effect count changes.

Some decals use `extra_height` as overscan. This lets an effect rise above the combatant box, but it still does not change the combatant's layout requirement. Rendering anchors the decal frame to the bottom of the combatant box and clips safely against the screen bounds.

`FlameWave` is the important example: it is intentionally taller than the targeted `Combatant`, so the flame can lick upward past the unit instead of feeling trapped inside the card. That extra height is visual only. The target combatant still reports the same fitted width and height with or without the flame decal.

## Rendering Model

A decal animation returns a `Frame`:

- `width`
- `height`
- a grid of `RenderCell` values

Each active cell may provide a glyph, foreground color, background color, and alpha. `AttackDecalNode::Render(...)` first renders the underlying combatant, then overlays active decal cells onto the screen.

Cells are bounds-checked before drawing. A background-only decal cell can tint existing text; a glyph cell can replace the visible character. Multiple active decals can stack on one combatant.

## Lifecycle

A decal is attached by adding a `CombatantDecals` component containing one or more `DecalEffect` values.

`VisualResolver` removes an individual decal when either condition is true:

- its beat-based `expires_at` has passed
- its normalized progress reaches `1.0`

When the last effect is removed, `VisualResolver` removes the `CombatantDecals` component.

## Current Animation Kinds

`DecalAnimationKind` currently includes:

- `FlameWave`
- `Shock`
- `RocksFall`
- `PoisonCloud`
- `HolyNova`
- `BloodBloom`
- `FrostCrack`
- `VoidRipple`
- `HitpointNumber`

Each concrete animation should follow the same two rules: render from normalized progress, and respect the width/height it was constructed with.

## Tests To Protect

`tests/widgets/test_combatant.cpp` has direct coverage for the layout invariant:

- flame decals do not change fitted combatant dimensions
- non-flame decals do not change fitted combatant dimensions
- multiple decals do not change fitted combatant dimensions

Those tests are part of the contract. If an animation changes layout size, it is probably wrong.
