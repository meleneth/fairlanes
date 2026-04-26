# Combatant

[Combatant](https://github.com/meleneth/fairlanes/blob/89571fb5f3e6396d5e191a557602c2b25c79a31c/src/fl/widgets/combatant.cpp#L58)

## TL;DR

One unit in combat, as seen by the player.

## Fun fact

This entire game grew out of messing with this widget.

## What it is

`Combatant` is the UI representation of a single participant in combat.

It is responsible for showing things like:
- identity
- stats
- current state (alive, acting, etc.)

## What to notice

- It is a **view over ECS state**, not a gameplay authority
- It pulls together multiple components into something human-readable
- It sits downstream of systems like Seerin and the event flow

## Why it matters

This is one of the smallest complete “slices” of the game:

- ECS → provides the data  
- systems → update that data  
- events → signal changes  
- this widget → makes it visible  

That makes it a great place to understand how everything fits together.

## Fairlanes take

Sometimes the best way to design a system is to start with what you want to see.

`Combatant` is that starting point.

## Implication for contributors

If you want to understand how gameplay becomes UI, start here.

If you change what a combatant *is*, this is one of the places that will need to reflect it.
