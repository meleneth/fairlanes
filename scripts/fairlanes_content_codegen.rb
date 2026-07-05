#!/usr/bin/env ruby
# frozen_string_literal: true

require "fileutils"
require "json"
require "optparse"
require "set"

DECLARATIONS_FILE = File.expand_path("fairlanes_content_declarations.rb", __dir__)

Skill = Struct.new(
  :id, :cpp_id, :display, :learn_chance_percent, :random_combat,
  :flee_success_percent, :execution, :visual, :tags, :declarative_shape,
  keyword_init: true
)

Monster = Struct.new(
  :id, :cpp_id, :display, :known_skills, :pool,
  keyword_init: true
)

VISUAL_CPP = {}
SKILLS = []
RANDOM_COMBAT_SKILLS = []
MONSTERS = []

VALID_POOLS = Set[:common_woodland, :rare_woodland].freeze
VALID_SKILL_CPP_IDS = Set[
  "Observe",
  "Flee",
  "Thump",
  "Eviscerate",
  "Poison",
  "ColdSnap",
  "FlameStrike",
  "FlameWave",
  "Joltspasm",
  "RocksFall",
  "SourBreath",
  "Mercyburst",
  "BloodBloom",
  "IceSplitter",
  "GravitySigh",
  "Bump",
  "Squish",
  "Smack"
].freeze
VALID_MONSTER_CPP_IDS = Set[
  "FieldMouse",
  "HoneyBadger",
  "BumpkinHare",
  "ScaredyCat",
  "MireSquish",
  "BarkSmack",
  "PoisonToad",
  "Yeti",
  "Salamander",
  "FireDrake",
  "StormtickImp",
  "CeilingGrudge",
  "MiasmaToad",
  "ChoirWisp",
  "GorecapSprout",
  "RimefangHare",
  "NullMote"
].freeze
VALID_EXECUTIONS = Set[
  :thump_like,
  :eviscerate,
  :poison,
  :cold_snap,
  :flame_strike,
  :flame_wave,
  :decal_strike,
  :flee,
  :observe
].freeze
VALID_TAGS = Set[
  :physical,
  :blunt,
  :piercing,
  :slashing,
  :bleed,
  :poison,
  :disease,
  :acid,
  :fire,
  :cold,
  :lightning,
  :earth,
  :gravity,
  :sonic,
  :healing,
  :holy,
  :control,
  :area,
  :projectile,
  :melee,
  :observe,
  :utility,
  :escape
].freeze
VALID_VISUAL_CPP = Set[
  "FlameWave",
  "Shock",
  "RocksFall",
  "PoisonCloud",
  "HolyNova",
  "BloodBloom",
  "FrostCrack",
  "VoidRipple",
  "HitpointNumber"
].freeze
VALID_DECLARATIVE_SHAPES = Set[
  :handwritten_behavior,
  :decal_strike
].freeze

def visual(id, cpp:)
  VISUAL_CPP[id] = cpp
end

def skill(id, cpp_id:, display:, learn_chance_percent:, random_combat:,
          flee_success_percent: 0,
          execution:, visual: nil, tags:, declarative_shape:)
  SKILLS << Skill.new(
    id: id,
    cpp_id: cpp_id,
    display: display,
    learn_chance_percent: learn_chance_percent,
    random_combat: random_combat,
    flee_success_percent: flee_success_percent,
    execution: execution,
    visual: visual,
    tags: tags,
    declarative_shape: declarative_shape
  )
end

def random_combat_skills(*ids)
  RANDOM_COMBAT_SKILLS.replace(ids)
end

def monster(id, cpp_id:, display:, known_skills:, pool:)
  MONSTERS << Monster.new(
    id: id,
    cpp_id: cpp_id,
    display: display,
    known_skills: known_skills,
    pool: pool
  )
end

load DECLARATIONS_FILE

VISUAL_CPP.freeze
SKILLS.each(&:freeze)
SKILLS.freeze
RANDOM_COMBAT_SKILLS.freeze
MONSTERS.each(&:freeze)
MONSTERS.freeze

def validate!
  errors = []

  skill_ids = SKILLS.map(&:id)
  monster_ids = MONSTERS.map(&:id)
  errors << "duplicate skill ids" unless skill_ids.uniq.size == skill_ids.size
  errors << "duplicate monster ids" unless monster_ids.uniq.size == monster_ids.size

  skills_by_id = SKILLS.to_h { |skill| [skill.id, skill] }
  random_skill_ids = SKILLS.select(&:random_combat).map(&:id)

  VISUAL_CPP.each do |id, cpp|
    errors << "visual #{id} has invalid C++ id #{cpp}" unless VALID_VISUAL_CPP.include?(cpp)
  end

  SKILLS.each do |skill|
    errors << "skill #{skill.id} is missing a C++ id" if skill.cpp_id.to_s.empty?
    errors << "skill #{skill.id} has invalid C++ id #{skill.cpp_id}" unless VALID_SKILL_CPP_IDS.include?(skill.cpp_id)
    errors << "skill #{skill.id} is missing a display name" if skill.display.to_s.empty?
    if skill.learn_chance_percent.negative? || skill.learn_chance_percent > 100
      errors << "skill #{skill.id} has invalid learn chance #{skill.learn_chance_percent}"
    end
    if skill.flee_success_percent.negative? || skill.flee_success_percent > 100
      errors << "skill #{skill.id} has invalid flee success #{skill.flee_success_percent}"
    end
    if skill.execution != :flee && skill.flee_success_percent != 0
      errors << "skill #{skill.id} has flee success but is not a flee execution"
    end
    errors << "skill #{skill.id} has invalid execution #{skill.execution}" unless VALID_EXECUTIONS.include?(skill.execution)
    errors << "skill #{skill.id} has invalid visual #{skill.visual}" if skill.visual && !VISUAL_CPP.key?(skill.visual)
    errors << "skill #{skill.id} has no tags" if skill.tags.empty?
    skill.tags.each do |tag|
      errors << "skill #{skill.id} has invalid tag #{tag}" unless VALID_TAGS.include?(tag)
    end
    unless skill.tags.uniq.size == skill.tags.size
      errors << "skill #{skill.id} has duplicate tags"
    end
    unless VALID_DECLARATIVE_SHAPES.include?(skill.declarative_shape)
      errors << "skill #{skill.id} has invalid declarative shape #{skill.declarative_shape}"
    end
  end

  unless RANDOM_COMBAT_SKILLS.uniq.size == RANDOM_COMBAT_SKILLS.size
    errors << "random combat skill order has duplicate entries"
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
    errors << "monster #{monster.id} has invalid C++ id #{monster.cpp_id}" unless VALID_MONSTER_CPP_IDS.include?(monster.cpp_id)
    errors << "monster #{monster.id} is missing a display name" if monster.display.to_s.empty?
    errors << "monster #{monster.id} has invalid pool #{monster.pool}" unless VALID_POOLS.include?(monster.pool)
    errors << "monster #{monster.id} has no known skills" if monster.known_skills.empty?
    unless monster.known_skills.uniq.size == monster.known_skills.size
      errors << "monster #{monster.id} has duplicate known skills"
    end
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

def cpp_enum_name(id)
  id.to_s.split("_").map(&:capitalize).join
end

def execution_cpp(skill)
  "fl::skills::SkillExecutionKind::#{cpp_enum_name(skill.execution)}"
end

def tag_cpp(tag)
  "fl::skills::SkillTag::#{cpp_enum_name(tag)}"
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
              #{skill.flee_success_percent},
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

  skill_tag_sections = SKILLS.map do |skill|
    tag_checks = skill.tags.map do |tag|
      "      REQUIRE(fl::skills::has_tag(skill, #{tag_cpp(tag)}));"
    end.join("\n")
    <<~CPP
        SECTION(#{cpp_string(skill.display)}) {
          const auto skill = #{skill_key_cpp(skill)};
          const auto &definition = fl::skills::definition(skill);
          REQUIRE(definition.execution == #{execution_cpp(skill)});
          REQUIRE(definition.tags.size() == #{skill.tags.size});
    #{tag_checks}
        }
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
      int flee_success_percent;
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
        REQUIRE(fl::skills::definition(expected.skill).flee_success_percent ==
                expected.flee_success_percent);
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
        REQUIRE(skill.flee_success_percent >= 0);
        REQUIRE(skill.flee_success_percent <= 100);
        if (fl::skills::definition(skill.skill).execution !=
            fl::skills::SkillExecutionKind::Flee) {
          REQUIRE(skill.flee_success_percent == 0);
        }

        for (std::size_t j = i + 1; j < kExpectedSkills.size(); ++j) {
          REQUIRE(skill.skill.base != kExpectedSkills[j].skill.base);
        }
      }

      REQUIRE_FALSE(std::find(fl::skills::kRandomCombatSkills.begin(),
                              fl::skills::kRandomCombatSkills.end(),
                              fl::skills::SkillKey{fl::skills::SkillId::Observe}) !=
                    fl::skills::kRandomCombatSkills.end());
    }

    TEST_CASE("Generated skill tags and execution kinds mirror runtime definitions",
              "[generated][content][skills][tags]") {
    #{skill_tag_sections}
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
    "| Skill | C++ ID | Learn chance | Flee success | Random combat | Execution | Visual | Monsters | Shape |",
    "| --- | --- | ---: | ---: | --- | --- | --- | --- | --- |"
  ]

  SKILLS.each do |skill|
    lines << [
      skill.display,
      skill.cpp_id,
      skill.learn_chance_percent,
      skill.flee_success_percent,
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

def generate_gallery
  monsters_by_skill = Hash.new { |hash, key| hash[key] = [] }
  MONSTERS.each do |monster|
    monster.known_skills.each { |skill| monsters_by_skill[skill] << monster }
  end

  visual_skills = SKILLS.select(&:visual).group_by(&:visual)

  lines = [
    "# Fairlanes Generated Effect Gallery Metadata",
    "",
    "Generated by `scripts/fairlanes_content_codegen.rb`.",
    "",
    "This is review metadata for the handwritten decal renderer and skill definitions. It is not runtime source of truth.",
    "",
    "| Visual | Skills | Monsters | Tags | Shape |",
    "| --- | --- | --- | --- | --- |"
  ]

  VISUAL_CPP.each do |visual_id, cpp_id|
    skills = visual_skills.fetch(visual_id, [])
    next if skills.empty?

    monsters = skills.flat_map { |skill| monsters_by_skill[skill.id] }.uniq
    tags = skills.flat_map(&:tags).uniq
    shapes = skills.map(&:declarative_shape).uniq

    lines << [
      cpp_id,
      skills.map(&:display).join(", "),
      monsters.map(&:display).join(", "),
      tags.join(", "),
      shapes.join(", ")
    ].join(" | ").then { |row| "| #{row} |" }
  end

  lines.concat [
    "",
    "## Skill Entries",
    "",
    "| Skill | Visual | Execution | Monsters | Tags |",
    "| --- | --- | --- | --- | --- |"
  ]

  SKILLS.select(&:visual).each do |skill|
    lines << [
      skill.display,
      VISUAL_CPP.fetch(skill.visual),
      skill.execution,
      monsters_by_skill[skill.id].map(&:display).join(", "),
      skill.tags.join(", ")
    ].join(" | ").then { |row| "| #{row} |" }
  end

  "#{lines.join("\n")}\n"
end

def generate_manifest
  {
    "schema_version" => 1,
    "source" => "scripts/fairlanes_content_declarations.rb",
    "runtime_authority" => "handwritten C++ SkillDefinition, monster registry, and encounter pool tables",
    "visuals" => VISUAL_CPP.map do |id, cpp_id|
      {
        "id" => id.to_s,
        "cpp_id" => cpp_id
      }
    end,
    "skills" => SKILLS.map do |skill|
      {
        "id" => skill.id.to_s,
        "cpp_id" => skill.cpp_id,
        "display" => skill.display,
        "learn_chance_percent" => skill.learn_chance_percent,
        "flee_success_percent" => skill.flee_success_percent,
        "random_combat" => skill.random_combat,
        "execution" => skill.execution.to_s,
        "visual" => skill.visual&.to_s,
        "tags" => skill.tags.map(&:to_s),
        "declarative_shape" => skill.declarative_shape.to_s
      }
    end,
    "random_combat_skills" => RANDOM_COMBAT_SKILLS.map(&:to_s),
    "monsters" => MONSTERS.map do |monster|
      {
        "id" => monster.id.to_s,
        "cpp_id" => monster.cpp_id,
        "display" => monster.display,
        "known_skills" => monster.known_skills.map(&:to_s),
        "pool" => monster.pool.to_s
      }
    end
  }.then { |manifest| "#{JSON.pretty_generate(manifest)}\n" }
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
File.write(File.join(options[:out_dir], "effect_gallery.md"), generate_gallery)
File.write(File.join(options[:out_dir], "content_manifest.json"), generate_manifest)
