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

      {
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
        "decal_content_balance.md" => renderer.balance_report,
        "effect_gallery.md" => renderer.effect_gallery,
        "content_manifest.json" => manifest_json,
        "content_manifest.schema.json" => renderer.manifest_schema
      }
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
        %w[id cpp_id display hp mp level known_skills pool].each do |key|
          errors << "manifest monster missing #{key}" unless monster.key?(key)
        end
        unless monster["hp"].is_a?(Integer)
          errors << "manifest monster #{monster['id']} hp is not an integer"
        end
        unless monster["mp"].is_a?(Integer)
          errors << "manifest monster #{monster['id']} mp is not an integer"
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
