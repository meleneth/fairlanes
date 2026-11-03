# frozen_string_literal: true

require "set"

module FairlanesContent
  class ValidationError < StandardError
    attr_reader :errors

    def initialize(errors)
      @errors = errors
      super(errors.join("\n"))
    end
  end

  class Validator
    VALID_POOLS = Set[:common_woodland, :rare_woodland, :raid].freeze
    GEAR_SLOTS = Set[:mainhand, :offhand, :chest, :helm, :pants, :boots,
                     :gloves, :sleeves, :belt, :cape, :necklace, :ring_1,
                     :ring_2, :knife].freeze
    ARMOR_KINDS = Set[:none, :cloth, :leather, :plate].freeze
    GEAR_TIERS = Set[:worn, :plain, :sturdy, :fine, :excellent,
                     :masterwork, :mythic].freeze
    RESISTANCE_CHANNELS = Set[:physical, :magical, :fire, :ice, :lightning].freeze
    STORY_LOCATIONS = Set[:pub, :village].freeze
    VALID_EXECUTIONS = Set[
      :thump_like, :eviscerate, :poison, :cold_snap, :flame_strike,
      :flame_wave, :decal_strike, :damage_strike, :group_damage,
      :single_heal, :group_heal, :placeholder_effect, :flee, :observe, :status_detonation
    ].freeze
    VALID_TAGS = Set[
      :physical, :blunt, :piercing, :slashing, :bleed, :poison, :disease,
      :acid, :fire, :cold, :lightning, :earth, :gravity, :sonic, :healing,
      :holy, :control, :area, :projectile, :melee, :observe, :utility,
      :escape, :rot, :water, :air, :light, :dark, :arcane, :ballistic,
      :laser, :plasma, :data, :ranged, :spell, :breath, :song, :trap, :self,
      :ally, :enemy, :group, :random_enemy, :all_enemies, :all_allies, :field,
      :summon, :damage, :heal, :cleanse, :buff, :debuff, :poison_status,
      :burn_status, :bleed_status, :slow_status, :stun_status, :blind_status,
      :silence_status, :fear_status, :regen_status, :shield_status,
      :taunt_status, :vulnerable_status, :haste_status, :reflect_status,
      :dispel, :drain, :execute, :beast, :vermin, :plant, :fungal,
      :amphibian, :reptile, :aquatic, :avian, :undead, :construct, :machine,
      :celestial, :void, :martial, :natural, :industrial, :cybernetic
    ].freeze
    VALID_VISUAL_CPP = Set[
      "FlameWave", "Shock", "RocksFall", "PoisonCloud", "HolyNova",
      "BloodBloom", "FrostCrack", "VoidRipple", "HitpointNumber",
      "Starfire", "Observe"
    ].freeze
    VALID_DECLARATIVE_SHAPES = Set[
      :handwritten_behavior,
      :decal_strike,
      :generated_runtime_behavior,
      :placeholder_effect
    ].freeze

    def initialize(declarations)
      @declarations = declarations
    end

    def validate!
      errors = validate
      raise ValidationError, errors unless errors.empty?

      true
    end

    def validate
      errors = []
      validate_duplicate_ids(errors)
      validate_visuals(errors)
      validate_statuses(errors)
      validate_skills(errors)
      validate_random_combat_skills(errors)
      validate_monsters(errors)
      validate_decision_rules(errors)
      validate_gear(errors)
      validate_floor_content(errors)
      validate_regressions(errors)
      validate_biome_plans(errors)
      validate_story(errors)
      errors
    end

    private

    attr_reader :declarations

    def validate_story(errors)
      events = declarations.story_events.map(&:id)
      errors << "duplicate story event ids" unless events.uniq == events
      declarations.story_events.each do |event|
        errors << "story event #{event.id} has invalid id" unless stable_id?(event.id)
        errors << "story event #{event.id} has invalid handled flag" unless [true, false].include?(event.handled)
      end
      objective_ids = declarations.objectives.map(&:id)
      errors << "duplicate objective ids" unless objective_ids.uniq == objective_ids
      declarations.objectives.each do |objective|
        errors << "objective #{objective.id} has invalid id" unless stable_id?(objective.id)
        errors << "objective #{objective.id} has no title or description" if objective.title.to_s.empty? || objective.description.to_s.empty?
        %i[revealed_by completed_by].each do |field|
          errors << "objective #{objective.id} references unknown #{field}" unless events.include?(objective.public_send(field))
        end
      end
      dialogue_ids = declarations.dialogues.map(&:id)
      errors << "duplicate dialogue ids" unless dialogue_ids.uniq == dialogue_ids
      declarations.dialogues.each do |dialogue|
        prefix = "dialogue #{dialogue.id}"
        errors << "#{prefix} has invalid id" unless stable_id?(dialogue.id)
        errors << "#{prefix} has invalid location or npc" unless STORY_LOCATIONS.include?(dialogue.location) && !dialogue.npc.to_s.empty?
        unless dialogue.nodes.is_a?(Array) && !dialogue.nodes.empty?
          errors << "#{prefix} has no nodes"
          next
        end
        nodes = dialogue.nodes.map { |node| node[:id] }
        errors << "#{prefix} has duplicate node ids" unless nodes.uniq == nodes
        errors << "#{prefix} has unknown start node" unless nodes.include?(dialogue.start)
        dialogue.nodes.each do |node|
          errors << "#{prefix} node #{node[:id]} has unsupported fields" unless (node.keys - %i[id speaker text choices]).empty?
          errors << "#{prefix} node #{node[:id]} has invalid id or text" unless stable_id?(node[:id]) && !node[:text].to_s.empty? && !node[:speaker].to_s.empty?
          unless node[:choices].is_a?(Array)
            errors << "#{prefix} node #{node[:id]} has invalid choices"
            next
          end
          ids = node[:choices].map { |choice| choice[:id] }
          errors << "#{prefix} node #{node[:id]} has duplicate choice ids" unless ids.uniq == ids
          node[:choices].each do |choice|
            errors << "#{prefix} choice #{choice[:id]} has unsupported fields" unless (choice.keys - %i[id label event next]).empty?
            errors << "#{prefix} has invalid choice" unless stable_id?(choice[:id]) && !choice[:label].to_s.empty?
            errors << "#{prefix} choice #{choice[:id]} has unknown next node" unless choice[:next] == :exit || nodes.include?(choice[:next])
            errors << "#{prefix} choice #{choice[:id]} emits unknown event" if choice[:event] && !events.include?(choice[:event])
          end
        end
      end
      route_ids = declarations.autoplay_routes.map(&:id)
      errors << "duplicate autoplay route ids" unless route_ids.uniq == route_ids
      declarations.autoplay_routes.each do |route|
        errors << "autoplay route #{route.id} has invalid id" unless stable_id?(route.id)
        unless route.steps.is_a?(Array) && !route.steps.empty?
          errors << "autoplay route #{route.id} has no steps"
          next
        end
        route.steps.each_with_index do |step, index|
          prefix = "autoplay route #{route.id} step #{index}"
          errors << "#{prefix} has unsupported fields" unless (step.keys - %i[wait_for action target choice]).empty?
          errors << "#{prefix} waits for unknown event" unless events.include?(step[:wait_for])
          case step[:action]
          when :choose_dialogue
            dialogue = declarations.dialogues.find { |entry| entry.id == step[:target] }
            errors << "#{prefix} has unknown dialogue" unless dialogue
            if dialogue && !dialogue.nodes.any? { |node| node[:choices].any? { |choice| choice[:id] == step[:choice] && choice[:event] } }
              errors << "#{prefix} has unknown emitting choice"
            end
          when :buy_gear
            errors << "#{prefix} has unknown gear" unless declarations.gears.any? { |gear| gear.id == step[:target] }
          when :return_to_town
            errors << "#{prefix} must not name a target" if step[:target] || step[:choice]
          else
            errors << "#{prefix} has unknown action"
          end
        end
      end
    end

    def validate_duplicate_ids(errors)
      skill_ids = declarations.skills.map(&:id)
      monster_ids = declarations.monsters.map(&:id)
      skill_cpp_ids = declarations.skills.map(&:cpp_id)
      monster_cpp_ids = declarations.monsters.map(&:cpp_id)
      status_ids = declarations.statuses.map(&:id)
      status_cpp_ids = declarations.statuses.map(&:cpp_id)
      errors << "duplicate skill ids" unless skill_ids.uniq.size == skill_ids.size
      errors << "duplicate monster ids" unless monster_ids.uniq.size == monster_ids.size
      errors << "duplicate status ids" unless status_ids.uniq.size == status_ids.size
      errors << "duplicate skill C++ ids" unless skill_cpp_ids.uniq.size == skill_cpp_ids.size
      errors << "duplicate monster C++ ids" unless monster_cpp_ids.uniq.size == monster_cpp_ids.size
      errors << "duplicate status C++ ids" unless status_cpp_ids.uniq.size == status_cpp_ids.size
      errors << "duplicate gear ids" unless declarations.gears.map(&:id).uniq.size == declarations.gears.size
      errors << "duplicate encounter pool ids" unless declarations.encounter_pools.map(&:id).uniq.size == declarations.encounter_pools.size
      errors << "duplicate biome ids" unless declarations.biomes.map(&:id).uniq.size == declarations.biomes.size
    end

    def validate_gear(errors)
      declarations.gears.each do |gear|
        prefix = "gear #{gear.id}"
        errors << "#{prefix} has invalid stable id" unless stable_id?(gear.id)
        errors << "#{prefix} has no display name" if gear.display.to_s.empty?
        errors << "#{prefix} has invalid slot #{gear.slot}" unless GEAR_SLOTS.include?(gear.slot)
        errors << "#{prefix} has invalid armor kind #{gear.armor_kind}" unless ARMOR_KINDS.include?(gear.armor_kind)
        errors << "#{prefix} has invalid tier #{gear.tier}" unless GEAR_TIERS.include?(gear.tier)
        errors << "#{prefix} has invalid weight #{gear.weight}" unless non_negative_integer?(gear.weight)
        if !gear.modifiers.is_a?(Hash) || (gear.modifiers.keys - [:resistances, :attack, :armor, :max_hp]).any?
          errors << "#{prefix} has unsupported stat modifiers"
        else
          validate_resistances(gear.modifiers.fetch(:resistances, {}), "#{prefix} resistances", errors)
          %i[attack armor max_hp].each do |stat|
            value = gear.modifiers.fetch(stat, 0)
            errors << "#{prefix} has invalid #{stat} modifier" unless non_negative_integer?(value) && value <= 1000
          end
        end
      end
    end

    def validate_floor_content(errors)
      monsters = declarations.monsters.map(&:id).to_set
      pools = declarations.encounter_pools.map(&:id).to_set
      declarations.encounter_pools.each do |pool|
        errors << "encounter pool #{pool.id} has invalid stable id" unless stable_id?(pool.id)
        unless pool.lineups.is_a?(Array) && !pool.lineups.empty?
          errors << "encounter pool #{pool.id} has no lineups"
          next
        end
        pool.lineups.each_with_index do |lineup, index|
          validate_lineup(lineup, "encounter pool #{pool.id} lineup #{index}", monsters, errors)
        end
      end
      declarations.biomes.each do |biome|
        prefix = "biome #{biome.id}"
        errors << "#{prefix} has invalid stable id" unless stable_id?(biome.id)
        errors << "#{prefix} has no display name" if biome.display.to_s.empty?
        errors << "#{prefix} must have 100 floors" unless biome.floors == 100
        next unless biome.floors.is_a?(Integer) && biome.floors.positive?

        covered = Array.new(biome.floors + 1, false)
        biome.bands.each do |band|
          unless valid_floor?(band.from, biome.floors) && valid_floor?(band.to, biome.floors) && band.from <= band.to
            errors << "#{prefix} has out-of-range band #{band.from}..#{band.to}"
            next
          end
          errors << "#{prefix} references unknown pool #{band.pool}" unless pools.include?(band.pool)
          (band.from..band.to).each do |number|
            errors << "#{prefix} has overlapping bands at floor #{number}" if covered[number]
            covered[number] = true
          end
        end
        seen_overrides = Set.new
        biome.overrides.each do |override|
          unless valid_floor?(override.floor, biome.floors)
            errors << "#{prefix} has out-of-range override #{override.floor}"
            next
          end
          errors << "#{prefix} has duplicate override #{override.floor}" unless seen_overrides.add?(override.floor)
          covered[override.floor] = true
          validate_lineup(override.lineup, "#{prefix} override #{override.floor}", monsters, errors)
        end
        missing = (1..biome.floors).reject { |number| covered[number] }
        errors << "#{prefix} is missing floor coverage: #{missing.join(', ')}" unless missing.empty?
      end
    end

    def validate_lineup(lineup, prefix, monsters, errors)
      unless lineup.is_a?(Array) && (1..5).cover?(lineup.size)
        errors << "#{prefix} must contain one to five monsters"
        return
      end
      lineup.each do |monster|
        errors << "#{prefix} references unknown monster #{monster}" unless monsters.include?(monster)
      end
    end

    def validate_regressions(errors)
      ids = declarations.regressions.map(&:id)
      errors << "duplicate regression ids" unless ids.uniq.size == ids.size
      scenarios = declarations.regressions
      scenarios.each do |scenario|
        prefix = "regression #{scenario.id}"
        errors << "#{prefix} has invalid stable id" unless stable_id?(scenario.id)
        encounter = scenario.encounter
        unless encounter.is_a?(Hash) && (encounter.keys - %i[biome floor]).empty? &&
               declarations.biomes.any? { |b| b.id == encounter[:biome] && valid_floor?(encounter[:floor], b.floors) }
          errors << "#{prefix} references unknown encounter floor"
        end
        unless scenario.party.is_a?(Array) && (1..5).cover?(scenario.party.size)
          errors << "#{prefix} needs one to five party members"
          next
        end
        scenario.party.each_with_index do |member, index|
          unless member.is_a?(Hash) && positive_integer?(member[:hp]) &&
                 non_negative_integer?(member[:attack]) && non_negative_integer?(member[:armor]) &&
                 member[:skill].is_a?(Symbol) && declarations.skills.any? { |s| s.id == member[:skill] } &&
                 member[:policy] == :single_equipped_skill
            errors << "#{prefix} has invalid party member #{index}"
          end
          validate_resistances(member.fetch(:resistances, {}),
                               "#{prefix} party member #{index} resistances", errors) if member.is_a?(Hash)
        end
        unless scenario.seeds.is_a?(Array) && !scenario.seeds.empty? &&
               scenario.seeds.uniq.size == scenario.seeds.size &&
               scenario.seeds.all? { |seed| non_negative_integer?(seed) }
          errors << "#{prefix} has invalid seeds"
        end
        errors << "#{prefix} has invalid beat cap" unless positive_integer?(scenario.beat_cap)
        variants = scenario.variants
        unless variants.is_a?(Array) && !variants.empty?
          errors << "#{prefix} needs variants"
          next
        end
        variant_ids = variants.map { |variant| variant[:id] }
        errors << "#{prefix} has duplicate variants" unless variant_ids.uniq.size == variant_ids.size
        variants.each do |variant|
          errors << "#{prefix} has invalid variant id" unless stable_id?(variant[:id])
          item = variant[:equipped_item]
          errors << "#{prefix} references unknown gear #{item}" unless declarations.gears.any? { |gear| gear.id == item }
          wins = variant[:expected_wins]
          if wins && (!wins.is_a?(Range) || !non_negative_integer?(wins.begin) ||
                      !non_negative_integer?(wins.end) || wins.end > scenario.seeds.size || wins.begin > wins.end)
            errors << "#{prefix} has invalid expected win range"
          end
        end
      end
    end

    def validate_biome_plans(errors)
      return if declarations.biome_plans.empty?
      ids = declarations.biome_plans.map(&:id)
      errors << "duplicate biome plan ids" unless ids.uniq.size == ids.size
      assigned = []
      CYCLES.each do |cycle|
        plans = declarations.biome_plans.select { |plan| plan.cycle == cycle }
        errors << "cycle #{cycle} must plan five ordered towers" unless plans.map(&:order).sort == (1..5).to_a
      end
      declarations.biome_plans.each do |plan|
        errors << "biome plan #{plan.id} has invalid id" unless stable_id?(plan.id)
        errors << "biome plan #{plan.id} has no monsters" if plan.monsters.empty?
        plan.monsters.each do |id|
          monster = declarations.monsters.find { |entry| entry.id == id }
          errors << "biome plan #{plan.id} references unknown monster #{id}" unless monster
          errors << "biome plan #{plan.id} has wrong-cycle monster #{id}" if monster && monster.cycle != plan.cycle
          assigned << id
        end
      end
      errors << "biome plans duplicate monster assignments" unless assigned.uniq.size == assigned.size
      declarations.biomes.each do |biome|
        errors << "biome #{biome.id} has no tower plan" unless ids.include?(biome.id)
      end
    end

    def stable_id?(id)
      id.is_a?(Symbol) && id.to_s.match?(/\A[a-z][a-z0-9_]*\z/)
    end

    def valid_floor?(number, count)
      number.is_a?(Integer) && (1..count).cover?(number)
    end

    def validate_visuals(errors)
      declarations.visuals.each do |id, cpp|
        errors << "visual #{id} has invalid C++ id #{cpp}" unless VALID_VISUAL_CPP.include?(cpp)
      end
    end

    def validate_statuses(errors)
      declarations.statuses.each do |status|
        errors << "status #{status.id} is missing a C++ id" if status.cpp_id.to_s.empty?
        errors << "status #{status.id} is missing a display name" if status.display.to_s.empty?
        errors << "status #{status.id} is missing a debug name" if status.debug_name.to_s.empty?
        errors << "status #{status.id} is missing a component" if status.component.to_s.empty?
        unless non_negative_integer?(status.palette_index) && status.palette_index <= 31
          errors << "status #{status.id} has invalid palette index #{status.palette_index}"
        end
      end
    end

    def validate_skills(errors)
      declarations.skills.each do |skill|
        if skill.execution == :status_detonation
          unless %i[shield haste burn blind silence slow stun].include?(skill.consumes_status)
            errors << "skill #{skill.id} has invalid consumed status"
          end
          errors << "skill #{skill.id} requires positive effect damage" unless positive_integer?(skill.effect_damage)
        elsif skill.execution == :group_damage
          errors << "skill #{skill.id} has invalid group damage" unless non_negative_integer?(skill.effect_damage)
          errors << "skill #{skill.id} has consumed status without detonation execution" if skill.consumes_status
        elsif skill.consumes_status || skill.effect_damage != 0
          errors << "skill #{skill.id} has detonation metadata without detonation execution"
        end
        errors << "skill #{skill.id} is missing a C++ id" if skill.cpp_id.to_s.empty?
        errors << "skill #{skill.id} is missing a display name" if skill.display.to_s.empty?
        unless percent?(skill.learn_chance_percent)
          errors << "skill #{skill.id} has invalid learn chance #{skill.learn_chance_percent}"
        end
        unless percent?(skill.flee_success_percent)
          errors << "skill #{skill.id} has invalid flee success #{skill.flee_success_percent}"
        end
        if skill.execution != :flee && skill.flee_success_percent != 0
          errors << "skill #{skill.id} has flee success but is not a flee execution"
        end
        unless VALID_EXECUTIONS.include?(skill.execution)
          errors << "skill #{skill.id} has invalid execution #{skill.execution}"
        end
        if skill.visual && !declarations.visuals.key?(skill.visual)
          errors << "skill #{skill.id} has invalid visual #{skill.visual}"
        end
        errors << "skill #{skill.id} has no tags" if skill.tags.empty?
        skill.tags.each do |tag|
          errors << "skill #{skill.id} has invalid tag #{tag}" unless VALID_TAGS.include?(tag)
        end
        errors << "skill #{skill.id} has duplicate tags" unless skill.tags.uniq.size == skill.tags.size
        unless VALID_DECLARATIVE_SHAPES.include?(skill.declarative_shape)
          errors << "skill #{skill.id} has invalid declarative shape #{skill.declarative_shape}"
        end
      end
    end

    def validate_random_combat_skills(errors)
      skills_by_id = declarations.skills.to_h { |skill| [skill.id, skill] }
      random_skill_ids = declarations.skills.select(&:random_combat).map(&:id)

      unless declarations.random_combat_skills.uniq.size == declarations.random_combat_skills.size
        errors << "random combat skill order has duplicate entries"
      end
      declarations.random_combat_skills.each do |skill_id|
        errors << "random combat skill #{skill_id} is unknown" unless skills_by_id.key?(skill_id)
        unless random_skill_ids.include?(skill_id)
          errors << "random combat skill #{skill_id} is not marked random_combat"
        end
      end
      random_skill_ids.each do |skill_id|
        unless declarations.random_combat_skills.include?(skill_id)
          errors << "random combat skill #{skill_id} is missing from order table"
        end
      end
    end

    def validate_decision_rules(errors)
      declarations.monsters.each do |monster|
        unless monster.decision_rules.is_a?(Array)
          errors << "monster #{monster.id} decision_rules must be an array"
          next
        end
        monster.decision_rules.each do |rule|
          prefix = "monster #{monster.id} decision rule"
          unless rule.is_a?(Hash)
            errors << "#{prefix} must be a hash"
            next
          end
          errors << "#{prefix} has unknown fields" unless (rule.keys - %i[skill target chance_percent conditions]).empty?
          errors << "#{prefix} uses an unknown or unequipped skill" unless monster.known_skills.include?(rule[:skill])
          errors << "#{prefix} has invalid target" unless %i[self ally enemy].include?(rule.fetch(:target, :enemy))
          chance = rule.fetch(:chance_percent, 100)
          errors << "#{prefix} has invalid chance" unless chance.is_a?(Integer) && (0..100).cover?(chance)
          conditions = rule.fetch(:conditions, [])
          unless conditions.is_a?(Array)
            errors << "#{prefix} conditions must be an array"
            next
          end
          conditions.each do |condition|
            unless condition.is_a?(Hash)
              errors << "#{prefix} condition must be a hash"
              next
            end
            errors << "#{prefix} condition has unknown fields" unless (condition.keys - %i[subject predicate status percent]).empty?
            errors << "#{prefix} has invalid subject" unless %i[actor target].include?(condition.fetch(:subject, :target))
            case condition[:predicate]
            when :has_status, :missing_status
              unless %i[shield haste burn blind silence slow stun poison dire_bleed freeze].include?(condition[:status])
                errors << "#{prefix} has invalid status"
              end
              errors << "#{prefix} status condition cannot have percent" if condition.key?(:percent)
            when :hp_below, :hp_above
              percent = condition[:percent]
              errors << "#{prefix} has invalid health percent" unless percent.is_a?(Integer) && (0..100).cover?(percent)
              errors << "#{prefix} health condition cannot have status" if condition.key?(:status)
            else
              errors << "#{prefix} has invalid predicate"
            end
          end
        end
      end
    end

    def validate_monsters(errors)
      skills_by_id = declarations.skills.to_h { |skill| [skill.id, skill] }

      declarations.monsters.each do |monster|
        errors << "monster #{monster.id} is missing a C++ id" if monster.cpp_id.to_s.empty?
        errors << "monster #{monster.id} is missing a display name" if monster.display.to_s.empty?
        unless monster.description.is_a?(String) && !monster.description.strip.empty?
          errors << "monster #{monster.id} is missing a description"
        end
        errors << "monster #{monster.id} has invalid cycle #{monster.cycle.inspect}" unless CYCLES.include?(monster.cycle)
        unless positive_integer?(monster.hp)
          errors << "monster #{monster.id} has invalid hp #{monster.hp}"
        end
        unless monster.level.nil? || positive_integer?(monster.level)
          errors << "monster #{monster.id} has invalid level #{monster.level}"
        end
        validate_resistances(monster.resistances, "monster #{monster.id} resistances", errors)
        errors << "monster #{monster.id} has invalid pool #{monster.pool}" unless VALID_POOLS.include?(monster.pool)
        errors << "monster #{monster.id} has no known skills" if monster.known_skills.empty?
        unless monster.known_skills.uniq.size == monster.known_skills.size
          errors << "monster #{monster.id} has duplicate known skills"
        end
        monster.known_skills.each do |skill_id|
          errors << "monster #{monster.id} references unknown skill #{skill_id}" unless skills_by_id.key?(skill_id)
        end
      end
    end

    def percent?(value)
      value.is_a?(Integer) && value >= 0 && value <= 100
    end

    def validate_resistances(values, prefix, errors)
      unless values.is_a?(Hash)
        errors << "#{prefix} must be a hash"
        return
      end
      values.each do |channel, percent|
        errors << "#{prefix} has unknown channel #{channel}" unless RESISTANCE_CHANNELS.include?(channel)
        errors << "#{prefix} #{channel} must be an integer from 0 to 100" unless percent?(percent)
      end
    end

    def positive_integer?(value)
      value.is_a?(Integer) && value.positive?
    end

    def non_negative_integer?(value)
      value.is_a?(Integer) && value >= 0
    end
  end
end
