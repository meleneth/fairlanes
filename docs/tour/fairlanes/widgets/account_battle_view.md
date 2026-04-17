# Account Battle View

[Account Battle View](https://github.com/meleneth/fairlanes/blob/89571fb5f3e6396d5e191a557602c2b25c79a31c/src/fl/widgets/account_battle_view.cpp#L21)

Calling this a “widget” is like calling an elephant “an animal.”

## TL;DR

Combatants and combat logs.

## What it is

`AccountBattleView` is the primary UI surface for presenting battle state at the account level.

It pulls together:
- who is fighting (combatants)
- what is happening (combat log)

…and turns that into something a human can actually follow.

## What to notice

- It is a **projection of state**, not a source of truth
- It consumes ECS data and event output rather than owning gameplay logic
- It sits on top of systems like Seerin and the event buses
- Most of the game becomes visible here

## Why it matters

This is where all the underlying systems become legible.

You can have perfect:
- ECS structure
- event flow
- combat logic

…but if this layer is wrong, the game will feel wrong.

## Fairlanes take

We call it a widget because of where it lives.

Functionally, it is the **view of combat**.

## Implication for contributors

If you want to understand what combat is doing, start here.

If you change combat behavior, expect to update this.
