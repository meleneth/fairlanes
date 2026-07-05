# frozen_string_literal: true

require "optparse"

require "fairlanes_content/artifacts"
require "fairlanes_content/declaration_set"
require "fairlanes_content/validator"

module FairlanesContent
  class CLI
    def self.run(argv, declarations_file:)
      new(argv, declarations_file: declarations_file).run
    end

    def initialize(argv, declarations_file:)
      @argv = argv
      @declarations_file = declarations_file
      @options = {}
    end

    def run
      parse!
      declarations = DeclarationSet.load_file(declarations_file)
      Validator.new(declarations).validate!
      artifacts = Artifacts.new(declarations)

      if options[:check]
        artifacts.check!(options.fetch(:out_dir))
      else
        artifacts.write!(options.fetch(:out_dir))
      end

      0
    rescue ValidationError => e
      warn "Fairlanes content declaration validation failed:"
      e.errors.each { |error| warn "  - #{error}" }
      1
    rescue ManifestShapeError => e
      warn "Fairlanes generated manifest validation failed:"
      e.errors.each { |error| warn "  - #{error}" }
      1
    rescue StaleArtifactsError => e
      warn "Fairlanes generated content check failed:"
      e.errors.each { |error| warn "  - #{error}" }
      1
    end

    private

    attr_reader :argv, :declarations_file, :options

    def parse!
      OptionParser.new do |parser|
        parser.banner = "Usage: ruby scripts/fairlanes_content_codegen.rb --out-dir DIR"
        parser.on("--out-dir DIR", "Directory for generated artifacts") do |dir|
          options[:out_dir] = dir
        end
        parser.on("--check", "Fail if generated artifacts are missing or stale") do
          options[:check] = true
        end
      end.parse!(argv)

      raise OptionParser::MissingArgument, "--out-dir" unless options[:out_dir]
    rescue OptionParser::ParseError => e
      warn e.message
      exit 2
    end
  end
end
