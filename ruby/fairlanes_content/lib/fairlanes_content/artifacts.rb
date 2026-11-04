# frozen_string_literal: true

require "fileutils"
require "json"

require "fairlanes_content/renderer"

module FairlanesContent
  class ManifestShapeError < StandardError
    attr_reader :errors

    def initialize(errors)
      @errors = errors
      super(errors.join("\n"))
    end
  end

  class StaleArtifactsError < StandardError
    attr_reader :errors

    def initialize(errors)
      @errors = errors
      super(errors.join("\n"))
    end
  end

  class Artifacts
    def initialize(declarations)
      @renderer = Renderer.new(declarations)
    end

    def generated
      manifest_json = renderer.manifest
      validate_manifest_shape!(manifest_json)

      files = {
        "generated_decal_content_tests.cpp" => renderer.test_cpp,
        "fl/generated/skill_ids.hpp" => renderer.skill_ids_hpp,
        "fl/generated/skill_content.hpp" => renderer.skill_content_hpp,
        "fl/generated/skill_content.cpp" => renderer.skill_content_cpp,
        "fl/generated/status_content.hpp" => renderer.status_content_hpp,
        "fl/generated/status_content.cpp" => renderer.status_content_cpp,
        "fl/generated/monster_kinds.hpp" => renderer.monster_kinds_hpp,
        "fl/generated/monster_content.hpp" => renderer.monster_content_hpp,
        "fl/generated/monster_content.cpp" => renderer.monster_content_cpp,
        "fl/generated/monster_registration.hpp" => renderer.monster_registration_hpp,
        "fl/generated/monster_registration.cpp" => renderer.monster_registration_cpp,
        "fl/generated/gear_content.hpp" => renderer.gear_content_hpp,
        "fl/generated/floor_content.hpp" => renderer.floor_content_hpp,
        "fl/generated/regression_content.hpp" => renderer.regression_content_hpp,
        "fl/generated/story_content.hpp" => renderer.story_content_hpp,
        "decal_content_balance.md" => renderer.balance_report,
        "floor_manifest.md" => renderer.floor_manifest,
        "effect_gallery.md" => renderer.effect_gallery,
        "content_manifest.json" => manifest_json,
        "content_manifest.schema.json" => renderer.manifest_schema
      }

      renderer.skills.each do |skill|
        files["fl/generated/skills/#{skill.id}.hpp"] = renderer.skill_entry_hpp(skill)
        files["fl/generated/skills/#{skill.id}.cpp"] = renderer.skill_entry_cpp(skill)
      end

      renderer.monsters.each do |monster|
        files["fl/generated/monsters/#{monster.id}.hpp"] =
          renderer.monster_entry_hpp(monster)
        files["fl/generated/monsters/#{monster.id}.cpp"] =
          renderer.monster_entry_cpp(monster)
      end
      files
    end

    def generated_files
      generated.keys
    end

    def generated_docs
      {
        "generated/content-balance.md" => renderer.balance_report,
        "generated/effect-gallery.md" => renderer.effect_gallery,
        "generated/floor-manifest.md" => renderer.floor_manifest,
        "generated/story-events.md" => renderer.story_event_doc
      }
    end

    def generated_doc_files
      generated_docs.keys
    end

    def generated_brainstorms
      { "biome-tower-map.md" => renderer.biome_plan_doc }
    end

    def write_brainstorms!(dir)
      FileUtils.mkdir_p(dir)
      generated_brainstorms.each do |name, contents|
        path = File.join(dir, name)
        File.binwrite(path, contents) unless File.exist?(path) && File.read(path) == contents
      end
    end

    def check_brainstorms!(dir)
      errors = generated_brainstorms.filter_map do |name, contents|
        path = File.join(dir, name)
        "#{name} is missing or stale" unless File.exist?(path) && File.read(path) == contents
      end
      raise StaleArtifactsError, errors unless errors.empty?
      true
    end

    def fl_source_files
      generated_files.select do |filename|
        filename.end_with?(".cpp") &&
          filename != "generated_decal_content_tests.cpp"
      end
    end

    def write!(out_dir)
      FileUtils.mkdir_p(out_dir)
      generated.each do |filename, contents|
        path = File.join(out_dir, filename)
        next if File.exist?(path) && File.binread(path) == contents

        FileUtils.mkdir_p(File.dirname(path))
        File.binwrite(path, contents)
      end
    end

    def write_docs!(docs_dir)
      FileUtils.mkdir_p(docs_dir)
      generated_docs.each do |filename, contents|
        path = File.join(docs_dir, filename)
        next if File.exist?(path) && File.binread(path) == contents

        FileUtils.mkdir_p(File.dirname(path))
        File.binwrite(path, contents)
      end
    end

    def check!(out_dir)
      errors = []
      generated.each do |filename, expected|
        path = File.join(out_dir, filename)
        unless File.exist?(path)
          errors << "#{filename} is missing"
          next
        end

        actual = File.read(path)
        errors << "#{filename} is stale" unless actual == expected
      end

      raise StaleArtifactsError, errors unless errors.empty?

      true
    end

    def check_docs!(docs_dir)
      errors = []
      generated_docs.each do |filename, expected|
        path = File.join(docs_dir, filename)
        unless File.exist?(path)
          errors << "#{filename} is missing"
          next
        end

        actual = File.read(path)
        errors << "#{filename} is stale" unless actual == expected
      end

      raise StaleArtifactsError, errors unless errors.empty?

      true
    end

    private

    attr_reader :renderer

    def validate_manifest_shape!(manifest_json)
      manifest = JSON.parse(manifest_json)
      errors = []

      required = %w[
        schema_version
        source
        runtime_authority
        visuals
        statuses
        skills
        random_combat_skills
        monsters
        gear
        encounter_pools
        tower_patterns
        biomes
        biome_plans
        story_events
        objectives
        dialogues
        autoplay_routes
        regressions
      ]
      required.each { |key| errors << "manifest missing #{key}" unless manifest.key?(key) }

      errors << "manifest schema_version must be 1" unless manifest["schema_version"] == 1
      errors << "manifest visuals must be an array" unless manifest["visuals"].is_a?(Array)
      errors << "manifest statuses must be an array" unless manifest["statuses"].is_a?(Array)
      errors << "manifest skills must be an array" unless manifest["skills"].is_a?(Array)
      unless manifest["random_combat_skills"].is_a?(Array)
        errors << "manifest random_combat_skills must be an array"
      end
      errors << "manifest monsters must be an array" unless manifest["monsters"].is_a?(Array)
      errors << "manifest gear must be an array" unless manifest["gear"].is_a?(Array)
      errors << "manifest encounter_pools must be an array" unless manifest["encounter_pools"].is_a?(Array)
      errors << "manifest tower_patterns must be an array" unless manifest["tower_patterns"].is_a?(Array)
      errors << "manifest biomes must be an array" unless manifest["biomes"].is_a?(Array)
      errors << "manifest biome_plans must be an array" unless manifest["biome_plans"].is_a?(Array)
      errors << "manifest story_events must be an array" unless manifest["story_events"].is_a?(Array)
      errors << "manifest objectives must be an array" unless manifest["objectives"].is_a?(Array)
      errors << "manifest dialogues must be an array" unless manifest["dialogues"].is_a?(Array)
      errors << "manifest autoplay_routes must be an array" unless manifest["autoplay_routes"].is_a?(Array)
      errors << "manifest regressions must be an array" unless manifest["regressions"].is_a?(Array)

      validate_manifest_skills(manifest, errors)
      validate_manifest_statuses(manifest, errors)
      validate_manifest_monsters(manifest, errors)

      raise ManifestShapeError, errors unless errors.empty?

      true
    end

    def validate_manifest_skills(manifest, errors)
      return unless manifest["skills"].is_a?(Array)

      manifest["skills"].each do |skill|
        %w[id cpp_id display execution tags declarative_shape].each do |key|
          errors << "manifest skill missing #{key}" unless skill.key?(key)
        end
        unless skill["learn_chance_percent"].is_a?(Integer)
          errors << "manifest skill #{skill['id']} learn_chance_percent is not an integer"
        end
        unless skill["flee_success_percent"].is_a?(Integer)
          errors << "manifest skill #{skill['id']} flee_success_percent is not an integer"
        end
        unless skill["random_combat"] == true || skill["random_combat"] == false
          errors << "manifest skill #{skill['id']} random_combat is not boolean"
        end
        errors << "manifest skill #{skill['id']} tags must be an array" unless skill["tags"].is_a?(Array)
      end
    end

    def validate_manifest_statuses(manifest, errors)
      return unless manifest["statuses"].is_a?(Array)

      manifest["statuses"].each do |status|
        %w[id cpp_id display debug_name component palette_index].each do |key|
          errors << "manifest status missing #{key}" unless status.key?(key)
        end
        unless status["palette_index"].is_a?(Integer)
          errors << "manifest status #{status['id']} palette_index is not an integer"
        end
      end
    end

    def validate_manifest_monsters(manifest, errors)
      return unless manifest["monsters"].is_a?(Array)

      manifest["monsters"].each do |monster|
        %w[id cpp_id display description cycle hp level resistances known_skills pool].each do |key|
          errors << "manifest monster missing #{key}" unless monster.key?(key)
        end
        unless monster["description"].is_a?(String) && !monster["description"].strip.empty?
          errors << "manifest monster #{monster['id']} description must be nonempty text"
        end
        unless CYCLES.map(&:to_s).include?(monster["cycle"])
          errors << "manifest monster #{monster['id']} has invalid cycle"
        end
        unless monster["hp"].is_a?(Integer)
          errors << "manifest monster #{monster['id']} hp is not an integer"
        end
        unless monster["resistances"].is_a?(Hash)
          errors << "manifest monster #{monster['id']} resistances must be an object"
        end
        unless monster["level"].nil? || monster["level"].is_a?(Integer)
          errors << "manifest monster #{monster['id']} level is not an integer or null"
        end
        unless monster["known_skills"].is_a?(Array)
          errors << "manifest monster #{monster['id']} known_skills must be an array"
        end
      end
    end
  end
end
