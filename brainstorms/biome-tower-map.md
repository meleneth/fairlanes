# Biome tower map

Generated from scripts/fairlanes_content_declarations.rb. Edit the Ruby DSL, then regenerate this map.
Each Cycle has five ordered 100-floor tower biomes. `Playable` means the biome has a resolved floor table; `Planned` means its tower content still needs authoring.

## Origin

### 1. `origin_woodland_tower` — Playable

| Monster ID | Current skills | Current HP | Current runtime pool |
| --- | --- | ---: | --- |
| `field_mouse` | `thump` | 5 | `common_woodland` |
| `honey_badger` | `eviscerate` | 120 | `authored_only` |
| `bumpkin_hare` | `bump` | 7 | `common_woodland` |
| `scaredy_cat` | `flee`, `thump` | 6 | `common_woodland` |
| `mire_squish` | `squish` | 9 | `common_woodland` |
| `bark_smack` | `smack` | 12 | `common_woodland` |
| `poison_toad` | `poison` | 16 | `common_woodland` |
| `squirrel_brigand` | `pebble_spit`, `flee`, `thump_around` | 20 | `common_woodland` |
| `webling_spider` | `venom_needle`, `web_snare`, `bite` | 24 | `common_woodland` |
| `moss_wolf` | `claw`, `pack_howl`, `root_leech` | 30 | `common_woodland` |
| `antler_stag` | `brace`, `bumper_rush`, `smackdown_sweep` | 36 | `common_woodland` |

### 2. `origin_plains_tower` — Playable

| Monster ID | Current skills | Current HP | Current runtime pool |
| --- | --- | ---: | --- |
| `grass_wolf` | `bite`, `pack_howl`, `claw` | 22 | `common_woodland` |
| `bristle_boar` | `bumper_rush`, `thump`, `shell_guard` | 28 | `common_woodland` |
| `prairie_meerkat` | `pebble_spit`, `flee`, `thump` | 18 | `common_woodland` |
| `dust_hare` | `bump`, `gnat_cloud`, `flee` | 22 | `common_woodland` |

### 3. `origin_murky_swamp_tower` — Playable

| Monster ID | Current skills | Current HP | Current runtime pool |
| --- | --- | ---: | --- |
| `mosquito_choir` | `venom_needle`, `gnat_cloud`, `blood_bloom` | 24 | `common_woodland` |
| `dragonfly_cutter` | `rake_line`, `venom_needle`, `flee` | 26 | `common_woodland` |
| `bog_snail` | `shell_guard`, `squish`, `miasma_cloud` | 38 | `common_woodland` |

### 4. `origin_caves_tower` — Playable

| Monster ID | Current skills | Current HP | Current runtime pool |
| --- | --- | ---: | --- |
| `cave_bat` | `bite`, `whiteout`, `flee` | 20 | `common_woodland` |
| `blind_scorpion` | `venom_needle`, `burrow`, `bump` | 30 | `common_woodland` |
| `echo_moth` | `whiteout`, `gnat_cloud`, `flee` | 24 | `common_woodland` |

### 5. `origin_desert_tower` — Playable

| Monster ID | Current skills | Current HP | Current runtime pool |
| --- | --- | ---: | --- |
| `sand_scorpion` | `venom_needle`, `burrow`, `bump` | 32 | `common_woodland` |
| `tarantula_nomad` | `bite`, `web_snare`, `venom_needle` | 34 | `common_woodland` |

## Resonance

### 1. `resonance_dark_forest_tower` — Planned

| Monster ID | Current skills | Current HP | Current runtime pool |
| --- | --- | ---: | --- |
| `briar_hexling` | `hush_hex`, `root_leech`, `blood_bloom` | 46 | `common_woodland` |
| `owlshade` | `whiteout`, `hush_hex`, `rake_line` | 44 | `common_woodland` |
| `snail_lantern` | `mercyburst`, `shell_guard`, `clearbell` | 52 | `common_woodland` |
| `choir_wisp` | `mercyburst` | 12 | `common_woodland` |

### 2. `resonance_fetid_swamp_tower` — Planned

| Monster ID | Current skills | Current HP | Current runtime pool |
| --- | --- | ---: | --- |
| `miasma_toad` | `sour_breath` | 15 | `common_woodland` |
| `gorecap_sprout` | `blood_bloom` | 16 | `common_woodland` |
| `rotcap_elder` | `rot_bloom`, `blood_bloom`, `miasma_cloud` | 62 | `common_woodland` |
| `carrion_dragonfly` | `rake_line`, `miasma_cloud`, `blood_bloom` | 54 | `common_woodland` |
| `sludge_saint` | `mercyburst`, `miasma_cloud`, `clearbell` | 68 | `common_woodland` |

### 3. `resonance_tundra_tower` — Planned

| Monster ID | Current skills | Current HP | Current runtime pool |
| --- | --- | ---: | --- |
| `yeti` | `cold_snap` | 40 | `common_woodland` |
| `rimefang_hare` | `ice_splitter` | 13 | `common_woodland` |
| `snowdrift_bison` | `bumper_rush`, `rime_armor`, `cold_snap` | 78 | `common_woodland` |
| `ice_mote` | `cold_snap`, `whiteout`, `mercyburst` | 42 | `common_woodland` |
| `berg_wyrm` | `frost_fan`, `ice_splitter`, `cold_snap` | 86 | `common_woodland` |

### 4. `resonance_volcanic_tower` — Planned

| Monster ID | Current skills | Current HP | Current runtime pool |
| --- | --- | ---: | --- |
| `salamander` | `flame_strike`, `kindle_wound`, `cinderburst` | 24 | `common_woodland` |
| `fire_drake` | `flame_wave` | 500 | `rare_woodland` |

### 5. `resonance_prismatic_tower` — Planned

| Monster ID | Current skills | Current HP | Current runtime pool |
| --- | --- | ---: | --- |
| `glass_lizard` | `laser_stitch`, `venom_needle`, `flee`, `cinder_veil` | 28 | `common_woodland` |
| `stormtick_imp` | `joltspasm` | 10 | `common_woodland` |
| `ceiling_grudge` | `rocks_fall` | 18 | `common_woodland` |
| `prism_newt` | `flame_strike`, `cold_snap`, `joltspasm`, `clearbell` | 50 | `common_woodland` |

## Conflict

### 1. `conflict_mines_tower` — Planned

| Monster ID | Current skills | Current HP | Current runtime pool |
| --- | --- | ---: | --- |
| `pickaxe_goblin` | `smackdown_sweep`, `pebble_spit`, `battle_focus` | 92 | `common_woodland` |
| `powder_rat` | `grenade_lob`, `flee`, `bite` | 70 | `common_woodland` |
| `drill_beetle` | `armor_plate`, `bumper_rush`, `smack` | 118 | `common_woodland` |

### 2. `conflict_industrial_tower` — Planned

| Monster ID | Current skills | Current HP | Current runtime pool |
| --- | --- | ---: | --- |
| `mine_canary_revenant` | `whiteout`, `sour_breath`, `clearbell` | 82 | `common_woodland` |
| `rust_loader` | `armor_plate`, `smackdown_sweep`, `suppressing_fire` | 150 | `common_woodland` |

### 3. `conflict_trenches_tower` — Planned

| Monster ID | Current skills | Current HP | Current runtime pool |
| --- | --- | ---: | --- |
| `rifle_hare` | `snap_shot`, `flee`, `battle_focus` | 86 | `common_woodland` |
| `trench_wolf` | `bite`, `pack_howl`, `suppressing_fire` | 112 | `common_woodland` |
| `grenadier_badger` | `grenade_lob`, `eviscerate`, `battle_focus` | 128 | `common_woodland` |
| `shield_bison` | `armor_plate`, `bumper_rush`, `shell_guard` | 170 | `common_woodland` |
| `smoke_crow` | `smoke_screen`, `rake_line`, `signal_flare` | 96 | `common_woodland` |

### 4. `conflict_ship_tower` — Planned

| Monster ID | Current skills | Current HP | Current runtime pool |
| --- | --- | ---: | --- |
| `swordfish_duelist` | `rake_line`, `battle_focus`, `field_dressing` | 120 | `common_woodland` |
| `narwhal_gunner` | `snap_shot`, `burst_fire`, `field_dressing` | 140 | `common_woodland` |
| `deck_kraken` | `smackdown_sweep`, `smoke_screen`, `bumper_rush`, `burst_fire` | 190 | `common_woodland` |

### 5. `conflict_oceanside_tower` — Planned

| Monster ID | Current skills | Current HP | Current runtime pool |
| --- | --- | ---: | --- |
| `bilge_eel` | `joltspasm`, `bite`, `clearbell` | 106 | `common_woodland` |

## Singularity

### 1. `singularity_cyber_jungle_tower` — Planned

| Monster ID | Current skills | Current HP | Current runtime pool |
| --- | --- | ---: | --- |
| `chrome_gecko` | `laser_stitch`, `flee`, `clock_up` | 210 | `common_woodland` |
| `packet_snake` | `venom_needle`, `signal_jam`, `packet_storm` | 230 | `common_woodland` |
| `holo_tarantula` | `web_snare`, `laser_sweep`, `packet_storm` | 250 | `common_woodland` |
| `neon_orchid` | `blood_bloom`, `checksum_ward`, `plasma_arc` | 260 | `common_woodland` |

### 2. `singularity_data_caves_tower` — Planned

| Monster ID | Current skills | Current HP | Current runtime pool |
| --- | --- | ---: | --- |
| `cache_bat` | `whiteout`, `signal_jam`, `packet_storm` | 220 | `common_woodland` |
| `kernel_grudge` | `rocks_fall`, `null_pointer`, `event_horizon` | 320 | `common_woodland` |
| `fork_bomb_imp` | `packet_storm`, `overcharge`, `blue_screen` | 240 | `common_woodland` |
| `segfault_loader` | `smackdown_sweep`, `blue_screen`, `reboot_pulse` | 360 | `common_woodland` |

### 3. `singularity_glass_desert_tower` — Planned

| Monster ID | Current skills | Current HP | Current runtime pool |
| --- | --- | ---: | --- |
| `glass_scorpion` | `laser_stitch`, `venom_needle`, `cinder_veil` | 275 | `common_woodland` |

### 4. `singularity_orbital_tundra_tower` — Planned

| Monster ID | Current skills | Current HP | Current runtime pool |
| --- | --- | ---: | --- |
| `orbital_yeti` | `ice_splitter`, `laser_sweep`, `checksum_ward` | 390 | `common_woodland` |

### 5. `singularity_null_ocean_tower` — Planned

| Monster ID | Current skills | Current HP | Current runtime pool |
| --- | --- | ---: | --- |
| `null_kraken` | `event_horizon`, `smoke_screen`, `packet_storm`, `smackdown_sweep` | 430 | `common_woodland` |
| `blue_screen_wisp` | `mercywave`, `blue_screen`, `reboot_pulse` | 260 | `common_woodland` |
| `starfire_anomaly` | `starblaze`, `flame_wave`, `gravity_sigh` | 320 | `common_woodland` |
| `null_mote` | `gravity_sigh` | 14 | `common_woodland` |

## Declared monsters without a biome

### Origin

| Monster ID | Current skills | Pool |
| --- | --- | --- |
| `visitor_herald` | `visitor_fall` | `raid` |

### Resonance

| Monster ID | Current skills | Pool |
| --- | --- | --- |
| — | None | — |

### Conflict

| Monster ID | Current skills | Pool |
| --- | --- | --- |
| — | None | — |

### Singularity

| Monster ID | Current skills | Pool |
| --- | --- | --- |
| — | None | — |

## Declared skills without a monster

- `observe`
- `forked_jolt`
- `static_field`
- `choirguard`
- `cave_in`
- `weight_of_tuesday`

Monster skills and HP above are the current Ruby declarations, not future balance promises.
