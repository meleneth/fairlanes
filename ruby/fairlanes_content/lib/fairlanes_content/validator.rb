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
    VALID_POOLS = Set[:common_woodland, :rare_woodland].freeze
    VALID_EXECUTIONS = Set[
      :thump_like, :eviscerate, :poison, :cold_snap, :flame_strike,
      :flame_wave, :decal_strike, :flee, :observe
    ].freeze
    VALID_TAGS = Set[
      :physical, :blunt, :piercing, :slashing, :bleed, :poison, :disease,
      :acid, :fire, :cold, :lightning, :earth, :gravity, :sonic, :healing,
      :holy, :control, :area, :projectile, :melee, :observe, :utility,
      :escape
    ].freeze
    VALID_VISUAL_CPP = Set[
      "FlameWave", "Shock", "RocksFall", "PoisonCloud", "HolyNova",
      "BloodBloom", "FrostCrack", "VoidRipple", "HitpointNumber"
    ].freeze
    VALID_DECLARATIVE_SHAPES = Set[
      :handwritten_behavior,
      :decal_strike
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
      errors
    end

    private

    attr_reader :declarations

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

    def validate_monsters(errors)
      skills_by_id = declarations.skills.to_h { |skill| [skill.id, skill] }

      declarations.monsters.each do |monster|
        errors << "monster #{monster.id} is missing a C++ id" if monster.cpp_id.to_s.empty?
        errors << "monster #{monster.id} is missing a display name" if monster.display.to_s.empty?
        unless positive_integer?(monster.hp)
          errors << "monster #{monster.id} has invalid hp #{monster.hp}"
        end
        unless non_negative_integer?(monster.mp)
          errors << "monster #{monster.id} has invalid mp #{monster.mp}"
        end
        unless monster.level.nil? || positive_integer?(monster.level)
          errors << "monster #{monster.id} has invalid level #{monster.level}"
        end
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

    def positive_integer?(value)
      value.is_a?(Integer) && value.positive?
    end

    def non_negative_integer?(value)
      value.is_a?(Integer) && value >= 0
    end
  end
end
