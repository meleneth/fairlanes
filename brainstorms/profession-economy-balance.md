# Profession economy balance experiments

This file records provisional implementation numbers and is not canonical
design authority.

- Gathering lasts 20–45 simulated seconds, yields 2 materials, and grants 6–15
  XP before triviality suppression.
- Proficiency is `floor(XP / 10)`, capped at 100.
- Healing potions require 2 Woodland Herbs and 30 seconds.
- Ordinary chest and weapon recipes consume 3 materials and take 40–45 seconds.
- Eviscerate study takes 120 seconds and grants 20 Study XP.

Material combat-drop rates, recipe discovery, prices, failure chances,
high-tier inputs, proficiency curves, offline behavior, and finished-loot
migration remain unresolved. Disk persistence is unavailable in the current
engine and is not simulated by this experiment.
