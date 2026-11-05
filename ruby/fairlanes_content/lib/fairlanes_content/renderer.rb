# frozen_string_literal: true

require "json"
require "fairlanes_content/floor_resolver"

module FairlanesContent
  class Renderer
    def initialize(declarations)
      @declarations = declarations
    end

    def skills = declarations.skills
    def monsters = declarations.monsters
    def statuses = declarations.statuses
    def visuals = declarations.visuals
    def random_combat_skills = declarations.random_combat_skills
    def gears = declarations.gears
    def encounter_pools = declarations.encounter_pools
    def tower_patterns = declarations.tower_patterns
    def biomes = declarations.biomes
    def regressions = declarations.regressions
    def biome_plans = declarations.biome_plans
    def recruitment_opportunities = declarations.recruitment_opportunities
    def story_events = declarations.story_events
    def objectives = declarations.objectives
    def dialogues = declarations.dialogues
    def autoplay_routes = declarations.autoplay_routes

    def recruitment_content_hpp
      rows = recruitment_opportunities.map do |entry|
        trigger = entry.biome ? "Trigger::TowerMilestone" : "Trigger::PartyCompleted"
        biome = cpp_string(entry.biome&.to_s || "")
        floor = entry.floor || 0
        completed_party = entry.unlocked_by_party_completion || 0
        "  Opportunity{#{cpp_string(entry.id.to_s)}, fl::primitives::Cycle::#{camel(entry.cycle)}, #{trigger}, #{biome}, #{floor}, #{entry.destination_party}, #{completed_party}},"
      end.join("\n")
      <<~CPP
        #pragma once
        #include <array>
        #include <string_view>
        #include "fl/primitives/cycle.hpp"

        namespace fl::recruitment::generated_content {
        enum class Trigger { TowerMilestone, PartyCompleted };
        struct Opportunity {
          std::string_view id;
          fl::primitives::Cycle cycle;
          Trigger trigger;
          std::string_view biome_id;
          int floor;
          int destination_party;
          int completed_party;
        };
        inline constexpr std::array<Opportunity, #{recruitment_opportunities.size}> kOpportunities{{
        #{rows}
        }};
        } // namespace fl::recruitment::generated_content
      CPP
    end

    def recruitment_progression_doc
      lines = ["# Origin recruitment progression", "",
               "Generated from the Ruby content DSL. Party numbers are one-based.", "",
               "| Opportunity | Trigger | Destination |", "| --- | --- | --- |"]
      recruitment_opportunities.each do |entry|
        trigger = entry.biome ? "`#{entry.biome}` floor #{entry.floor}" :
          "party #{entry.unlocked_by_party_completion} roster completed"
        lines << "| `#{entry.id}` | #{trigger} | party #{entry.destination_party} |"
      end
      lines << ""
      lines.join("\n")
    end

    def story_content_hpp
      event_rows = story_events.map { |event| "  Event{#{cpp_string(event.id.to_s)}, #{event.handled ? 'true' : 'false'}}," }.join("\n")
      objective_rows = objectives.map do |objective|
        "  Objective{#{cpp_string(objective.id.to_s)}, #{cpp_string(objective.title)}, #{cpp_string(objective.description)}, #{cpp_string(objective.revealed_by.to_s)}, #{cpp_string(objective.completed_by.to_s)}},"
      end.join("\n")
      definitions = dialogues.each_with_index.map do |dialogue, di|
        choice_tables = dialogue.nodes.each_with_index.map do |node, ni|
          rows = node[:choices].map do |choice|
            "  Choice{#{cpp_string(choice[:id].to_s)}, #{cpp_string(choice[:label])}, #{cpp_string(choice.fetch(:event, '').to_s)}, #{cpp_string(choice[:next].to_s)}},"
          end.join("\n")
          "inline constexpr std::array<Choice, #{node[:choices].size}> kChoices#{di}_#{ni}{{\n#{rows}\n}};"
        end.join("\n")
        node_rows = dialogue.nodes.each_with_index.map do |node, ni|
          "  Node{#{cpp_string(node[:id].to_s)}, #{cpp_string(node[:speaker])}, #{cpp_string(node[:text])}, kChoices#{di}_#{ni}},"
        end.join("\n")
        "#{choice_tables}\ninline constexpr std::array<Node, #{dialogue.nodes.size}> kNodes#{di}{{\n#{node_rows}\n}};"
      end.join("\n")
      dialogue_rows = dialogues.each_with_index.map do |dialogue, di|
        "  Dialogue{#{cpp_string(dialogue.id.to_s)}, #{cpp_string(dialogue.location.to_s)}, #{cpp_string(dialogue.npc)}, #{cpp_string(dialogue.start.to_s)}, kNodes#{di}},"
      end.join("\n")
      autoplay_tables = autoplay_routes.each_with_index.map do |route, ri|
        rows = route.steps.map do |step|
          action = { choose_dialogue: "ChooseDialogue", return_to_town: "ReturnToTown", buy_gear: "BuyGear" }.fetch(step[:action])
          "  AutoplayStep{#{cpp_string(step[:wait_for].to_s)}, AutoplayAction::#{action}, #{cpp_string(step.fetch(:target, '').to_s)}, #{cpp_string(step.fetch(:choice, '').to_s)}},"
        end.join("\n")
        "inline constexpr std::array<AutoplayStep, #{route.steps.size}> kAutoplaySteps#{ri}{{\n#{rows}\n}};"
      end.join("\n")
      autoplay_rows = autoplay_routes.each_with_index.map do |route, ri|
        "  AutoplayRoute{#{cpp_string(route.id.to_s)}, kAutoplaySteps#{ri}},"
      end.join("\n")
      <<~CPP
        #pragma once
        #include <array>
        #include <span>
        #include <string_view>
        namespace fl::story::generated_content {
        struct Event { std::string_view id; bool handled; };
        struct Objective {
          std::string_view id, title, description, revealed_by, completed_by;
        };
        struct Choice { std::string_view id, label, event_id, next_node; };
        struct Node { std::string_view id, speaker, text; std::span<const Choice> choices; };
        struct Dialogue {
          std::string_view id, location, npc, start_node;
          std::span<const Node> nodes;
        };
        enum class AutoplayAction { ChooseDialogue, ReturnToTown, BuyGear };
        struct AutoplayStep {
          std::string_view wait_for;
          AutoplayAction action;
          std::string_view target, choice;
        };
        struct AutoplayRoute { std::string_view id; std::span<const AutoplayStep> steps; };
        inline constexpr std::array<Event, #{story_events.size}> kEvents{{
        #{event_rows}
        }};
        inline constexpr std::array<Objective, #{objectives.size}> kObjectives{{
        #{objective_rows}
        }};
        #{definitions}
        inline constexpr std::array<Dialogue, #{dialogues.size}> kDialogues{{
        #{dialogue_rows}
        }};
        #{autoplay_tables}
        inline constexpr std::array<AutoplayRoute, #{autoplay_routes.size}> kAutoplayRoutes{{
        #{autoplay_rows}
        }};
        inline const Dialogue *find_dialogue(std::string_view id) {
          for (const auto &dialogue : kDialogues) if (dialogue.id == id) return &dialogue;
          return nullptr;
        }
        inline const Node *find_node(const Dialogue &dialogue, std::string_view id) {
          for (const auto &node : dialogue.nodes) if (node.id == id) return &node;
          return nullptr;
        }
        } // namespace fl::story::generated_content
      CPP
    end

    def story_event_doc
      lines = ["# Story events", "", "Generated from the Ruby story DSL. `Handled` entries must match the handwritten dispatcher.", "", "| Event ID | Handler | Dialogue emitter |", "| --- | --- | --- |"]
      story_events.each do |event|
        emitters = dialogues.flat_map do |dialogue|
          dialogue.nodes.flat_map do |node|
            node[:choices].filter_map { |choice| dialogue.id if choice[:event] == event.id }
          end
        end.uniq
        lines << "| `#{event.id}` | #{event.handled ? 'Handled' : '**Unimplemented**'} | #{emitters.empty? ? '—' : emitters.map { |id| "`#{id}`" }.join(', ')} |"
      end
      lines << ""
      lines << "## Dialogue events without handlers"
      lines << ""
      missing = story_events.reject(&:handled).select do |event|
        dialogues.any? { |dialogue| dialogue.nodes.any? { |node| node[:choices].any? { |choice| choice[:event] == event.id } } }
      end
      lines.concat(missing.empty? ? ["None."] : missing.map { |event| "- `#{event.id}`" })
      lines << ""
      lines << "## Autoplay enabled routes"
      lines << ""
      autoplay_routes.each do |route|
        lines << "### `#{route.id}`"
        lines << ""
        lines << "| Wait for fact | Action | Target | Choice |"
        lines << "| --- | --- | --- | --- |"
        route.steps.each do |step|
          lines << "| `#{step[:wait_for]}` | `#{step[:action]}` | #{step[:target] ? "`#{step[:target]}`" : '—'} | #{step[:choice] ? "`#{step[:choice]}`" : '—'} |"
        end
        lines << ""
      end
      lines.join("\n")
    end

    def biome_plan_doc
      lines = ["# Biome tower map", "",
               "Generated from scripts/fairlanes_content_declarations.rb. Edit the Ruby DSL, then regenerate this map.",
               "Each Cycle has five ordered 100-floor tower biomes. `Playable` means the biome has a resolved floor table; `Planned` means its tower content still needs authoring.", ""]
      CYCLES.each do |cycle|
        lines << "## #{cycle.to_s.capitalize}"
        lines << ""
        biome_plans.select { |plan| plan.cycle == cycle }.sort_by(&:order).each do |plan|
          playable = biomes.any? { |biome| biome.id == plan.id }
          lines << "### #{plan.order}. `#{plan.id}` — #{playable ? 'Playable' : 'Planned'}"
          lines << ""
          lines << "| Monster ID | Current skills | Current HP | Current runtime pool |"
          lines << "| --- | --- | ---: | --- |"
          plan.monsters.each do |id|
            monster = monsters.find { |entry| entry.id == id }
            lines << "| `#{id}` | #{monster.known_skills.map { |skill| "`#{skill}`" }.join(', ')} | #{monster.hp} | `#{monster.pool}` |"
          end
          lines << ""
        end
      end
      lines << "## Declared monsters without a biome"
      lines << ""
      assigned = biome_plans.flat_map(&:monsters)
      CYCLES.each do |cycle|
        missing = monsters.select { |monster| monster.cycle == cycle && !assigned.include?(monster.id) }
        lines << "### #{cycle.to_s.capitalize}"
        lines << ""
        lines << "| Monster ID | Current skills | Pool |"
        lines << "| --- | --- | --- |"
        missing.each do |monster|
          lines << "| `#{monster.id}` | #{monster.known_skills.map { |skill| "`#{skill}`" }.join(', ')} | `#{monster.pool}` |"
        end
        lines << "| — | None | — |" if missing.empty?
        lines << ""
      end
      lines << "## Declared skills without a monster"
      lines << ""
      used = monsters.flat_map(&:known_skills).uniq
      unused = skills.reject { |skill| used.include?(skill.id) }
      lines << (unused.empty? ? "None." : unused.map { |skill| "- `#{skill.id}`" }.join("\n"))
      lines << ""
      lines << "Monster skills and HP above are the current Ruby declarations, not future balance promises."
      lines << ""
      lines.join("\n")
    end

    def regression_content_hpp
      definitions = regressions.each_with_index.map do |scenario, index|
        members = scenario.party.map do |member|
          "  MemberSetup{#{member[:hp]}, #{member[:attack]}, #{member[:armor]}, #{resistance_cpp(member.fetch(:resistances, {}))}, fl::skills::SkillId::#{skills.find { |skill| skill.id == member[:skill] }.cpp_id}, ActionPolicy::SingleEquippedSkill},"
        end.join("\n")
        seeds = scenario.seeds.join(", ")
        encounters = scenario.encounters.map do |encounter|
          "  EncounterSetup{#{cpp_string(encounter[:biome].to_s)}, #{encounter[:floor]}},"
        end.join("\n")
        variants = scenario.variants.map do |variant|
          wins = variant[:expected_wins]
          item = variant[:equipped_item] ? "fl::loot::generated_content::GearId::#{camel(variant[:equipped_item])}" : "std::nullopt"
          overrides = (0...5).map do |member_index|
            skill_id = variant.fetch(:skill_overrides, {})[member_index]
            skill_id ? "fl::skills::SkillId::#{skills.find { |skill| skill.id == skill_id }.cpp_id}" : "std::nullopt"
          end.join(", ")
          observers = (0...5).map { |member_index| variant.fetch(:observe_members, []).include?(member_index) ? "true" : "false" }.join(", ")
          next_skill = variant[:next_skill] ? "fl::skills::SkillId::#{skills.find { |skill| skill.id == variant[:next_skill] }.cpp_id}" : "std::nullopt"
          "  Variant{#{cpp_string(variant[:id].to_s)}, #{item}, {#{overrides}}, {#{observers}}, #{next_skill}, #{wins ? wins.begin : 0}, #{wins ? wins.end : 0}, #{wins ? 'true' : 'false'}},"
        end.join("\n")
        <<~CPP
          inline constexpr std::array<MemberSetup, #{scenario.party.size}> kMembers#{index}{{
          #{members}
          }};
          inline constexpr std::array<EncounterSetup, #{scenario.encounters.size}> kEncounters#{index}{{
          #{encounters}
          }};
          inline constexpr std::array<std::uint64_t, #{scenario.seeds.size}> kSeeds#{index}{{#{seeds}}};
          inline constexpr std::array<Variant, #{scenario.variants.size}> kVariants#{index}{{
          #{variants}
          }};
        CPP
      end.join("\n")
      rows = regressions.each_with_index.map do |scenario, index|
        "  Scenario{#{cpp_string(scenario.id.to_s)}, kEncounters#{index}, kMembers#{index}, kSeeds#{index}, #{scenario.beat_cap}, #{scenario.recording_version}, kVariants#{index}},"
      end.join("\n")
      <<~CPP
        #pragma once
        #include <array>
        #include <cstdint>
        #include <span>
        #include <optional>
        #include <string_view>
        #include "fl/generated/gear_content.hpp"
        #include "fl/primitives/damage.hpp"
        #include "fl/skills/skill.hpp"

        namespace fl::regression::generated_content {
        enum class ActionPolicy { SingleEquippedSkill };
        struct EncounterSetup {
          std::string_view biome_id;
          int floor;
        };
        struct MemberSetup {
          int hp;
          int attack;
          int armor;
          fl::primitives::Resistances resistances;
          fl::skills::SkillId skill;
          ActionPolicy policy;
        };
        struct Variant {
          std::string_view id;
          std::optional<fl::loot::generated_content::GearId> equipped_item;
          std::array<std::optional<fl::skills::SkillId>, 5> skill_overrides;
          std::array<bool, 5> observe_equipped;
          std::optional<fl::skills::SkillId> next_skill;
          int min_wins;
          int max_wins;
          bool asserted;
        };
        struct Scenario {
          std::string_view id;
          std::span<const EncounterSetup> encounters;
          std::span<const MemberSetup> party;
          std::span<const std::uint64_t> seeds;
          std::uint64_t beat_cap;
          int recording_version;
          std::span<const Variant> variants;
        };
        #{definitions}
        inline constexpr std::array<Scenario, #{regressions.size}> kScenarios{{
        #{rows}
        }};
        inline const Scenario *find(std::string_view id) {
          for (const auto &scenario : kScenarios) if (scenario.id == id) return &scenario;
          return nullptr;
        }
        inline const Variant *find_variant(const Scenario &scenario, std::string_view id) {
          for (const auto &variant : scenario.variants) if (variant.id == id) return &variant;
          return nullptr;
        }
        } // namespace fl::regression::generated_content
      CPP
    end

    def resistance_cpp(values)
      channels = %i[physical magical fire ice lightning]
      "fl::primitives::Resistances{#{channels.map { |channel| values.fetch(channel, 0) }.join(', ')}}"
    end

    def resistance_schema
      { "type" => "object", "additionalProperties" => false,
        "properties" => %w[physical magical fire ice lightning].to_h do |channel|
          [channel, { "type" => "integer", "minimum" => 0, "maximum" => 100 }]
        end }
    end

    def gear_content_hpp
      rows = gears.map do |gear|
        <<~CPP
          SpecialItem{Weight{#{gear.weight}}, EquipmentBuilder{
              .slot = EquipmentSlot::#{gear.slot},
              .armor_kind = ArmorKind::#{gear.armor_kind},
              .tier = Tier::#{gear.tier},
              .name = #{cpp_string(gear.display)},
              .unique_id = #{cpp_string(gear.id.to_s)},
              .resistances = #{resistance_cpp(gear.modifiers.fetch(:resistances, {}))},
              .attack = #{gear.modifiers.fetch(:attack, 0)},
              .armor = #{gear.modifiers.fetch(:armor, 0)},
              .max_hp = #{gear.modifiers.fetch(:max_hp, 0)},
          }},
        CPP
      end.join
      ids = gears.map { |gear| "  #{camel(gear.id)}," }.join("\n")
      <<~CPP
        #pragma once
        #include <array>
        #include <cstddef>
        #include <vector>
        #include "fl/loot/special_item.hpp"

        namespace fl::loot::generated_content {
        enum class GearId {
        #{ids}
        };

        inline const std::array<SpecialItem, #{gears.size}> kSpecialItems{{
        #{rows}
        }};

        inline const SpecialItem &definition(GearId id) {
          return kSpecialItems.at(static_cast<std::size_t>(id));
        }

        inline std::vector<SpecialItem> special_items() {
          return {kSpecialItems.begin(), kSpecialItems.end()};
        }
        } // namespace fl::loot::generated_content
      CPP
    end

    def floor_content_hpp
      tables = biomes.map do |biome|
        rows = resolved_floors(biome).map do |floor|
          kinds = floor[:lineup].map { |id| "fl::monster::MonsterKind::#{monster_by_id(id).cpp_id}" }.join(", ")
          levels = Array.new(floor[:lineup].size, floor[:level_percent]).join(", ")
          hp = Array.new(floor[:lineup].size, floor[:hp_percent]).join(", ")
          attack = Array.new(floor[:lineup].size, floor[:attack_percent]).join(", ")
          cover = floor[:cover]
          cover_cpp = if cover
            "CoverRule{#{cover.fetch(:protected)}, #{cover.fetch(:protector)}, fl::skills::SkillId::#{skills.find { |skill| skill.id == cover.fetch(:release_skill) }.cpp_id}}"
          else
            "CoverRule{}"
          end
          "  FloorLineup{#{floor[:floor]}, #{cpp_string(floor[:id])}, {#{kinds}}, {#{levels}}, {#{hp}}, {#{attack}}, #{floor[:lineup].size}, #{cover_cpp}},"
        end.join("\n")
        <<~CPP
          inline constexpr std::array<FloorLineup, #{biome.floors}> k#{camel(biome.id)}{{
          #{rows}
          }};
        CPP
      end.join("\n")
      lookup = biomes.map do |biome|
        "  if (biome_id == #{cpp_string(biome.id.to_s)}) return &k#{camel(biome.id)}[static_cast<std::size_t>(floor - 1)];"
      end.join("\n")
      <<~CPP
        #pragma once
        #include <array>
        #include <cstddef>
        #include <string_view>
        #include "fl/monsters/monster_kind.hpp"
        #include "fl/skills/skill.hpp"

        namespace fl::primitives::generated_content {
        struct CoverRule {
          int protected_index{-1};
          int protector_index{-1};
          fl::skills::SkillId release_skill{fl::skills::SkillId::Observe};

          [[nodiscard]] constexpr bool enabled() const noexcept {
            return protected_index >= 0 && protector_index >= 0;
          }
        };

        struct FloorLineup {
          int floor;
          std::string_view id;
          std::array<fl::monster::MonsterKind, 5> monsters;
          std::array<int, 5> level_percent;
          std::array<int, 5> hp_percent;
          std::array<int, 5> attack_percent;
          std::size_t monster_count;
          CoverRule cover;
        };

        #{tables}
        inline const FloorLineup *floor_lineup(std::string_view biome_id, int floor) {
          if (floor < 1 || floor > 100) return nullptr;
        #{lookup}
          return nullptr;
        }
        } // namespace fl::primitives::generated_content
      CPP
    end

    def floor_manifest
      lines = ["# Resolved tower floors", "", "Generated from scripts/fairlanes_content_declarations.rb.", ""]
      biomes.each do |biome|
        lines.concat(["## #{biome.display} (`#{biome.id}`)", "",
                      "| Floor ID | Source | Scale | Lineup | Rule |", "| --- | --- | ---: | --- | --- |"])
        resolved_floors(biome).each do |floor|
          cover = floor[:cover]
          rule = cover ? "cover #{cover[:protected]} by #{cover[:protector]} until #{cover[:release_skill]}" : "--"
          lines << "| `#{floor[:id]}` | #{floor[:source]} | #{floor[:level_percent]}% | #{floor[:lineup].join(', ')} | #{rule} |"
        end
        lines << ""
      end
      lines.pop if lines.last == ""
      "#{lines.join("\n")}\n"
    end

    def test_cpp
      skill_rows = skills.map do |skill|
        visual = skill.visual ? visual_cpp(skill) : "std::nullopt"
        <<~CPP
              ExpectedSkill{
                  #{skill_key_cpp(skill)},
                  #{cpp_string(skill.display)},
                  #{skill.learn_chance_percent},
                  #{skill.flee_success_percent},
                  #{skill.random_combat ? "true" : "false"},
                  #{visual},
                  #{cpp_string(skill.description.to_s)},
              },
        CPP
      end.join

      random_skill_rows = random_combat_skills.map do |skill_id|
        "    fl::skills::SkillKey{#{skill_symbol_cpp(skill_id)}},"
      end.join("\n")

      monster_rows = monsters.map do |monster|
        known_skills = monster.known_skills.map { |skill_id| skill_symbol_cpp(skill_id) }
                                    .join(", ")
        <<~CPP
              ExpectedMonster{
                  #{monster_cpp(monster)},
                  #{cpp_string(monster.display)},
                  #{cpp_string(monster.description)},
                  #{monster.hp},
                  #{monster.level || 0},
                  fl::primitives::Cycle::#{camel(monster.cycle)},
                  #{resistance_cpp(monster.resistances)},
                  std::array<fl::skills::SkillId, #{monster.known_skills.size}>{#{known_skills}},
                  Pool::#{pool_cpp(monster.pool)},
              },
        CPP
      end.join

      status_rows = statuses.map do |status|
        <<~CPP
              ExpectedStatus{
                  fl::status::StatusId::#{status.cpp_id},
                  #{cpp_string(status.display)},
                  #{cpp_string(status.debug_name)},
                  #{cpp_string(status.component)},
                  #{status.palette_index},
              },
        CPP
      end.join

      skill_tag_sections = skills.map do |skill|
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
        #include "fl/ecs/components/track_xp.hpp"
        #include "fl/grand_central.hpp"
        #include "fl/primitives/cycle.hpp"
        #include "fl/monsters/monster_kind.hpp"
        #include "fl/monsters/monster_registry.hpp"
        #include "fl/monsters/register_monsters.hpp"
        #include "fl/primitives/encounter_builder.hpp"
        #include "fl/primitives/entity_builder.hpp"
        #include "fl/skills/skill.hpp"
        #include "fl/skills/skill_definition.hpp"
        #include "fl/skills/skill_visuals.hpp"
        #include "fl/generated/status_content.hpp"
        #include "fl/widgets/effects/decal.hpp"

        namespace {

        enum class Pool {
          CommonWoodland,
          RareWoodland,
          AuthoredOnly,
          Raid,
        };

        struct ExpectedSkill {
          fl::skills::SkillKey skill;
          std::string_view skill_name;
          int learn_chance_percent;
          int flee_success_percent;
          bool random_combat;
          std::optional<fl::widgets::effects::DecalAnimationKind> visual;
          std::string_view description;
        };

        struct ExpectedStatus {
          fl::status::StatusId status;
          std::string_view display_name;
          std::string_view debug_name;
          std::string_view component_name;
          int palette_index;
        };

        template <std::size_t KnownSkillCount>
        struct ExpectedMonster {
          fl::monster::MonsterKind monster;
          std::string_view monster_name;
          std::string_view description;
          int hp;
          int level;
          fl::primitives::Cycle cycle;
          fl::primitives::Resistances resistances;
          std::array<fl::skills::SkillId, KnownSkillCount> known_skills;
          Pool pool;
        };

        constexpr std::array<ExpectedSkill, #{skills.size}> kExpectedSkills{{
        #{skill_rows}
        }};

        constexpr std::array<fl::skills::SkillKey, #{random_combat_skills.size}> kExpectedRandomCombatSkills{{
        #{random_skill_rows}
        }};

        constexpr std::array<ExpectedStatus, #{statuses.size}> kExpectedStatuses{{
        #{status_rows}
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
            REQUIRE(fl::skills::definition(expected.skill).description == expected.description);
            REQUIRE(fl::skills::name(expected.skill) == expected.skill_name);
            REQUIRE(fl::skills::learn_chance_percent(expected.skill) ==
                    expected.learn_chance_percent);
            REQUIRE(fl::skills::definition(expected.skill).flee_success_percent ==
                    expected.flee_success_percent);
            REQUIRE(fl::skills::definition(expected.skill).decal_animation ==
                    expected.visual);
          }
        }

        TEST_CASE("Generated skill declarations satisfy content invariants",
                  "[generated][content][skills][invariants]") {
          REQUIRE(fl::skills::all_definitions().size() == kExpectedSkills.size());
          REQUIRE(std::ranges::equal(fl::skills::random_combat_skills(),
                                     kExpectedRandomCombatSkills));

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

          const auto random_combat_skills = fl::skills::random_combat_skills();
          REQUIRE_FALSE(std::find(random_combat_skills.begin(),
                                  random_combat_skills.end(),
                                  fl::skills::SkillKey{fl::skills::SkillId::Observe}) !=
                        random_combat_skills.end());
        }

        TEST_CASE("Generated skill tags and execution kinds mirror runtime definitions",
                  "[generated][content][skills][tags]") {
        #{skill_tag_sections}
        }

        TEST_CASE("Generated status declarations mirror runtime metadata",
                  "[generated][content][statuses]") {
          REQUIRE(fl::status::generated_content::all_statuses().size() ==
                  kExpectedStatuses.size());

          for (const auto &expected : kExpectedStatuses) {
            CAPTURE(expected.display_name);
            const auto metadata =
                fl::status::generated_content::metadata(expected.status);
            REQUIRE(metadata.display_name == expected.display_name);
            REQUIRE(metadata.debug_name == expected.debug_name);
            REQUIRE(metadata.component_name == expected.component_name);
            REQUIRE(metadata.palette_index == expected.palette_index);
          }
        }

        TEST_CASE("Generated monster declarations mirror runtime registry",
                  "[generated][content][monsters]") {
          fl::monster::register_all_monsters();
          REQUIRE(fl::monster::monster_registry().size() == #{monsters.size});

          fl::GrandCentral gc{1, 1, 1};
          auto account_ctx = gc.account_context(0);
          auto party_ctx = account_ctx.party_context(0);
          auto build_ctx = party_ctx.build_context();

          for_each_expected_monster([&](const auto &expected) {
            CAPTURE(expected.monster_name);
            const auto metadata = fl::monster::generated_content::stats(expected.monster);
            REQUIRE_FALSE(metadata.description.empty());
            REQUIRE(metadata.description == expected.description);
            REQUIRE(metadata.cycle == expected.cycle);
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
            REQUIRE(stats.hp_ == expected.hp);
            REQUIRE(stats.max_hp_ == expected.hp);
            REQUIRE(stats.resistances_.physical == expected.resistances.physical);
            REQUIRE(stats.resistances_.magical == expected.resistances.magical);
            REQUIRE(stats.resistances_.fire == expected.resistances.fire);
            REQUIRE(stats.resistances_.ice == expected.resistances.ice);
            REQUIRE(stats.resistances_.lightning == expected.resistances.lightning);
            if (expected.level > 0) {
              const auto &xp =
                  party_ctx.reg().get<fl::ecs::components::TrackXP>(entity);
              REQUIRE(xp.level_ == expected.level);
            }
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
            const auto metadata = fl::monster::generated_content::stats(expected.monster);
            REQUIRE_FALSE(metadata.description.empty());
            REQUIRE(metadata.description == expected.description);
            REQUIRE(metadata.cycle == expected.cycle);
            if (expected.pool == Pool::RareWoodland) {
              const auto pool = fl::primitives::EncounterBuilder::rare_woodland();
              REQUIRE(std::find(pool.begin(), pool.end(), expected.monster) !=
                      pool.end());
            } else if (expected.pool == Pool::AuthoredOnly) {
              const auto common = fl::primitives::EncounterBuilder::common_woodland();
              const auto rare = fl::primitives::EncounterBuilder::rare_woodland();
              const auto raid = fl::monster::generated_content::raid_bosses();
              REQUIRE(std::find(common.begin(), common.end(), expected.monster) == common.end());
              REQUIRE(std::find(rare.begin(), rare.end(), expected.monster) == rare.end());
              REQUIRE(std::find(raid.begin(), raid.end(), expected.monster) == raid.end());
            } else if (expected.pool == Pool::Raid) {
              const auto pool = fl::monster::generated_content::raid_bosses();
              REQUIRE(std::find(pool.begin(), pool.end(), expected.monster) != pool.end());
              const auto common = fl::primitives::EncounterBuilder::common_woodland();
              const auto rare = fl::primitives::EncounterBuilder::rare_woodland();
              REQUIRE(std::find(common.begin(), common.end(), expected.monster) == common.end());
              REQUIRE(std::find(rare.begin(), rare.end(), expected.monster) == rare.end());
            } else {
              const auto pool = fl::primitives::EncounterBuilder::common_woodland();
              REQUIRE(std::find(pool.begin(), pool.end(), expected.monster) !=
                      pool.end());
            }
          });
        }
      CPP
    end

    def skill_content_hpp
      <<~CPP
        // Generated by scripts/fairlanes_content_codegen.rb. Do not edit by hand.
        #pragma once

        #include <span>

        #include "fl/skills/skill_definition.hpp"

        namespace fl::skills::generated_content {

        std::span<const SkillDefinition *const> all_definitions() noexcept;

        } // namespace fl::skills::generated_content
      CPP
    end

    def skill_ids_hpp
      rows = skills.map { |skill| "  #{skill.cpp_id}," }.join("\n")
      stable_ids = skills.map do |skill|
        "  case SkillId::#{skill.cpp_id}: return #{cpp_string(skill.id.to_s)};"
      end.join("\n")

      <<~CPP
        // Generated by scripts/fairlanes_content_codegen.rb. Do not edit by hand.
        #pragma once
        #include <string_view>

        namespace fl::skills {

        enum class SkillId {
        #{rows}
        };

        inline constexpr std::string_view stable_id(SkillId id) noexcept {
          switch (id) {
        #{stable_ids}
          }
          return "unknown";
        }

        } // namespace fl::skills
      CPP
    end

    def skill_content_cpp
      includes = skills.map do |skill|
        "#include \"fl/generated/skills/#{skill.id}.hpp\""
      end.join("\n")
      definition_pointer_rows = skills.map do |skill|
        "    &generated_content::detail::k#{skill.cpp_id}Definition,"
      end.join("\n")

      random_skill_rows = random_combat_skills.map do |skill_id|
        "    fl::skills::SkillKey{#{skill_symbol_cpp(skill_id)}},"
      end.join("\n")

      <<~CPP
        // Generated by scripts/fairlanes_content_codegen.rb. Do not edit by hand.
        #include "fl/generated/skill_content.hpp"

        #{includes}

        #include <array>
        #include <span>

        namespace fl::skills {
        namespace {

        constexpr std::array<const SkillDefinition *, #{skills.size}> kSkillDefinitions{{
        #{definition_pointer_rows}
        }};

        constexpr std::array<SkillKey, #{random_combat_skills.size}> kRandomCombatSkills{{
        #{random_skill_rows}
        }};

        } // namespace

        std::span<const SkillKey> random_combat_skills() noexcept {
          return {kRandomCombatSkills.data(), kRandomCombatSkills.size()};
        }

        namespace generated_content {

        std::span<const SkillDefinition *const> all_definitions() noexcept {
          return {kSkillDefinitions.data(), kSkillDefinitions.size()};
        }

        } // namespace generated_content
        } // namespace fl::skills
      CPP
    end

    def skill_entry_hpp(skill)
      <<~CPP
        // Generated by scripts/fairlanes_content_codegen.rb. Do not edit by hand.
        #pragma once

        #include "fl/skills/skill_definition.hpp"

        namespace fl::skills::generated_content::detail {

        extern const SkillDefinition k#{skill.cpp_id}Definition;

        } // namespace fl::skills::generated_content::detail
      CPP
    end

    def skill_entry_cpp(skill)
      tags = skill.tags.map { |tag| tag_cpp(tag) }.join(", ")
      visual = skill.visual ? visual_cpp(skill) : "std::nullopt"

      <<~CPP
        // Generated by scripts/fairlanes_content_codegen.rb. Do not edit by hand.
        #include "fl/generated/skills/#{skill.id}.hpp"

        #include <array>
        #include <optional>
        #include <span>

        namespace fl::skills::generated_content::detail {
        namespace {

        constexpr std::array k#{skill.cpp_id}Tags{#{tags}};

        } // namespace

        const SkillDefinition k#{skill.cpp_id}Definition{
            #{skill_key_cpp(skill)},
            #{cpp_string(skill.display)},
            std::span<const SkillTag>{k#{skill.cpp_id}Tags.data(),
                                      k#{skill.cpp_id}Tags.size()},
            #{skill.learn_chance_percent},
            #{skill.flee_success_percent},
            #{execution_cpp(skill)},
            #{visual},
            #{cpp_string(skill.description.to_s)},
            #{skill.consumes_status ? "fl::ecs::components::CombatStatusKind::#{camel(skill.consumes_status)}" : "std::nullopt"},
            #{skill.effect_damage},
        };

        } // namespace fl::skills::generated_content::detail
      CPP
    end

    def status_content_hpp
      rows = statuses.map { |status| "  #{status.cpp_id}," }.join("\n")

      <<~CPP
        // Generated by scripts/fairlanes_content_codegen.rb. Do not edit by hand.
        #pragma once

        #include <span>
        #include <string_view>

        namespace fl::status {

        enum class StatusId {
        #{rows}
        };

        struct StatusMetadata {
          StatusId id;
          std::string_view display_name;
          std::string_view debug_name;
          std::string_view component_name;
          int palette_index;
        };

        namespace generated_content {

        StatusMetadata metadata(StatusId status) noexcept;
        std::span<const StatusMetadata> all_statuses() noexcept;

        } // namespace generated_content
        } // namespace fl::status
      CPP
    end

    def status_content_cpp
      status_rows = statuses.map do |status|
        <<~CPP
            StatusMetadata{
                StatusId::#{status.cpp_id},
                #{cpp_string(status.display)},
                #{cpp_string(status.debug_name)},
                #{cpp_string(status.component)},
                #{status.palette_index},
            },
        CPP
      end.join

      status_cases = statuses.each_with_index.map do |status, index|
        <<~CPP
            case StatusId::#{status.cpp_id}:
              return kStatuses[#{index}];
        CPP
      end.join

      <<~CPP
        // Generated by scripts/fairlanes_content_codegen.rb. Do not edit by hand.
        #include "fl/generated/status_content.hpp"

        #include <array>
        #include <span>
        #include <string_view>

        namespace fl::status::generated_content {
        namespace {

        constexpr std::array<StatusMetadata, #{statuses.size}> kStatuses{{
        #{status_rows}
        }};

        } // namespace

        StatusMetadata metadata(StatusId status) noexcept {
          switch (status) {
        #{status_cases}
          }
          return StatusMetadata{status, "", "", "", 0};
        }

        std::span<const StatusMetadata> all_statuses() noexcept {
          return {kStatuses.data(), kStatuses.size()};
        }

        } // namespace fl::status::generated_content
      CPP
    end

    def monster_content_hpp
      <<~CPP
        // Generated by scripts/fairlanes_content_codegen.rb. Do not edit by hand.
        #pragma once

        #include <span>
        #include <string_view>

        #include "fl/primitives/cycle.hpp"
        #include "fl/primitives/damage.hpp"
        #include "fl/monsters/monster_kind.hpp"
        #include "fl/monsters/decision_rule.hpp"
        #include "fl/skills/skill.hpp"

        namespace fl::monster::generated_content {

        struct MonsterStats {
          std::string_view display_name;
          std::string_view description;
          int hp;
          int level;
          fl::primitives::Cycle cycle;
          fl::primitives::Resistances resistances;
        };

        MonsterStats stats(MonsterKind kind) noexcept;
        std::span<const DecisionRule> decision_rules(MonsterKind kind) noexcept;
        std::span<const fl::skills::SkillId> known_skills(MonsterKind kind) noexcept;
        std::span<const MonsterKind> common_woodland() noexcept;
        std::span<const MonsterKind> rare_woodland() noexcept;
        std::span<const MonsterKind> raid_bosses() noexcept;

        } // namespace fl::monster::generated_content
      CPP
    end

    def monster_kinds_hpp
      rows = monsters.map { |monster| "  #{monster.cpp_id}," }.join("\n")

      <<~CPP
        // Generated by scripts/fairlanes_content_codegen.rb. Do not edit by hand.
        #pragma once

        namespace fl::monster {

        enum class MonsterKind {
        #{rows}
        };

        } // namespace fl::monster
      CPP
    end

    def monster_content_cpp
      includes = monsters.map do |monster|
        "#include \"fl/generated/monsters/#{monster.id}.hpp\""
      end.join("\n")

      stat_cases = monsters.map do |monster|
        <<~CPP
          case MonsterKind::#{monster.cpp_id}:
            return detail::#{monster.cpp_id}Stats();
        CPP
      end.join

      known_skill_cases = monsters.map do |monster|
        <<~CPP
          case MonsterKind::#{monster.cpp_id}:
            return detail::#{monster.cpp_id}KnownSkills();
        CPP
      end.join

      common_rows = monsters.select { |monster| monster.pool == :common_woodland }.map do |monster|
        "    #{monster_cpp(monster)},"
      end.join("\n")

      rare_rows = monsters.select { |monster| monster.pool == :rare_woodland }.map do |monster|
        "    #{monster_cpp(monster)},"
      end.join("\n")

      raid_rows = monsters.select { |monster| monster.pool == :raid }.map do |monster|
        "    #{monster_cpp(monster)},"
      end.join("\n")

      rule_data = monsters.map do |monster|
        conditions = monster.decision_rules.each_with_index.map do |rule, index|
          rows = rule.fetch(:conditions, []).map do |condition|
            "RuleCondition{RuleSubject::#{camel(condition.fetch(:subject, :target))}, RulePredicate::#{camel(condition.fetch(:predicate))}, RuleStatus::#{camel(condition.fetch(:status, :none))}, #{condition.fetch(:percent, 0)}}"
          end
          "constexpr std::array<RuleCondition, #{rows.size}> k#{monster.cpp_id}Conditions#{index}{{#{rows.join(', ')}}};"
        end.join("\n")
        rows = monster.decision_rules.each_with_index.map do |rule, index|
          "DecisionRule{#{skill_symbol_cpp(rule.fetch(:skill))}, RuleTarget::#{camel(rule.fetch(:target, :enemy))}, #{rule.fetch(:chance_percent, 100)}, k#{monster.cpp_id}Conditions#{index}, #{rule.fetch(:once_per_encounter, false) ? 'true' : 'false'}}"
        end
        conditions + "\nconstexpr std::array<DecisionRule, #{rows.size}> k#{monster.cpp_id}Rules{{#{rows.join(', ')}}};"
      end.join("\n")
      rule_cases = monsters.map { |monster| "case MonsterKind::#{monster.cpp_id}: return k#{monster.cpp_id}Rules;" }.join("\n")

      <<~CPP
        // Generated by scripts/fairlanes_content_codegen.rb. Do not edit by hand.
        #include "fl/generated/monster_content.hpp"

        #{includes}

        #include <array>
        #include <span>
        #include <string_view>

        #include "fl/primitives/cycle.hpp"
        #include "fl/monsters/monster_kind.hpp"
        #include "fl/skills/skill.hpp"

        namespace fl::monster::generated_content {
        namespace {

        #{rule_data}

        constexpr std::array<MonsterKind, #{monsters.count { |monster| monster.pool == :common_woodland }}> kCommonWoodland{{
        #{common_rows}
        }};

        constexpr std::array<MonsterKind, #{monsters.count { |monster| monster.pool == :raid }}> kRaidBosses{{
        #{raid_rows}
        }};

        constexpr std::array<MonsterKind, #{monsters.count { |monster| monster.pool == :rare_woodland }}> kRareWoodland{{
        #{rare_rows}
        }};

        } // namespace

        std::span<const DecisionRule> decision_rules(MonsterKind kind) noexcept {
          switch (kind) {
        #{rule_cases}
          }
          return {};
        }

        MonsterStats stats(MonsterKind kind) noexcept {
          switch (kind) {
        #{stat_cases}
          }
          return MonsterStats{};
        }

        std::span<const fl::skills::SkillId> known_skills(MonsterKind kind) noexcept {
          switch (kind) {
        #{known_skill_cases}
          }
          return {};
        }

        std::span<const MonsterKind> common_woodland() noexcept {
          return {kCommonWoodland.data(), kCommonWoodland.size()};
        }

        std::span<const MonsterKind> raid_bosses() noexcept {
          return kRaidBosses;
        }

        std::span<const MonsterKind> rare_woodland() noexcept {
          return {kRareWoodland.data(), kRareWoodland.size()};
        }

        } // namespace fl::monster::generated_content
      CPP
    end

    def monster_entry_hpp(monster)
      <<~CPP
        // Generated by scripts/fairlanes_content_codegen.rb. Do not edit by hand.
        #pragma once

        #include <span>

        #include "fl/generated/monster_content.hpp"
        #include "fl/skills/skill.hpp"

        namespace fl::monster::generated_content::detail {

        const MonsterStats &#{monster.cpp_id}Stats() noexcept;
        std::span<const fl::skills::SkillId> #{monster.cpp_id}KnownSkills() noexcept;

        } // namespace fl::monster::generated_content::detail
      CPP
    end

    def monster_entry_cpp(monster)
      known_skills = monster.known_skills.map { |skill_id| skill_symbol_cpp(skill_id) }.join(", ")

      <<~CPP
        // Generated by scripts/fairlanes_content_codegen.rb. Do not edit by hand.
        #include "fl/generated/monsters/#{monster.id}.hpp"

        #include <array>
        #include <span>

        #include "fl/skills/skill.hpp"

        namespace fl::monster::generated_content::detail {
        namespace {

        constexpr std::array k#{monster.cpp_id}KnownSkillsData{#{known_skills}};
        constexpr MonsterStats k#{monster.cpp_id}StatsData{
            #{cpp_string(monster.display)},
            #{cpp_string(monster.description)},
            #{monster.hp},
            #{monster.level || 0},
            fl::primitives::Cycle::#{camel(monster.cycle)},
            #{resistance_cpp(monster.resistances)},
        };

        } // namespace

        const MonsterStats &#{monster.cpp_id}Stats() noexcept {
          return k#{monster.cpp_id}StatsData;
        }

        std::span<const fl::skills::SkillId> #{monster.cpp_id}KnownSkills() noexcept {
          return {k#{monster.cpp_id}KnownSkillsData.data(),
                  k#{monster.cpp_id}KnownSkillsData.size()};
        }

        } // namespace fl::monster::generated_content::detail
      CPP
    end

    def monster_registration_hpp
      <<~CPP
        // Generated by scripts/fairlanes_content_codegen.rb. Do not edit by hand.
        #pragma once

        namespace fl::monster::generated_content {

        void register_monster_archetypes();

        } // namespace fl::monster::generated_content
      CPP
    end

    def monster_registration_cpp
      registration_rows = monsters.map do |monster|
        <<~CPP
            register_monster(MonsterKind::#{monster.cpp_id}, [](EntityBuilder &builder) {
              apply_monster_stats(builder, MonsterKind::#{monster.cpp_id});
            });
        CPP
      end.join("\n")

      <<~CPP
        // Generated by scripts/fairlanes_content_codegen.rb. Do not edit by hand.
        #include "fl/generated/monster_registration.hpp"

        #include "fl/monsters/apply_monster_stats.hpp"
        #include "fl/primitives/cycle.hpp"
        #include "fl/monsters/monster_kind.hpp"
        #include "fl/monsters/monster_registry.hpp"
        #include "fl/primitives/entity_builder.hpp"

        namespace fl::monster::generated_content {

        void register_monster_archetypes() {
        #{registration_rows}
        }

        } // namespace fl::monster::generated_content
      CPP
    end

    def balance_report
      skill_metadata_by_id = skills.to_h { |skill| [skill.id, skill] }
      monsters_by_skill = monsters_by_skill_map

      lines = [
        "# Fairlanes Generated Content Balance Report",
        "",
        "Generated by `scripts/fairlanes_content_codegen.rb`.",
        "",
        "Generated artifacts currently provide runtime skill metadata C++, status metadata C++, monster topology metadata C++, monster registration glue, and review reports.",
        "",
        "## Summary",
        "",
        "| Projection | Count | Source of truth |",
        "| --- | ---: | --- |",
        "| Skill definitions | #{skills.size} | Ruby DSL generated C++ `SkillDefinition` table |",
        "| Random combat skills | #{random_combat_skills.size} | Ruby DSL generated C++ random combat table |",
        "| Status declarations | #{statuses.size} | Ruby DSL generated C++ status metadata table |",
        "| Monster declarations | #{monsters.size} | Ruby DSL generated C++ monster topology table |",
        "| Common woodland monsters | #{monsters.count { |monster| monster.pool == :common_woodland }} | Ruby DSL generated C++ encounter pool |",
        "| Rare woodland monsters | #{monsters.count { |monster| monster.pool == :rare_woodland }} | Ruby DSL generated C++ encounter pool |",
        "",
        "## Monsters by Cycle",
        "",
        "Initial themed assignments for content planning; cycle metadata does not filter encounter pools.",
        "",
        "| Cycle | Monsters |",
        "| --- | ---: |",
        *CYCLES.map { |cycle| "| #{camel(cycle)} | #{monsters.count { |monster| monster.cycle == cycle }} |" },
        "",
        "## Skill Metadata",
        "",
        "| Skill | C++ ID | Learn chance | Flee success | Random combat | Execution | Visual | Monsters | Shape |",
        "| --- | --- | ---: | ---: | --- | --- | --- | --- | --- |"
      ]

      skills.each do |skill|
        lines << [
          skill.display,
          skill.cpp_id,
          skill.learn_chance_percent,
          skill.flee_success_percent,
          skill.random_combat ? "yes" : "no",
          skill.execution,
          skill.visual ? visuals.fetch(skill.visual) : "",
          monsters_by_skill[skill.id].map(&:display).join(", "),
          skill.declarative_shape
        ].join(" | ").then { |row| "| #{row} |" }
      end

      lines.concat [
        "",
        "## Status Metadata",
        "",
        "| Status | C++ ID | Component | Debug name | Palette index |",
        "| --- | --- | --- | --- | ---: |"
      ]

      statuses.each do |status|
        lines << [
          status.display,
          status.cpp_id,
          status.component,
          status.debug_name,
          status.palette_index
        ].join(" | ").then { |row| "| #{row} |" }
      end

      lines.concat [
        "",
        "## Monster Descriptions",
        "",
        *monsters.map { |monster| "- **#{monster.display}:** #{monster.description}" },
        "",
        "## Monster Topology",
        "",
        "| Monster | C++ ID | Cycle | HP | Level | Known skills | Pool |",
        "| --- | --- | --- | ---: | --- | --- | --- |"
      ]

      monsters.each do |monster|
        lines << [
          monster.display,
          monster.cpp_id,
          camel(monster.cycle),
          monster.hp,
          monster.level || "",
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

      skills.select(&:visual).each do |skill|
        lines << [
          skill.display,
          monsters_by_skill[skill.id].map(&:display).join(", "),
          visuals.fetch(skill.visual),
          skill.tags.join(", ")
        ].join(" | ").then { |row| "| #{row} |" }
      end

      "#{lines.join("\n")}\n"
    end

    def effect_gallery
      monsters_by_skill = monsters_by_skill_map
      visual_skills = skills.select(&:visual).group_by(&:visual)

      lines = [
        "# Fairlanes Generated Effect Gallery Metadata",
        "",
        "Generated by `scripts/fairlanes_content_codegen.rb`.",
        "",
        "This is review metadata for the handwritten decal renderer and generated skill metadata.",
        "",
        "| Visual | Skills | Monsters | Tags | Shape |",
        "| --- | --- | --- | --- | --- |"
      ]

      visuals.each do |visual_id, cpp_id|
        visual_skill_rows = visual_skills.fetch(visual_id, [])
        next if visual_skill_rows.empty?

        visual_monsters = visual_skill_rows.flat_map { |skill| monsters_by_skill[skill.id] }.uniq
        tags = visual_skill_rows.flat_map(&:tags).uniq
        shapes = visual_skill_rows.map(&:declarative_shape).uniq

        lines << [
          cpp_id,
          visual_skill_rows.map(&:display).join(", "),
          visual_monsters.map(&:display).join(", "),
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

      skills.select(&:visual).each do |skill|
        lines << [
          skill.display,
          visuals.fetch(skill.visual),
          skill.execution,
          monsters_by_skill[skill.id].map(&:display).join(", "),
          skill.tags.join(", ")
        ].join(" | ").then { |row| "| #{row} |" }
      end

      "#{lines.join("\n")}\n"
    end

    def manifest
      {
        "schema_version" => 1,
        "source" => "scripts/fairlanes_content_declarations.rb",
        "runtime_authority" => "Ruby DSL generated C++ skill, status, monster, gear, resolved tower floor, regression, objective, and dialogue data; handwritten C++ gameplay and story effects",
        "visuals" => visuals.map do |id, cpp_id|
          { "id" => id.to_s, "cpp_id" => cpp_id }
        end,
        "statuses" => statuses.map do |status|
          {
            "id" => status.id.to_s,
            "cpp_id" => status.cpp_id,
            "display" => status.display,
            "debug_name" => status.debug_name,
            "component" => status.component,
            "palette_index" => status.palette_index
          }
        end,
        "skills" => skills.map do |skill|
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
        "random_combat_skills" => random_combat_skills.map(&:to_s),
        "monsters" => monsters.map do |monster|
          {
            "id" => monster.id.to_s,
            "cpp_id" => monster.cpp_id,
            "display" => monster.display,
            "description" => monster.description,
            "cycle" => monster.cycle.to_s,
            "hp" => monster.hp,
            "resistances" => monster.resistances,
            "level" => monster.level,
            "known_skills" => monster.known_skills.map(&:to_s),
            "pool" => monster.pool.to_s
          }
        end,
        "gear" => gears.map do |gear|
          { "id" => gear.id.to_s, "display" => gear.display,
            "slot" => gear.slot.to_s, "armor_kind" => gear.armor_kind.to_s,
            "tier" => gear.tier.to_s, "weight" => gear.weight,
            "modifiers" => gear.modifiers }
        end,
        "encounter_pools" => encounter_pools.map do |pool|
          { "id" => pool.id.to_s, "lineups" => pool.lineups.map { |lineup| lineup.map(&:to_s) } }
        end,
        "tower_patterns" => tower_patterns.map do |pattern|
          { "id" => pattern.id.to_s, "monsters" => pattern.monsters.map(&:to_s) }
        end,
        "biomes" => biomes.map do |biome|
          { "id" => biome.id.to_s, "display" => biome.display, "floors" => biome.floors,
            "pattern" => biome.pattern&.to_s,
            "bands" => biome.bands.map { |band| { "from" => band.from, "to" => band.to, "pool" => band.pool.to_s } },
            "overrides" => biome.overrides.map do |entry|
              { "floor" => entry.floor, "lineup" => entry.lineup.map(&:to_s),
                "level_percent" => entry.level_percent, "hp_percent" => entry.hp_percent,
                "attack_percent" => entry.attack_percent,
                "cover" => entry.cover&.transform_values { |value| value.is_a?(Symbol) ? value.to_s : value } }
            end,
            "resolved_floors" => resolved_floors(biome).map do |floor|
              { "id" => floor[:id], "floor" => floor[:floor], "source" => floor[:source],
                "lineup" => floor[:lineup].map(&:to_s),
                "level_percent" => floor[:level_percent], "hp_percent" => floor[:hp_percent],
                "attack_percent" => floor[:attack_percent] }
            end }
        end,
        "biome_plans" => biome_plans.map do |plan|
          { "id" => plan.id.to_s, "cycle" => plan.cycle.to_s,
            "order" => plan.order, "monsters" => plan.monsters.map(&:to_s) }
        end,
        "recruitment_opportunities" => recruitment_opportunities.map do |entry|
          { "id" => entry.id.to_s, "cycle" => entry.cycle.to_s,
            "biome" => entry.biome&.to_s, "floor" => entry.floor,
            "destination_party" => entry.destination_party,
            "unlocked_by_party_completion" => entry.unlocked_by_party_completion }
        end,
        "story_events" => story_events.map { |event| { "id" => event.id.to_s, "handled" => event.handled } },
        "objectives" => objectives.map do |objective|
          { "id" => objective.id.to_s, "title" => objective.title,
            "description" => objective.description,
            "revealed_by" => objective.revealed_by.to_s,
            "completed_by" => objective.completed_by.to_s }
        end,
        "dialogues" => dialogues.map do |dialogue|
          { "id" => dialogue.id.to_s, "location" => dialogue.location.to_s,
            "npc" => dialogue.npc, "start" => dialogue.start.to_s,
            "nodes" => dialogue.nodes.map do |node|
              { "id" => node[:id].to_s, "speaker" => node[:speaker], "text" => node[:text],
                "choices" => node[:choices].map do |choice|
                  { "id" => choice[:id].to_s, "label" => choice[:label],
                    "event" => choice[:event]&.to_s, "next" => choice[:next].to_s }
                end }
            end }
        end,
        "autoplay_routes" => autoplay_routes.map do |route|
          { "id" => route.id.to_s,
            "steps" => route.steps.map do |step|
              { "wait_for" => step[:wait_for].to_s, "action" => step[:action].to_s,
                "target" => step[:target]&.to_s, "choice" => step[:choice]&.to_s }
            end }
        end,
        "regressions" => regressions.map do |scenario|
          { "id" => scenario.id.to_s,
            "encounter" => { "biome" => scenario.encounter[:biome].to_s,
                             "floor" => scenario.encounter[:floor] },
            "encounters" => scenario.encounters.map do |encounter|
              { "biome" => encounter[:biome].to_s, "floor" => encounter[:floor] }
            end,
            "party" => scenario.party.map { |member| member.transform_values { |value| value.is_a?(Symbol) ? value.to_s : value } },
            "seeds" => scenario.seeds, "beat_cap" => scenario.beat_cap,
            "recording_version" => scenario.recording_version,
            "variants" => scenario.variants.map do |variant|
              { "id" => variant[:id].to_s, "equipped_item" => variant[:equipped_item]&.to_s,
                "skill_overrides" => variant.fetch(:skill_overrides, {}).transform_keys(&:to_s).transform_values(&:to_s),
                "observe_members" => variant.fetch(:observe_members, []),
                "next_skill" => variant[:next_skill]&.to_s,
                "assertions" => variant.fetch(:assertions, []).map(&:to_s),
                "expected_wins" => variant[:expected_wins] && [variant[:expected_wins].begin, variant[:expected_wins].end] }
            end }
        end
      }.then { |data| "#{JSON.pretty_generate(data)}\n" }
    end

    def manifest_schema
      string_type = { "type" => "string", "minLength" => 1 }
      percent_type = { "type" => "integer", "minimum" => 0, "maximum" => 100 }

      schema = {
        "$schema" => "https://json-schema.org/draft/2020-12/schema",
        "$id" => "https://fairlanes.local/generated/content_manifest.schema.json",
        "title" => "Fairlanes Generated Content Manifest",
        "type" => "object",
        "additionalProperties" => false,
        "required" => %w[schema_version source runtime_authority visuals statuses skills random_combat_skills monsters gear encounter_pools tower_patterns biomes biome_plans recruitment_opportunities story_events objectives dialogues autoplay_routes regressions],
        "properties" => {
          "schema_version" => { "const" => 1 },
          "source" => string_type,
          "runtime_authority" => string_type,
          "visuals" => array_schema(%w[id cpp_id], { "id" => string_type, "cpp_id" => string_type }),
          "statuses" => array_schema(
            %w[id cpp_id display debug_name component palette_index],
            {
              "id" => string_type,
              "cpp_id" => string_type,
              "display" => string_type,
              "debug_name" => string_type,
              "component" => string_type,
              "palette_index" => { "type" => "integer", "minimum" => 0, "maximum" => 31 }
            }
          ),
          "skills" => array_schema(
            %w[id cpp_id display learn_chance_percent flee_success_percent random_combat execution visual tags declarative_shape],
            {
              "id" => string_type,
              "cpp_id" => string_type,
              "display" => string_type,
              "learn_chance_percent" => percent_type,
              "flee_success_percent" => percent_type,
              "random_combat" => { "type" => "boolean" },
              "execution" => string_type,
              "visual" => { "type" => %w[string null] },
              "tags" => { "type" => "array", "minItems" => 1, "items" => string_type },
              "declarative_shape" => string_type
            }
          ),
          "random_combat_skills" => { "type" => "array", "items" => string_type },
          "monsters" => array_schema(
            %w[id cpp_id display description cycle hp level resistances known_skills pool],
            {
              "id" => string_type,
              "cpp_id" => string_type,
              "display" => string_type,
              "description" => { "type" => "string", "minLength" => 1 },
              "cycle" => { "enum" => CYCLES.map(&:to_s) },
              "hp" => { "type" => "integer", "minimum" => 1 },
              "resistances" => resistance_schema,
              "level" => { "type" => %w[integer null], "minimum" => 1 },
              "known_skills" => { "type" => "array", "minItems" => 1, "items" => string_type },
              "pool" => string_type
            }
          ),
          "gear" => array_schema(%w[id display slot armor_kind tier weight modifiers], {
            "id" => string_type, "display" => string_type, "slot" => string_type,
            "armor_kind" => string_type, "tier" => string_type,
            "weight" => { "type" => "integer", "minimum" => 0 },
            "modifiers" => { "type" => "object", "additionalProperties" => false,
                             "properties" => { "resistances" => resistance_schema,
                                               "attack" => { "type" => "integer", "minimum" => 0 },
                                               "armor" => { "type" => "integer", "minimum" => 0 },
                                               "max_hp" => { "type" => "integer", "minimum" => 0 } } }
          }),
          "encounter_pools" => array_schema(%w[id lineups], {
            "id" => string_type,
            "lineups" => { "type" => "array", "minItems" => 1,
                           "items" => { "type" => "array", "minItems" => 1, "maxItems" => 5,
                                        "items" => string_type } }
          }),
          "tower_patterns" => array_schema(%w[id monsters], {
            "id" => string_type,
            "monsters" => { "type" => "array", "minItems" => 1, "maxItems" => 5,
                             "items" => string_type }
          }),
          "biomes" => array_schema(%w[id display floors pattern bands overrides resolved_floors], {
            "id" => string_type, "display" => string_type, "floors" => { "const" => 100 },
            "pattern" => { "type" => %w[string null] },
            "bands" => { "type" => "array", "items" => { "type" => "object" } },
            "overrides" => { "type" => "array", "items" => { "type" => "object" } },
            "resolved_floors" => { "type" => "array", "minItems" => 100, "maxItems" => 100,
                                    "items" => { "type" => "object" } }
          }),
          "biome_plans" => array_schema(%w[id cycle order monsters], {
            "id" => string_type, "cycle" => { "enum" => CYCLES.map(&:to_s) },
            "order" => { "type" => "integer", "minimum" => 1, "maximum" => 5 },
            "monsters" => { "type" => "array", "items" => string_type }
          }),
          "recruitment_opportunities" => array_schema(
            %w[id cycle biome floor destination_party unlocked_by_party_completion], {
              "id" => string_type, "cycle" => { "enum" => CYCLES.map(&:to_s) },
              "biome" => { "type" => %w[string null] },
              "floor" => { "type" => %w[integer null] },
              "destination_party" => { "type" => "integer", "minimum" => 1, "maximum" => 5 },
              "unlocked_by_party_completion" => { "type" => %w[integer null] }
            }
          ),
          "story_events" => array_schema(%w[id handled], {
            "id" => string_type, "handled" => { "type" => "boolean" }
          }),
          "objectives" => array_schema(%w[id title description revealed_by completed_by], {
            "id" => string_type, "title" => string_type,
            "description" => string_type, "revealed_by" => string_type,
            "completed_by" => string_type
          }),
          "dialogues" => array_schema(%w[id location npc start nodes], {
            "id" => string_type, "location" => string_type,
            "npc" => string_type, "start" => string_type,
            "nodes" => { "type" => "array", "items" => { "type" => "object" } }
          }),
          "autoplay_routes" => array_schema(%w[id steps], {
            "id" => string_type,
            "steps" => { "type" => "array", "minItems" => 1,
                         "items" => { "type" => "object" } }
          }),
          "regressions" => array_schema(%w[id encounter encounters party seeds beat_cap recording_version variants], {
            "id" => string_type, "encounter" => { "type" => "object" },
            "encounters" => { "type" => "array", "minItems" => 1,
                                "items" => { "type" => "object" } },
            "party" => { "type" => "array", "items" => { "type" => "object" } },
            "seeds" => { "type" => "array", "items" => { "type" => "integer", "minimum" => 0 } },
            "beat_cap" => { "type" => "integer", "minimum" => 1 },
            "recording_version" => { "type" => "integer", "enum" => [1, 2] },
            "variants" => { "type" => "array", "items" => { "type" => "object" } }
          })
        }
      }

      "#{JSON.pretty_generate(schema)}\n"
    end

    private

    attr_reader :declarations

    def resolved_floors(biome)
      FloorResolver.new(declarations).resolve(biome)
    end

    def monster_by_id(id)
      monsters.find { |monster| monster.id == id }
    end

    def camel(value)
      value.to_s.split("_").map(&:capitalize).join
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
      skill = skills.find { |candidate| candidate.id == skill_id }
      raise "unknown skill #{skill_id}" unless skill

      skill_id_cpp(skill)
    end

    def monster_cpp(monster)
      "fl::monster::MonsterKind::#{monster.cpp_id}"
    end

    def visual_cpp(skill)
      "fl::widgets::effects::DecalAnimationKind::#{visuals.fetch(skill.visual)}"
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
      { common_woodland: "CommonWoodland", rare_woodland: "RareWoodland",
        authored_only: "AuthoredOnly", raid: "Raid" }.fetch(pool)
    end

    def monsters_by_skill_map
      Hash.new { |hash, key| hash[key] = [] }.tap do |result|
        monsters.each do |monster|
          monster.known_skills.each { |skill| result[skill] << monster }
        end
      end
    end

    def array_schema(required, properties)
      {
        "type" => "array",
        "items" => {
          "type" => "object",
          "additionalProperties" => false,
          "required" => required,
          "properties" => properties
        }
      }
    end
  end
end
