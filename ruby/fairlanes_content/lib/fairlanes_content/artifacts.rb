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
        "fl/generated/skill_content.hpp" => renderer.skill_content_hpp,
        "fl/generated/skill_content.cpp" => renderer.skill_content_cpp,
        "fl/generated/monster_content.hpp" => renderer.monster_content_hpp,
        "fl/generated/monster_content.cpp" => renderer.monster_content_cpp,
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
        skills
        random_combat_skills
        monsters
      ]
      required.each { |key| errors << "manifest missing #{key}" unless manifest.key?(key) }

      errors << "manifest schema_version must be 1" unless manifest["schema_version"] == 1
      errors << "manifest visuals must be an array" unless manifest["visuals"].is_a?(Array)
      errors << "manifest skills must be an array" unless manifest["skills"].is_a?(Array)
      unless manifest["random_combat_skills"].is_a?(Array)
        errors << "manifest random_combat_skills must be an array"
      end
      errors << "manifest monsters must be an array" unless manifest["monsters"].is_a?(Array)

      validate_manifest_skills(manifest, errors)
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

    def validate_manifest_monsters(manifest, errors)
      return unless manifest["monsters"].is_a?(Array)

      manifest["monsters"].each do |monster|
        %w[id cpp_id display known_skills pool].each do |key|
          errors << "manifest monster missing #{key}" unless monster.key?(key)
        end
        unless monster["known_skills"].is_a?(Array)
          errors << "manifest monster #{monster['id']} known_skills must be an array"
        end
      end
    end
  end
end
