# frozen_string_literal: true

require "fairlanes_content/models"

module FairlanesContent
  class DeclarationSet
    attr_reader :visuals, :skills, :random_combat_skills, :monsters

    def self.load_file(path)
      new.tap { |set| set.load_file(path) }
    end

    def initialize
      @visuals = {}
      @skills = []
      @random_combat_skills = []
      @monsters = []
    end

    def load_file(path)
      instance_eval(File.read(path), path)
      freeze_records
      self
    end

    def visual(id, cpp: nil)
      visuals[id] = cpp || cpp_name(id)
    end

    def skill(id, cpp_id: nil, display: nil, learn_chance_percent:, random_combat: true,
              flee_success_percent: 0,
              execution:, visual: nil, tags:, declarative_shape:)
      skills << Skill.new(
        id: id,
        cpp_id: cpp_id || cpp_name(id),
        display: display || display_name(id),
        learn_chance_percent: learn_chance_percent,
        random_combat: random_combat,
        flee_success_percent: flee_success_percent,
        execution: execution,
        visual: visual,
        tags: tags,
        declarative_shape: declarative_shape
      )
    end

    def decal_skill(id, visual:, tags:, learn_chance_percent: 5,
                    random_combat: true, cpp_id: nil, display: nil)
      skill(
        id,
        cpp_id: cpp_id,
        display: display,
        learn_chance_percent: learn_chance_percent,
        random_combat: random_combat,
        execution: :decal_strike,
        visual: visual,
        tags: tags,
        declarative_shape: :decal_strike
      )
    end

    def random_combat_skills(*ids)
      return @random_combat_skills if ids.empty?

      @random_combat_skills.replace(ids)
    end

    def monster(id, cpp_id: nil, display: nil, hp:, mp: 0, level: nil,
                known_skills:, pool:)
      monsters << Monster.new(
        id: id,
        cpp_id: cpp_id || cpp_name(id),
        display: display || display_name(id),
        hp: hp,
        mp: mp,
        level: level,
        known_skills: known_skills,
        pool: pool
      )
    end

    private

    def cpp_name(id)
      id.to_s.split("_").map(&:capitalize).join
    end

    def display_name(id)
      id.to_s.split("_").map(&:capitalize).join(" ")
    end

    def freeze_records
      visuals.freeze
      skills.each(&:freeze)
      skills.freeze
      random_combat_skills.freeze
      monsters.each(&:freeze)
      monsters.freeze
    end
  end
end
