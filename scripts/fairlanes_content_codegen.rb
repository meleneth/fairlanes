#!/usr/bin/env ruby
# frozen_string_literal: true

require "fileutils"
require "optparse"
require "set"

Skill = Struct.new(
  :id, :cpp_id, :display, :learn_chance_percent, :random_combat,
  :execution, :visual, :tags, :declarative_shape,
  keyword_init: true
)

Monster = Struct.new(
  :id, :cpp_id, :display, :known_skills, :pool,
  keyword_init: true
)

VISUAL_CPP = {
  flame_wave: "FlameWave",
  shock: "Shock",
  rocks_fall: "RocksFall",
  poison_cloud: "PoisonCloud",
  holy_nova: "HolyNova",
  blood_bloom: "BloodBloom",
  frost_crack: "FrostCrack",
  void_ripple: "VoidRipple"
}.freeze

VALID_POOLS = Set[:common_woodland, :rare_woodland].freeze

SKILLS = [
  Skill.new(
    id: :observe,
    cpp_id: "Observe",
    display: "Observe",
    learn_chance_percent: 0,
    random_combat: false,
    execution: :observe,
    visual: nil,
    tags: %i[observe utility],
    declarative_shape: :handwritten_behavior
  ),
  Skill.new(
    id: :flee,
    cpp_id: "Flee",
    display: "Flee",
    learn_chance_percent: 0,
    random_combat: true,
    execution: :flee,
    visual: nil,
    tags: %i[utility escape],
    declarative_shape: :handwritten_behavior
  ),
  Skill.new(
    id: :thump,
    cpp_id: "Thump",
    display: "Thump",
    learn_chance_percent: 20,
    random_combat: true,
    execution: :thump_like,
    visual: nil,
    tags: %i[physical blunt melee],
    declarative_shape: :handwritten_behavior
  ),
  Skill.new(
    id: :eviscerate,
    cpp_id: "Eviscerate",
    display: "Eviscerate",
    learn_chance_percent: 2,
    random_combat: true,
    execution: :eviscerate,
    visual: nil,
    tags: %i[physical slashing bleed melee],
    declarative_shape: :handwritten_behavior
  ),
  Skill.new(
    id: :poison,
    cpp_id: "Poison",
    display: "Poison",
    learn_chance_percent: 5,
    random_combat: true,
    execution: :poison,
    visual: nil,
    tags: %i[poison disease],
    declarative_shape: :handwritten_behavior
  ),
  Skill.new(
    id: :cold_snap,
    cpp_id: "ColdSnap",
    display: "Cold Snap",
    learn_chance_percent: 5,
    random_combat: true,
    execution: :cold_snap,
    visual: nil,
    tags: %i[cold control],
    declarative_shape: :handwritten_behavior
  ),
  Skill.new(
    id: :flame_strike,
    cpp_id: "FlameStrike",
    display: "Flame Strike",
    learn_chance_percent: 5,
    random_combat: true,
    execution: :flame_strike,
    visual: :flame_wave,
    tags: %i[fire projectile],
    declarative_shape: :handwritten_behavior
  ),
  Skill.new(
    id: :flame_wave,
    cpp_id: "FlameWave",
    display: "Flame Wave",
    learn_chance_percent: 2,
    random_combat: true,
    execution: :flame_wave,
    visual: :flame_wave,
    tags: %i[fire area],
    declarative_shape: :handwritten_behavior
  ),
  Skill.new(
    id: :joltspasm,
    cpp_id: "Joltspasm",
    display: "Joltspasm",
    learn_chance_percent: 5,
    random_combat: true,
    execution: :decal_strike,
    visual: :shock,
    tags: %i[lightning projectile],
    declarative_shape: :decal_strike
  ),
  Skill.new(
    id: :rocks_fall,
    cpp_id: "RocksFall",
    display: "Rocks Fall",
    learn_chance_percent: 5,
    random_combat: true,
    execution: :decal_strike,
    visual: :rocks_fall,
    tags: %i[earth projectile],
    declarative_shape: :decal_strike
  ),
  Skill.new(
    id: :sour_breath,
    cpp_id: "SourBreath",
    display: "Sour Breath",
    learn_chance_percent: 5,
    random_combat: true,
    execution: :decal_strike,
    visual: :poison_cloud,
    tags: %i[poison acid projectile],
    declarative_shape: :decal_strike
  ),
  Skill.new(
    id: :mercyburst,
    cpp_id: "Mercyburst",
    display: "Mercyburst",
    learn_chance_percent: 5,
    random_combat: true,
    execution: :decal_strike,
    visual: :holy_nova,
    tags: %i[healing holy area],
    declarative_shape: :decal_strike
  ),
  Skill.new(
    id: :blood_bloom,
    cpp_id: "BloodBloom",
    display: "Blood Bloom",
    learn_chance_percent: 5,
    random_combat: true,
    execution: :decal_strike,
    visual: :blood_bloom,
    tags: %i[bleed projectile],
    declarative_shape: :decal_strike
  ),
  Skill.new(
    id: :ice_splitter,
    cpp_id: "IceSplitter",
    display: "Ice Splitter",
    learn_chance_percent: 5,
    random_combat: true,
    execution: :decal_strike,
    visual: :frost_crack,
    tags: %i[cold projectile],
    declarative_shape: :decal_strike
  ),
  Skill.new(
    id: :gravity_sigh,
    cpp_id: "GravitySigh",
    display: "Gravity Sigh",
    learn_chance_percent: 5,
    random_combat: true,
    execution: :decal_strike,
    visual: :void_ripple,
    tags: %i[gravity control],
    declarative_shape: :decal_strike
  ),
  Skill.new(
    id: :bump,
    cpp_id: "Bump",
    display: "Bump",
    learn_chance_percent: 20,
    random_combat: true,
    execution: :thump_like,
    visual: nil,
    tags: %i[physical blunt melee],
    declarative_shape: :handwritten_behavior
  ),
  Skill.new(
    id: :squish,
    cpp_id: "Squish",
    display: "Squish",
    learn_chance_percent: 20,
    random_combat: true,
    execution: :thump_like,
    visual: nil,
    tags: %i[physical blunt melee],
    declarative_shape: :handwritten_behavior
  ),
  Skill.new(
    id: :smack,
    cpp_id: "Smack",
    display: "Smack",
    learn_chance_percent: 20,
    random_combat: true,
    execution: :thump_like,
    visual: nil,
    tags: %i[physical blunt melee],
    declarative_shape: :handwritten_behavior
  )
].freeze

RANDOM_COMBAT_SKILLS = %i[
  thump
  eviscerate
  poison
  cold_snap
  flame_strike
  flame_wave
  bump
  squish
  smack
  joltspasm
  rocks_fall
  sour_breath
  mercyburst
  blood_bloom
  ice_splitter
  gravity_sigh
  flee
].freeze

MONSTERS = [
  Monster.new(
    id: :field_mouse,
    cpp_id: "FieldMouse",
    display: "Field Mouse",
    known_skills: %i[thump],
    pool: :common_woodland
  ),
  Monster.new(
    id: :honey_badger,
    cpp_id: "HoneyBadger",
    display: "Honey Badger",
    known_skills: %i[eviscerate],
    pool: :rare_woodland
  ),
  Monster.new(
    id: :bumpkin_hare,
    cpp_id: "BumpkinHare",
    display: "Bumpkin Hare",
    known_skills: %i[bump],
    pool: :common_woodland
  ),
  Monster.new(
    id: :scaredy_cat,
    cpp_id: "ScaredyCat",
    display: "Scaredy Cat",
    known_skills: %i[flee thump],
    pool: :common_woodland
  ),
  Monster.new(
    id: :mire_squish,
    cpp_id: "MireSquish",
    display: "Mire Squish",
    known_skills: %i[squish],
    pool: :common_woodland
  ),
  Monster.new(
    id: :bark_smack,
    cpp_id: "BarkSmack",
    display: "Bark Smack",
    known_skills: %i[smack],
    pool: :common_woodland
  ),
  Monster.new(
    id: :poison_toad,
    cpp_id: "PoisonToad",
    display: "Poison Toad",
    known_skills: %i[poison],
    pool: :common_woodland
  ),
  Monster.new(
    id: :yeti,
    cpp_id: "Yeti",
    display: "Yeti",
    known_skills: %i[cold_snap],
    pool: :common_woodland
  ),
  Monster.new(
    id: :salamander,
    cpp_id: "Salamander",
    display: "Salamander",
    known_skills: %i[flame_strike],
    pool: :common_woodland
  ),
  Monster.new(
    id: :fire_drake,
    cpp_id: "FireDrake",
    display: "Fire Drake",
    known_skills: %i[flame_wave],
    pool: :rare_woodland
  ),
  Monster.new(
    id: :stormtick_imp,
    cpp_id: "StormtickImp",
    display: "Stormtick Imp",
    known_skills: %i[joltspasm],
    pool: :common_woodland
  ),
  Monster.new(
    id: :ceiling_grudge,
    cpp_id: "CeilingGrudge",
    display: "Ceiling Grudge",
    known_skills: %i[rocks_fall],
    pool: :common_woodland
  ),
  Monster.new(
    id: :miasma_toad,
    cpp_id: "MiasmaToad",
    display: "Miasma Toad",
    known_skills: %i[sour_breath],
    pool: :common_woodland
  ),
  Monster.new(
    id: :choir_wisp,
    cpp_id: "ChoirWisp",
    display: "Choir Wisp",
    known_skills: %i[mercyburst],
    pool: :common_woodland
  ),
  Monster.new(
    id: :gorecap_sprout,
    cpp_id: "GorecapSprout",
    display: "Gorecap Sprout",
    known_skills: %i[blood_bloom],
    pool: :common_woodland
  ),
  Monster.new(
    id: :rimefang_hare,
    cpp_id: "RimefangHare",
    display: "Rimefang Hare",
    known_skills: %i[ice_splitter],
    pool: :common_woodland
  ),
  Monster.new(
    id: :null_mote,
    cpp_id: "NullMote",
    display: "Null Mote",
    known_skills: %i[gravity_sigh],
    pool: :common_woodland
  )
].freeze

def validate!
  errors = []

  skill_ids = SKILLS.map(&:id)
  monster_ids = MONSTERS.map(&:id)
  errors << "duplicate skill ids" unless skill_ids.uniq.size == skill_ids.size
  errors << "duplicate monster ids" unless monster_ids.uniq.size == monster_ids.size

  skills_by_id = SKILLS.to_h { |skill| [skill.id, skill] }
  random_skill_ids = SKILLS.select(&:random_combat).map(&:id)

  SKILLS.each do |skill|
    errors << "skill #{skill.id} is missing a C++ id" if skill.cpp_id.to_s.empty?
    errors << "skill #{skill.id} is missing a display name" if skill.display.to_s.empty?
    if skill.learn_chance_percent.negative? || skill.learn_chance_percent > 100
      errors << "skill #{skill.id} has invalid learn chance #{skill.learn_chance_percent}"
    end
    errors << "skill #{skill.id} has invalid visual #{skill.visual}" if skill.visual && !VISUAL_CPP.key?(skill.visual)
    errors << "skill #{skill.id} has no tags" if skill.tags.empty?
  end

  RANDOM_COMBAT_SKILLS.each do |skill_id|
    errors << "random combat skill #{skill_id} is unknown" unless skills_by_id.key?(skill_id)
    errors << "random combat skill #{skill_id} is not marked random_combat" unless random_skill_ids.include?(skill_id)
  end

  random_skill_ids.each do |skill_id|
    errors << "random combat skill #{skill_id} is missing from order table" unless RANDOM_COMBAT_SKILLS.include?(skill_id)
  end

  MONSTERS.each do |monster|
    errors << "monster #{monster.id} is missing a C++ id" if monster.cpp_id.to_s.empty?
    errors << "monster #{monster.id} is missing a display name" if monster.display.to_s.empty?
    errors << "monster #{monster.id} has invalid pool #{monster.pool}" unless VALID_POOLS.include?(monster.pool)
    errors << "monster #{monster.id} has no known skills" if monster.known_skills.empty?
    monster.known_skills.each do |skill_id|
      errors << "monster #{monster.id} references unknown skill #{skill_id}" unless skills_by_id.key?(skill_id)
    end
  end

  return if errors.empty?

  warn "Fairlanes content declaration validation failed:"
  errors.each { |error| warn "  - #{error}" }
  exit 1
end

def cpp_string(value)
  value.inspect
end

def skill_id_cpp(skill)
  "fl::skills::SkillId::#{skill.cpp_id}"
end

def skill_key_cpp(skill)
  "fl::skills::SkillKey{#{skill_id_cpp(skill)}}"
end

def skill_symbol_cpp(skill_id)
  skill = SKILLS.find { |candidate| candidate.id == skill_id }
  raise "unknown skill #{skill_id}" unless skill

  skill_id_cpp(skill)
end

def monster_cpp(monster)
  "fl::monster::MonsterKind::#{monster.cpp_id}"
end

def visual_cpp(skill)
  "fl::widgets::effects::DecalAnimationKind::#{VISUAL_CPP.fetch(skill.visual)}"
end

def pool_cpp(pool)
  pool == :rare_woodland ? "RareWoodland" : "CommonWoodland"
end

def generate_test_cpp
  skill_rows = SKILLS.map do |skill|
    visual = skill.visual ? visual_cpp(skill) : "std::nullopt"
    <<~CPP
          ExpectedSkill{
              #{skill_key_cpp(skill)},
              #{cpp_string(skill.display)},
              #{skill.learn_chance_percent},
              #{skill.random_combat ? "true" : "false"},
              #{visual},
          },
    CPP
  end.join

  random_skill_rows = RANDOM_COMBAT_SKILLS.map do |skill_id|
    "    fl::skills::SkillKey{#{skill_symbol_cpp(skill_id)}},"
  end.join("\n")

  monster_rows = MONSTERS.map do |monster|
    known_skills = monster.known_skills.map { |skill_id| skill_symbol_cpp(skill_id) }
                                .join(", ")
    <<~CPP
          ExpectedMonster{
              #{monster_cpp(monster)},
              #{cpp_string(monster.display)},
              std::array<fl::skills::SkillId, #{monster.known_skills.size}>{#{known_skills}},
              Pool::#{pool_cpp(monster.pool)},
          },
    CPP
  end.join

  <<~CPP
    // Generated by scripts/fairlanes_content_codegen.rb. Do not edit by hand.
    #include <catch2/catch_test_macros.hpp>

    #include <algorithm>
    #include <array>
    #include <optional>
    #include <span>
    #include <string_view>
    #include <tuple>

    #include "fl/ecs/components/skill_slots.hpp"
    #include "fl/ecs/components/stats.hpp"
    #include "fl/grand_central.hpp"
    #include "fl/monsters/monster_kind.hpp"
    #include "fl/monsters/monster_registry.hpp"
    #include "fl/monsters/register_monsters.hpp"
    #include "fl/primitives/encounter_builder.hpp"
    #include "fl/primitives/entity_builder.hpp"
    #include "fl/skills/skill.hpp"
    #include "fl/skills/skill_definition.hpp"
    #include "fl/skills/skill_visuals.hpp"
    #include "fl/widgets/effects/decal.hpp"

    namespace {

    enum class Pool {
      CommonWoodland,
      RareWoodland,
    };

    struct ExpectedSkill {
      fl::skills::SkillKey skill;
      std::string_view skill_name;
      int learn_chance_percent;
      bool random_combat;
      std::optional<fl::widgets::effects::DecalAnimationKind> visual;
    };

    template <std::size_t KnownSkillCount>
    struct ExpectedMonster {
      fl::monster::MonsterKind monster;
      std::string_view monster_name;
      std::array<fl::skills::SkillId, KnownSkillCount> known_skills;
      Pool pool;
    };

    constexpr std::array<ExpectedSkill, #{SKILLS.size}> kExpectedSkills{{
    #{skill_rows}
    }};

    constexpr std::array<fl::skills::SkillKey, #{RANDOM_COMBAT_SKILLS.size}> kExpectedRandomCombatSkills{{
    #{random_skill_rows}
    }};

    constexpr auto kExpectedMonsters = std::tuple{
    #{monster_rows}
    };

    template <class Fn> void for_each_expected_monster(Fn &&fn) {
      std::apply([&](const auto &...monster) { (fn(monster), ...); },
                 kExpectedMonsters);
    }

    } // namespace

    TEST_CASE("Generated skill declarations mirror runtime definitions",
              "[generated][content][skills]") {
      for (const auto &expected : kExpectedSkills) {
        CAPTURE(expected.skill_name);
        REQUIRE(fl::skills::name(expected.skill) == expected.skill_name);
        REQUIRE(fl::skills::learn_chance_percent(expected.skill) ==
                expected.learn_chance_percent);
        REQUIRE(fl::skills::decal_animation_for(expected.skill) == expected.visual);
      }
    }

    TEST_CASE("Generated skill declarations satisfy content invariants",
              "[generated][content][skills][invariants]") {
      REQUIRE(fl::skills::all_definitions().size() == kExpectedSkills.size());
      REQUIRE(fl::skills::kRandomCombatSkills == kExpectedRandomCombatSkills);

      for (std::size_t i = 0; i < kExpectedSkills.size(); ++i) {
        const auto &skill = kExpectedSkills[i];
        CAPTURE(skill.skill_name);
        REQUIRE_FALSE(skill.skill_name.empty());
        REQUIRE(skill.learn_chance_percent >= 0);
        REQUIRE(skill.learn_chance_percent <= 100);

        for (std::size_t j = i + 1; j < kExpectedSkills.size(); ++j) {
          REQUIRE(skill.skill.base != kExpectedSkills[j].skill.base);
        }
      }

      REQUIRE_FALSE(std::find(fl::skills::kRandomCombatSkills.begin(),
                              fl::skills::kRandomCombatSkills.end(),
                              fl::skills::SkillKey{fl::skills::SkillId::Observe}) !=
                    fl::skills::kRandomCombatSkills.end());
    }

    TEST_CASE("Generated monster declarations mirror runtime registry",
              "[generated][content][monsters]") {
      fl::monster::register_all_monsters();
      fl::GrandCentral gc{1, 1, 1};
      auto account_ctx = gc.account_context(0);
      auto party_ctx = account_ctx.party_context(0);
      auto build_ctx = party_ctx.build_context();

      for_each_expected_monster([&](const auto &expected) {
        CAPTURE(expected.monster_name);
        auto &registry = fl::monster::monster_registry();
        const auto found = registry.find(expected.monster);
        REQUIRE(found != registry.end());
        REQUIRE(found->second.known_skills.size() ==
                expected.known_skills.size());
        for (const auto skill : expected.known_skills) {
          REQUIRE(std::find(found->second.known_skills.begin(),
                            found->second.known_skills.end(),
                            skill) != found->second.known_skills.end());
        }

        const auto entity =
            fl::primitives::EntityBuilder(build_ctx).monster(expected.monster).build();
        const auto &stats =
            party_ctx.reg().get<fl::ecs::components::Stats>(entity);
        REQUIRE(stats.name_ == expected.monster_name);
        const auto &slots =
            party_ctx.reg().get<fl::ecs::components::SkillSlots>(entity);
        for (const auto skill : expected.known_skills) {
          REQUIRE(slots.knows(skill));
        }
      });
    }

    TEST_CASE("Generated encounter pool declarations mirror runtime pools",
              "[generated][content][encounter_builder]") {
      for_each_expected_monster([](const auto &expected) {
        CAPTURE(expected.monster_name);
        if (expected.pool == Pool::RareWoodland) {
          const auto &pool = fl::primitives::EncounterBuilder::kRareWoodland;
          REQUIRE(std::find(pool.begin(), pool.end(), expected.monster) !=
                  pool.end());
        } else {
          const auto &pool = fl::primitives::EncounterBuilder::kCommonWoodland;
          REQUIRE(std::find(pool.begin(), pool.end(), expected.monster) !=
                  pool.end());
        }
      });
    }
  CPP
end

def generate_report
  skill_metadata_by_id = SKILLS.to_h { |skill| [skill.id, skill] }
  monsters_by_skill = Hash.new { |hash, key| hash[key] = [] }
  MONSTERS.each do |monster|
    monster.known_skills.each { |skill| monsters_by_skill[skill] << monster }
  end

  lines = [
    "# Fairlanes Generated Content Balance Report",
    "",
    "Generated by `scripts/fairlanes_content_codegen.rb`.",
    "",
    "Generated artifacts currently validate handwritten C++ skill definitions, monster topology, tests, and this report.",
    "",
    "## Summary",
    "",
    "| Projection | Count | Source of truth |",
    "| --- | ---: | --- |",
    "| Skill definitions | #{SKILLS.size} | handwritten C++ `SkillDefinition` functions |",
    "| Random combat skills | #{RANDOM_COMBAT_SKILLS.size} | handwritten C++ random combat table |",
    "| Monster declarations | #{MONSTERS.size} | handwritten C++ monster registry |",
    "| Common woodland monsters | #{MONSTERS.count { |monster| monster.pool == :common_woodland }} | handwritten `EncounterBuilder` pool |",
    "| Rare woodland monsters | #{MONSTERS.count { |monster| monster.pool == :rare_woodland }} | handwritten `EncounterBuilder` pool |",
    "",
    "## Skill Metadata",
    "",
    "| Skill | C++ ID | Learn chance | Random combat | Execution | Visual | Monsters | Shape |",
    "| --- | --- | ---: | --- | --- | --- | --- | --- |"
  ]

  SKILLS.each do |skill|
    lines << [
      skill.display,
      skill.cpp_id,
      skill.learn_chance_percent,
      skill.random_combat ? "yes" : "no",
      skill.execution,
      skill.visual ? VISUAL_CPP.fetch(skill.visual) : "",
      monsters_by_skill[skill.id].map(&:display).join(", "),
      skill.declarative_shape
    ].join(" | ").then { |row| "| #{row} |" }
  end

  lines.concat [
    "",
    "## Monster Topology",
    "",
    "| Monster | C++ ID | Known skills | Pool |",
    "| --- | --- | --- | --- |"
  ]

  MONSTERS.each do |monster|
    lines << [
      monster.display,
      monster.cpp_id,
      monster.known_skills.map { |skill| skill_metadata_by_id.fetch(skill).display }.join(", "),
      monster.pool
    ].join(" | ").then { |row| "| #{row} |" }
  end

  lines.concat [
    "",
    "## Decal Skill Projection",
    "",
    "| Skill | Monster | Visual | Tags |",
    "| --- | --- | --- | --- |"
  ]

  SKILLS.select(&:visual).each do |skill|
    lines << [
      skill.display,
      monsters_by_skill[skill.id].map(&:display).join(", "),
      VISUAL_CPP.fetch(skill.visual),
      skill.tags.join(", ")
    ].join(" | ").then { |row| "| #{row} |" }
  end

  "#{lines.join("\n")}\n"
end

options = {}
OptionParser.new do |parser|
  parser.banner = "Usage: ruby scripts/fairlanes_content_codegen.rb --out-dir DIR"
  parser.on("--out-dir DIR", "Directory for generated artifacts") do |dir|
    options[:out_dir] = dir
  end
end.parse!

unless options[:out_dir]
  warn "missing required --out-dir"
  exit 2
end

validate!

FileUtils.mkdir_p(options[:out_dir])
File.write(File.join(options[:out_dir], "generated_decal_content_tests.cpp"), generate_test_cpp)
File.write(File.join(options[:out_dir], "decal_content_balance.md"), generate_report)
