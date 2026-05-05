Fairlanes

One day, a Field Mouse thumped me.




tl:dr

8 accounts.
5 parties per account
5 players per party
5 skills per player

dynamic TUI animation effects?

up next: event driven w/animation timed combat simulation
see!  field mouse flash twice!
see!  targetted player flash the result! (crit/dmg/heal)


step one: get the code compiling with enough pieces it could maybe eventually work
step two: write tests for major classes to prove functionality works as expected
step thr: Observe Thump from a Field Mouse, and be Enlightened








to build:

    mkdir build
    cd build
    cmake ..

windows
    cmake --build .

linux
    cmake --build . -- -j16

f1-f8 will switch between accounts

Tracy is integrated, grab it from https://github.com/wolfpld/tracy/releases and you should be able to connect

# Blather

Playing around w/EnTT, FTX::UI, boost_ext / SML and <strike>turn based</strike> combat systems

    cmake --build . --target clang-format

    find src -type f \( -name '_.cpp' -o -name '_.hpp' -o -name '_.c' -o -name '_.h' \) -exec clang-format -i {} +

This is the design doc at the moment, none of the below is implemented.

At the start, for dev purposes, this is scoped as an autobattler that the machine itself plays - this is basically a advanced version of Progress Quest.

The Loop:
Go to (Area) kill monsters get loot
return to town, heal up
craft gear, maintain gear
on level up, progress skills (assuming skills are level-cap based)
get consumables
goto 11

# Story

We start in a small village, and life is simple enough. As we get to a certain level, the village is attacked and we must explore the larger world.

A core part of the progression is that we start in a small village, w/no magic - just normal swords / daggers / maces etc. at 25% into the game, we start seeing magic. at 50%, we start seeing modern weapons. at 75%, we start seeing cyber + lazer weapons. (modern / magic introductions are still 'up in the air' for ordering)

This progression means that old defenses are invalidated as we move 'chapters'. This implies at the least gearing resets, and possibly different sets of gear depending on what types of enemies you are expecting to encounter.

# Questions

How is area selected? is it level locked or ability to kill locked?
is creature selection biome based?
what damage types do we have?
what are caps for resistances?
what are the damage reduction formulas?

# Skills

so by default, you don't know anything
but you venture into the forest, and a field mouse uses Thump on you
and then you learn the skill Thump
and the power spiral begins

we're making 5's a thing here. so each acccount has 5 parties, each party has 5 members, and each party member has 5 skills.

Skills can be abandoned at any time. Some skills will leave behind permanent buffs even when abandoned, others require the skill to be learned (and optionally levelled!) for their effects.

We're going to need tags for the skills. Thump is a damage, physical skill. It doesn't use any weapons.

## how does experience work?

Level cap is 100
at level 1, you need to kill 10 things to get to level 2
at level 70, you would need to kill 710 things to get to level 71

## Biomes

Plains - Wolves, Horses, Meerkats
Tundra - Bison
Forest - Spiders, Squirrels, Wolves
Dark Forest - Snails
Jungle - Snakes, Gecko
Murky Swamp - Mosquitos, Dragonflies
Fetid Swamp - active rot - Mushrooms
Oceanside - Crabs, Eels, Octopus
Caves - Bats, Blind Scorpions
Desert - Scorpions, Tarantula
Mines - Dark Bats,
Ship - Narwahl, Swordfish, Kraken
Ice (berg? or just snowy outside?) - Yeti

# Drops

Loot tables are biome based
Each monster type has a specific rare drop
most monsters do not drop currency, but they do drop trash that merchants will buy
bag full = go sell

# Farming minigame

Grow reagents for consumables, seeds are purchasable or available as drops in certain biomes
plot size and time passing
mutations?

# Consumables

- Healing Potion (1 for each 10 levels)
- Mana Potion (1 for each 10 levels)
- Micro, Mini, Tiny, Little, Small, Plain, Extra, Large, Huge, Gigantic

# Entity Component Types

please generate the documentation and refer to fl::ecs::components

# Areas

## Cycle I: Origin

## Cycle II: Resonance

## Cycle III: Conflict

## Cycle IV: Singularity

## Filigree: Experience-Bound Armor Progression

Armor in *Fairlanes* does not gain power solely through upgrades or abstract currencies. It learns—and it has limits.

Each piece of armor accumulates **filigree**—Cycle-specific experience—based on when it is worn and what it endures. Fighting during a given Cycle while wearing a coat will shape that coat’s affinity for that phase of the world. Over time, the armor becomes attuned through lived use.

Only the filigree relevant to the current Cycle is active:

```cpp
effective_filigree = armor.filigree[current_cycle]
```

This creates a simple but powerful rule:

> You can only get good at what you actually do.

### Armor Tiers and Filigree Caps

Armor quality defines how much experience it can meaningfully hold.

* **Higher-tier armor**

  * greater filigree caps per Cycle
  * better base damage absorption
  * higher cost and investment

* **Lower-tier armor**

  * reaches mastery quickly
  * but caps out earlier

This creates a natural tension:

> Do you invest in better armor with long-term potential, or master what you have right now?

### Design Implications

* **Experience is temporal**
  Progress is tied to *when* actions occur, not just where.

* **Gear has memory**
  A well-worn piece reflects the Cycles it has survived.

* **Upgrades extend potential, not replace experience**
  New armor does not inherit mastery—it must earn it.

* **No stat stacking or slot puzzles**
  Filigree is additive in storage, but selectively active in context.

* **Natural progression loop**
  Enter a Cycle → endure → strengthen that Cycle’s filigree → push further as it returns.

* **Closets become lived loadouts**
  Players curate sets of gear shaped by different phases of the world.

This system keeps gear relevant indefinitely: rather than being replaced, armor becomes increasingly specialized—while higher-tier equipment expands how far that specialization can go.

# Item Slots

Chest      (core defense + filigree anchor)
Helm       (perception / status)
Boots      (movement / positioning)
Gloves     (execution / precision)
Belt       (resources / utility)

Cape       (stance / targeting / decision bias)

Necklace   (global modifier)
Ring x2    (fine-grain modifiers)

# Currencies

ᛞ Darrin-mark

# The Code

We currently have a god object, GrandCentral. Nothing else is allowed to know about GrandCentral, it's just the owner for all the things.

AppContext has log*, registry*, and rng\_.
