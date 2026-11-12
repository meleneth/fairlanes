# frozen_string_literal: true

require "fairlanes_content/models"
require "fairlanes_content/tower_pattern"

module FairlanesContent
  class DeclarationSet
    attr_reader :visuals, :skills, :random_combat_skills, :monsters, :statuses,
                :gears, :encounter_pools, :tower_patterns, :biomes, :regressions, :biome_plans,
                :recruitment_opportunities, :story_events, :objectives, :dialogues, :autoplay_routes,
                :origin_auto_regressions, :equipment_profiles

    def self.load_file(path)
      new.tap { |set| set.load_file(path) }
    end

    def initialize
      @visuals = {}
      @skills = []
      @random_combat_skills = []
      @monsters = []
      @statuses = []
      @gears = []
      @equipment_profiles = []
      @encounter_pools = []
      @tower_patterns = []
      @biomes = []
      @regressions = []
      @biome_plans = []
      @recruitment_opportunities = []
      @story_events = []
      @objectives = []
      @dialogues = []
      @autoplay_routes = []
      @origin_auto_regressions = []
    end

    def load_file(path)
      instance_eval(File.read(path), path)
      regression_path = File.join(File.dirname(path), "fairlanes_regression_declarations.rb")
      instance_eval(File.read(regression_path), regression_path) if File.file?(regression_path)
      story_path = File.join(File.dirname(path), "fairlanes_story_declarations.rb")
      instance_eval(File.read(story_path), story_path) if File.file?(story_path)
      freeze_records
      self
    end

    def visual(id, cpp: nil)
      visuals[id] = cpp || cpp_name(id)
    end

    def skill(id, cpp_id: nil, display: nil, learn_chance_percent:, random_combat: true,
              flee_success_percent: 0,
              execution:, visual: nil, tags:, declarative_shape:, description: nil,
              consumes_status: nil, effect_damage: 0)
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
        declarative_shape: declarative_shape,
        description: description,
        consumes_status: consumes_status,
        effect_damage: effect_damage
      )
    end

    def decal_skill(id, visual:, tags:, learn_chance_percent: 5,
                    random_combat: true, cpp_id: nil, display: nil, description: nil)
      skill(
        id,
        cpp_id: cpp_id,
        display: display,
        learn_chance_percent: learn_chance_percent,
        random_combat: random_combat,
        execution: :decal_strike,
        visual: visual,
        tags: tags,
        declarative_shape: :decal_strike,
        description: description
      )
    end

    def random_combat_skills(*ids)
      return @random_combat_skills if ids.empty?

      @random_combat_skills.replace(ids)
    end

    def monster(id, cpp_id: nil, display: nil, hp:, level: nil,
                known_skills:, pool:, cycle:, decision_rules: [], description: nil,
                resistances: {})
      monsters << Monster.new(
        id: id,
        cpp_id: cpp_id || cpp_name(id),
        display: display || display_name(id),
        cycle: cycle,
        hp: hp,
        level: level,
        known_skills: known_skills,
        pool: pool,
        decision_rules: decision_rules,
        description: description,
        resistances: resistances
      )
    end

    def monster_rules(id, *rules)
      entry = monsters.find { |monster| monster.id == id }
      raise ArgumentError, "Unknown monster #{id}" unless entry
      entry.decision_rules = rules
    end

    def status(id, cpp_id: nil, display: nil, debug_name: nil, component: nil,
               palette_index:)
      statuses << Status.new(
        id: id,
        cpp_id: cpp_id || cpp_name(id),
        display: display || display_name(id),
        debug_name: debug_name || id.to_s.tr("_", "-"),
        component: component || cpp_name(id),
        palette_index: palette_index
      )
    end

    def gear(id, display:, slot:, armor_kind:, tier:, weight:, modifiers: {})
      gears << Gear.new(id: id, display: display, slot: slot,
                        armor_kind: armor_kind, tier: tier, weight: weight,
                        modifiers: modifiers)
    end

    def equipment_profile(biome, weights:, kinds:, armor_kinds:)
      equipment_profiles << EquipmentProfile.new(biome: biome, weights: weights, kinds: kinds,
                                                  armor_kinds: armor_kinds)
    end

    def encounter_pool(id, lineups:)
      encounter_pools << EncounterPool.new(id: id, lineups: lineups)
    end

    def tower_pattern(id, monsters:)
      tower_patterns << TowerPattern.new(id: id, monsters: monsters)
    end

    def biome(id, display:, floors:, bands: [], pattern: nil, overrides: [])
      biomes << Biome.new(
        id: id, display: display, floors: floors,
        bands: bands.map { |entry| FloorBand.new(**entry) }, pattern: pattern,
        overrides: overrides.map { |entry| FloorOverride.new(**entry) }
      )
    end

    def regression(id, encounter: nil, encounters: nil, party:, seeds:, beat_cap:,
                   recording_version: 1, variants:)
      sequence = encounters || [encounter]
      regressions << RegressionScenario.new(id: id, encounter: sequence.first,
        encounters: sequence, party: party, seeds: seeds, beat_cap: beat_cap,
        recording_version: recording_version, variants: variants)
    end

    def biome_plan(id, cycle:, order:, monsters:)
      biome_plans << BiomePlan.new(id: id, cycle: cycle, order: order,
                                  monsters: monsters)
    end

    def recruitment_opportunity(id, cycle:, destination_party:, biome: nil, floor: nil,
                                unlocked_by_party_completion: nil)
      recruitment_opportunities << RecruitmentOpportunity.new(
        id: id, cycle: cycle, biome: biome, floor: floor,
        destination_party: destination_party,
        unlocked_by_party_completion: unlocked_by_party_completion
      )
    end

    def story_event(id, handled: false)
      story_events << StoryEvent.new(id: id, handled: handled)
    end

    def objective(id, title:, description:, revealed_by:, completed_by:)
      objectives << Objective.new(id: id, title: title, description: description,
                                  revealed_by: revealed_by, completed_by: completed_by)
    end

    def dialogue(id, location:, npc:, start:, nodes:)
      dialogues << Dialogue.new(id: id, location: location, npc: npc,
                                start: start, nodes: nodes)
    end

    def autoplay_route(id, steps:)
      autoplay_routes << AutoplayRoute.new(id: id, steps: steps)
    end

    def origin_auto_regression(id, seed:, clears:, expected_decisions:)
      origin_auto_regressions << OriginAutoRegression.new(
        id: id, seed: seed, clears: clears, expected_decisions: expected_decisions
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
      statuses.each(&:freeze)
      statuses.freeze
      gears.each(&:freeze)
      gears.freeze
      equipment_profiles.each(&:freeze)
      equipment_profiles.freeze
      encounter_pools.each(&:freeze)
      encounter_pools.freeze
      tower_patterns.each(&:freeze)
      tower_patterns.freeze
      biomes.each(&:freeze)
      biomes.freeze
      regressions.each(&:freeze)
      regressions.freeze
      biome_plans.each(&:freeze)
      biome_plans.freeze
      recruitment_opportunities.each(&:freeze)
      recruitment_opportunities.freeze
      story_events.each(&:freeze)
      story_events.freeze
      objectives.each(&:freeze)
      objectives.freeze
      dialogues.each(&:freeze)
      dialogues.freeze
      autoplay_routes.each(&:freeze)
      autoplay_routes.freeze
      origin_auto_regressions.each(&:freeze)
      origin_auto_regressions.freeze
    end
  end
end
