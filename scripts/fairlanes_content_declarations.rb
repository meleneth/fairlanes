# frozen_string_literal: true

# Declarative content source for generated C++ metadata, parity tests, and
# review docs. Handwritten C++ remains the authority for behavior.

visual :flame_wave
visual :shock
visual :rocks_fall
visual :poison_cloud
visual :holy_nova
visual :blood_bloom
visual :frost_crack
visual :void_ripple
visual :starfire

status :poison,
       display: "Poison",
       component: "Poison",
       palette_index: 20

status :dire_bleed,
       display: "Dire Bleed",
       debug_name: "dire-bleed",
       component: "DireBleed",
       palette_index: 6

status :freeze,
       display: "Frozen",
       component: "Freeze",
       palette_index: 27

skill :observe,
      learn_chance_percent: 0,
      random_combat: false,
      execution: :observe,
      tags: %i[observe utility],
      declarative_shape: :handwritten_behavior

skill :flee,
      learn_chance_percent: 0,
      flee_success_percent: 65,
      execution: :flee,
      tags: %i[utility escape],
      declarative_shape: :handwritten_behavior

skill :thump,
      learn_chance_percent: 20,
      execution: :thump_like,
      tags: %i[physical blunt melee enemy damage],
      declarative_shape: :handwritten_behavior

skill :eviscerate,
      learn_chance_percent: 2,
      execution: :eviscerate,
      tags: %i[physical slashing bleed melee enemy damage],
      declarative_shape: :handwritten_behavior

skill :poison,
      learn_chance_percent: 5,
      execution: :poison,
      tags: %i[poison enemy debuff poison_status natural],
      declarative_shape: :handwritten_behavior

skill :cold_snap,
      learn_chance_percent: 5,
      execution: :cold_snap,
      tags: %i[cold spell enemy damage slow_status],
      declarative_shape: :handwritten_behavior

skill :flame_strike,
      learn_chance_percent: 5,
      execution: :flame_strike,
      visual: :flame_wave,
      tags: %i[fire spell enemy damage],
      declarative_shape: :handwritten_behavior

skill :flame_wave,
      learn_chance_percent: 2,
      execution: :flame_wave,
      visual: :flame_wave,
      tags: %i[fire area spell group damage],
      declarative_shape: :handwritten_behavior

decal_skill :joltspasm,
            visual: :shock,
            tags: %i[lightning control enemy damage]

decal_skill :rocks_fall,
            visual: :rocks_fall,
            tags: %i[physical earth blunt area group damage]

decal_skill :sour_breath,
            visual: :poison_cloud,
            tags: %i[acid disease area group damage]

decal_skill :mercyburst,
            visual: :holy_nova,
            tags: %i[holy spell ally healing heal]

decal_skill :blood_bloom,
            visual: :blood_bloom,
            tags: %i[bleed healing area group damage]

decal_skill :ice_splitter,
            visual: :frost_crack,
            tags: %i[cold piercing projectile enemy damage]

decal_skill :gravity_sigh,
            visual: :void_ripple,
            tags: %i[gravity control area group damage]

skill :bump,
      learn_chance_percent: 20,
      execution: :thump_like,
      tags: %i[physical blunt melee enemy damage],
      declarative_shape: :handwritten_behavior

skill :squish,
      learn_chance_percent: 20,
      execution: :thump_like,
      tags: %i[physical blunt control melee enemy damage],
      declarative_shape: :handwritten_behavior

skill :smack,
      learn_chance_percent: 20,
      execution: :thump_like,
      tags: %i[physical blunt melee enemy damage],
      declarative_shape: :handwritten_behavior

[
  [:thump_around, :group_damage, %i[physical blunt melee group damage natural]],
  [:rake_line, :group_damage, %i[physical slashing melee group damage bleed_status natural]],
  [:smackdown_sweep, :group_damage, %i[physical blunt melee group damage natural]],
  [:bumper_rush, :group_damage, %i[physical blunt melee group damage natural]],
  [:kindle_wound, :damage_strike, %i[fire spell enemy damage burn_status]],
  [:cinder_veil, :placeholder_effect, %i[fire spell ally buff shield_status]],
  [:frost_fan, :group_damage, %i[cold spell group damage]],
  [:rime_armor, :placeholder_effect, %i[cold spell ally buff shield_status]],
  [:whiteout, :placeholder_effect, %i[cold air group debuff blind_status]],
  [:forked_jolt, :damage_strike, %i[lightning spell random_enemy damage]],
  [:static_field, :group_damage, %i[lightning field group damage slow_status]],
  [:overcharge, :placeholder_effect, %i[lightning self buff]],
  [:mercywave, :group_heal, %i[holy spell all_allies healing heal]],
  [:clearbell, :placeholder_effect, %i[holy spell ally cleanse]],
  [:hush_hex, :placeholder_effect, %i[holy spell enemy debuff silence_status]],
  [:choirguard, :placeholder_effect, %i[holy song all_allies buff shield_status]],
  [:miasma_cloud, :placeholder_effect, %i[poison rot field group poison_status debuff]],
  [:rot_bloom, :group_damage, %i[rot plant group damage debuff]],
  [:root_leech, :damage_strike, %i[plant rot enemy damage drain slow_status]],
  [:pebble_spit, :damage_strike, %i[earth ranged enemy damage]],
  [:cave_in, :group_damage, %i[earth trap group damage slow_status]],
  [:weight_of_tuesday, :placeholder_effect, %i[gravity void spell group debuff slow_status]],
  [:snap_shot, :damage_strike, %i[ballistic ranged enemy damage industrial]],
  [:burst_fire, :group_damage, %i[ballistic ranged group damage industrial]],
  [:suppressing_fire, :group_damage, %i[ballistic ranged group damage debuff slow_status industrial]],
  [:smoke_screen, :placeholder_effect, %i[industrial field group debuff blind_status]],
  [:field_dressing, :single_heal, %i[martial ally healing heal]],
  [:laser_stitch, :damage_strike, %i[laser ranged enemy damage cybernetic]],
  [:laser_sweep, :group_damage, %i[laser ranged group damage cybernetic]],
  [:packet_storm, :damage_strike, %i[data ranged random_enemy damage cybernetic]],
  [:signal_jam, :placeholder_effect, %i[data enemy debuff silence_status]],
  [:reboot_pulse, :group_heal, %i[data all_allies cleanse healing heal]],
  [:clock_up, :placeholder_effect, %i[data ally buff haste_status]],
  [:blue_screen, :placeholder_effect, %i[data group debuff stun_status silence_status]],
  [:null_pointer, :damage_strike, %i[void data enemy damage execute]],
  [:event_horizon, :group_damage, %i[gravity void all_enemies damage slow_status]],
  [:bite, :damage_strike, %i[physical piercing melee enemy damage beast]],
  [:pack_howl, :placeholder_effect, %i[song group buff beast natural]],
  [:claw, :damage_strike, %i[physical slashing melee enemy damage beast]],
  [:shell_guard, :placeholder_effect, %i[physical self buff shield_status natural]],
  [:venom_needle, :damage_strike, %i[poison piercing ranged enemy damage poison_status natural]],
  [:web_snare, :placeholder_effect, %i[trap enemy debuff slow_status vermin natural]],
  [:gnat_cloud, :placeholder_effect, %i[poison air group debuff blind_status vermin]],
  [:burrow, :placeholder_effect, %i[earth self buff natural]],
  [:battle_focus, :placeholder_effect, %i[martial self buff]],
  [:grenade_lob, :group_damage, %i[ballistic trap group damage industrial]],
  [:armor_plate, :placeholder_effect, %i[industrial self buff shield_status machine]],
  [:signal_flare, :placeholder_effect, %i[fire holy field group debuff industrial]],
  [:checksum_ward, :placeholder_effect, %i[data ally buff shield_status cybernetic]],
  [:plasma_arc, :damage_strike, %i[plasma lightning ranged enemy damage cybernetic]],
].each do |id, execution, tags|
  skill id,
        learn_chance_percent: 5,
        execution: execution,
        tags: tags,
        declarative_shape: execution == :placeholder_effect ? :placeholder_effect : :generated_runtime_behavior
end

skill :starblaze,
      learn_chance_percent: 1,
      execution: :decal_strike,
      visual: :starfire,
      tags: %i[fire arcane spell enemy damage],
      declarative_shape: :decal_strike

random_combat_skills(
  :flee,
  :thump,
  :eviscerate,
  :poison,
  :cold_snap,
  :flame_strike,
  :flame_wave,
  :bump,
  :squish,
  :smack,
  :joltspasm,
  :rocks_fall,
  :sour_breath,
  :mercyburst,
  :blood_bloom,
  :ice_splitter,
  :gravity_sigh,
  :thump_around,
  :rake_line,
  :smackdown_sweep,
  :bumper_rush,
  :kindle_wound,
  :cinder_veil,
  :frost_fan,
  :rime_armor,
  :whiteout,
  :forked_jolt,
  :static_field,
  :overcharge,
  :mercywave,
  :clearbell,
  :hush_hex,
  :choirguard,
  :miasma_cloud,
  :rot_bloom,
  :root_leech,
  :pebble_spit,
  :cave_in,
  :weight_of_tuesday,
  :snap_shot,
  :burst_fire,
  :suppressing_fire,
  :smoke_screen,
  :field_dressing,
  :laser_stitch,
  :laser_sweep,
  :packet_storm,
  :signal_jam,
  :reboot_pulse,
  :clock_up,
  :blue_screen,
  :null_pointer,
  :event_horizon,
  :bite,
  :pack_howl,
  :claw,
  :shell_guard,
  :venom_needle,
  :web_snare,
  :gnat_cloud,
  :burrow,
  :battle_focus,
  :grenade_lob,
  :armor_plate,
  :signal_flare,
  :checksum_ward,
  :plasma_arc,
  :starblaze
)

monster :field_mouse,
        hp: 5,
        known_skills: %i[thump],
        pool: :common_woodland

monster :honey_badger,
        hp: 500,
        known_skills: %i[eviscerate],
        pool: :rare_woodland

monster :bumpkin_hare,
        hp: 7,
        level: 2,
        known_skills: %i[bump],
        pool: :common_woodland

monster :scaredy_cat,
        hp: 6,
        level: 2,
        known_skills: %i[flee thump],
        pool: :common_woodland

monster :mire_squish,
        hp: 9,
        level: 3,
        known_skills: %i[squish],
        pool: :common_woodland

monster :bark_smack,
        hp: 12,
        level: 4,
        known_skills: %i[smack],
        pool: :common_woodland

monster :poison_toad,
        hp: 16,
        level: 5,
        known_skills: %i[poison],
        pool: :common_woodland

monster :yeti,
        hp: 40,
        known_skills: %i[cold_snap],
        pool: :common_woodland

monster :salamander,
        hp: 24,
        level: 6,
        known_skills: %i[flame_strike],
        pool: :common_woodland

monster :fire_drake,
        hp: 500,
        level: 8,
        known_skills: %i[flame_wave],
        pool: :rare_woodland

monster :stormtick_imp,
        hp: 10,
        level: 3,
        known_skills: %i[joltspasm],
        pool: :common_woodland

monster :ceiling_grudge,
        hp: 18,
        level: 5,
        known_skills: %i[rocks_fall],
        pool: :common_woodland

monster :miasma_toad,
        hp: 15,
        level: 4,
        known_skills: %i[sour_breath],
        pool: :common_woodland

monster :choir_wisp,
        hp: 12,
        level: 4,
        known_skills: %i[mercyburst],
        pool: :common_woodland

monster :gorecap_sprout,
        hp: 16,
        level: 5,
        known_skills: %i[blood_bloom],
        pool: :common_woodland

monster :rimefang_hare,
        hp: 13,
        level: 4,
        known_skills: %i[ice_splitter],
        pool: :common_woodland

monster :null_mote,
        hp: 14,
        level: 5,
        known_skills: %i[gravity_sigh],
        pool: :common_woodland

[
  [:grass_wolf, "Grass Wolf", 22, 0, 3, %i[bite pack_howl claw]],
  [:bristle_boar, "Bristle Boar", 28, 0, 4, %i[bumper_rush thump shell_guard]],
  [:prairie_meerkat, "Prairie Meerkat", 18, 0, 3, %i[pebble_spit flee thump]],
  [:squirrel_brigand, "Squirrel Brigand", 20, 0, 4, %i[pebble_spit flee thump_around]],
  [:webling_spider, "Webling Spider", 24, 0, 4, %i[venom_needle web_snare bite]],
  [:moss_wolf, "Moss Wolf", 30, 0, 5, %i[claw pack_howl root_leech]],
  [:antler_stag, "Antler Stag", 36, 0, 6, %i[bumper_rush smackdown_sweep shell_guard]],
  [:mosquito_choir, "Mosquito Choir", 24, 0, 5, %i[venom_needle gnat_cloud blood_bloom]],
  [:dragonfly_cutter, "Dragonfly Cutter", 26, 0, 5, %i[rake_line venom_needle flee]],
  [:bog_snail, "Bog Snail", 38, 0, 6, %i[shell_guard squish miasma_cloud]],
  [:cave_bat, "Cave Bat", 20, 0, 3, %i[bite whiteout flee]],
  [:blind_scorpion, "Blind Scorpion", 30, 0, 5, %i[venom_needle burrow bump]],
  [:echo_moth, "Echo Moth", 24, 0, 5, %i[whiteout gnat_cloud flee]],
  [:sand_scorpion, "Sand Scorpion", 32, 0, 5, %i[venom_needle burrow bump]],
  [:tarantula_nomad, "Tarantula Nomad", 34, 0, 6, %i[bite web_snare venom_needle]],
  [:dust_hare, "Dust Hare", 22, 0, 4, %i[bump gnat_cloud flee]],
  [:glass_lizard, "Glass Lizard", 28, 0, 7, %i[laser_stitch venom_needle flee cinder_veil]],
  [:snail_lantern, "Snail Lantern", 52, 18, 8, %i[mercyburst shell_guard clearbell]],
  [:briar_hexling, "Briar Hexling", 46, 28, 9, %i[hush_hex root_leech blood_bloom]],
  [:owlshade, "Owlshade", 44, 24, 9, %i[whiteout hush_hex rake_line]],
  [:rotcap_elder, "Rotcap Elder", 62, 35, 11, %i[rot_bloom blood_bloom miasma_cloud]],
  [:carrion_dragonfly, "Carrion Dragonfly", 54, 22, 10, %i[rake_line miasma_cloud blood_bloom]],
  [:sludge_saint, "Sludge Saint", 68, 38, 12, %i[mercyburst miasma_cloud clearbell]],
  [:snowdrift_bison, "Snowdrift Bison", 78, 20, 11, %i[bumper_rush rime_armor cold_snap]],
  [:ice_mote, "Ice Mote", 42, 32, 8, %i[cold_snap whiteout mercyburst]],
  [:berg_wyrm, "Berg Wyrm", 86, 35, 13, %i[frost_fan ice_splitter cold_snap]],
  [:prism_newt, "Prism Newt", 50, 40, 10, %i[flame_strike cold_snap joltspasm clearbell]],
  [:pickaxe_goblin, "Pickaxe Goblin", 92, 0, 14, %i[smackdown_sweep pebble_spit battle_focus]],
  [:powder_rat, "Powder Rat", 70, 0, 14, %i[grenade_lob flee bite]],
  [:drill_beetle, "Drill Beetle", 118, 0, 16, %i[armor_plate bumper_rush smack]],
  [:mine_canary_revenant, "Mine Canary Revenant", 82, 15, 17, %i[whiteout sour_breath clearbell]],
  [:rust_loader, "Rust Loader", 150, 0, 19, %i[armor_plate smackdown_sweep suppressing_fire]],
  [:rifle_hare, "Rifle Hare", 86, 0, 14, %i[snap_shot flee battle_focus]],
  [:trench_wolf, "Trench Wolf", 112, 0, 16, %i[bite pack_howl suppressing_fire]],
  [:grenadier_badger, "Grenadier Badger", 128, 0, 18, %i[grenade_lob eviscerate battle_focus]],
  [:shield_bison, "Shield Bison", 170, 0, 20, %i[armor_plate bumper_rush shell_guard]],
  [:smoke_crow, "Smoke Crow", 96, 0, 17, %i[smoke_screen rake_line signal_flare]],
  [:swordfish_duelist, "Swordfish Duelist", 120, 0, 18, %i[rake_line battle_focus field_dressing]],
  [:narwhal_gunner, "Narwhal Gunner", 140, 0, 19, %i[snap_shot burst_fire field_dressing]],
  [:deck_kraken, "Deck Kraken", 190, 0, 21, %i[smackdown_sweep smoke_screen bumper_rush burst_fire]],
  [:bilge_eel, "Bilge Eel", 106, 20, 16, %i[joltspasm bite clearbell]],
  [:chrome_gecko, "Chrome Gecko", 210, 55, 24, %i[laser_stitch flee clock_up]],
  [:packet_snake, "Packet Snake", 230, 70, 25, %i[venom_needle signal_jam packet_storm]],
  [:holo_tarantula, "Holo-Tarantula", 250, 80, 26, %i[web_snare laser_sweep packet_storm]],
  [:neon_orchid, "Neon Orchid", 260, 95, 27, %i[blood_bloom checksum_ward plasma_arc]],
  [:cache_bat, "Cache Bat", 220, 75, 24, %i[whiteout signal_jam packet_storm]],
  [:kernel_grudge, "Kernel Grudge", 320, 100, 29, %i[rocks_fall null_pointer event_horizon]],
  [:fork_bomb_imp, "Fork Bomb Imp", 240, 110, 28, %i[packet_storm overcharge blue_screen]],
  [:segfault_loader, "Segfault Loader", 360, 90, 30, %i[smackdown_sweep blue_screen reboot_pulse]],
  [:glass_scorpion, "Glass Scorpion", 275, 65, 26, %i[laser_stitch venom_needle cinder_veil]],
  [:orbital_yeti, "Orbital Yeti", 390, 100, 31, %i[ice_splitter laser_sweep checksum_ward]],
  [:null_kraken, "Null Kraken", 430, 120, 32, %i[event_horizon smoke_screen packet_storm smackdown_sweep]],
  [:blue_screen_wisp, "Blue Screen Wisp", 260, 130, 28, %i[mercywave blue_screen reboot_pulse]],
  [:starfire_anomaly, "Starfire Anomaly", 320, 120, 24, %i[starblaze flame_wave gravity_sigh]],
].each do |id, display, hp, mp, level, known_skills|
  monster id,
          display: display,
          hp: hp,
          mp: mp,
          level: level,
          known_skills: known_skills,
          pool: :common_woodland
end
