# frozen_string_literal: true

require "fileutils"
require "json"
require "open3"
require "pathname"

module FairlanesRegression
  SUMMARY_SCHEMA = "fairlanes-regression-experiment-summary"
  SUMMARY_VERSION = 1

  class Error < StandardError
    attr_reader :status

    def initialize(message, status: 1)
      super(message)
      @status = status
    end
  end

  class CommandRunner
    DIAGNOSTIC = /(error:|fatal:|FAILED:|failed:|Assertion|undefined reference|replay divergence)/i

    def run(command, log_path:, chdir:)
      FileUtils.mkdir_p(File.dirname(log_path))
      stdout, stderr, status = Open3.capture3(*command, chdir: chdir)
      File.write(log_path, stdout + stderr)
      return stdout if status.success?

      diagnostics = (stdout + stderr).lines.grep(DIAGNOSTIC).first(20).join
      diagnostics = "No focused diagnostic found; see the complete log.\n" if diagnostics.empty?
      raise Error.new("FAILED: #{command.join(' ')}\nexit=#{status.exitstatus}\n#{diagnostics}log=#{log_path}",
                      status: status.exitstatus)
    end
  end

  class Catalog
    attr_reader :manifest

    def initialize(path)
      @manifest = JSON.parse(File.read(path))
    rescue Errno::ENOENT
      raise Error, "generated regression manifest is missing: #{path}"
    rescue JSON::ParserError => e
      raise Error, "generated regression manifest is malformed: #{path}: #{e.message}"
    end

    def regressions = manifest.fetch("regressions")

    def fixture(id)
      regressions.find { |entry| entry.fetch("id") == id } ||
        raise(Error, "unknown regression fixture: #{id}")
    end

    def variants(fixture, requested = nil)
      variants = fixture.fetch("variants")
      return variants if requested.nil? || requested.empty?

      selected = variants.select { |entry| requested.include?(entry.fetch("id")) }
      missing = requested - selected.map { |entry| entry.fetch("id") }
      raise Error, "unknown variant for #{fixture.fetch('id')}: #{missing.join(', ')}" unless missing.empty?

      selected
    end
  end

  module Metrics
    module_function

    def mean(values)
      return nil if values.empty?

      values.sum.to_f / values.length
    end

    def range(values)
      return {"min" => nil, "max" => nil} if values.empty?

      {"min" => values.min, "max" => values.max}
    end

    def known_skills(party)
      Array(party).flat_map { |member| Array(member["known_skills"]) }.uniq.sort
    end

    def learned_skills(run)
      known_skills(run.fetch("final_party_state")) - known_skills(run.fetch("initial_party_state"))
    end

    def retained_skills(run)
      learned = learned_skills(run)
      later = Array(run["party_state_before_encounters"])[1..] || []
      learned.select { |skill| later.any? { |party| known_skills(party).include?(skill) } }
    end

    def sum_encounters(runs, field)
      runs.sum { |run| Array(run["encounters"]).sum { |encounter| encounter.fetch(field, 0) } }
    end

    def cover_metrics(recordings)
      interceptions = []
      releases = 0
      recordings.each do |recording|
        actions = {}
        recording.fetch("events", []).each do |event|
          key = [event["encounter_index"] || 0, event["actor"], event["skill"]]
          if event["type"] == "action"
            actions[key] = event["target"]
            releases += 1 if event["skill"] == "bumper_rush/1"
          elsif event["type"] == "hit" && event["skill"] == "pebble_spit/1" &&
                actions[key] && actions[key] != event["target"]
            interceptions << {"skill" => event["skill"], "actor" => event["actor"],
                              "aimed_target" => actions[key], "resolved_target" => event["target"]}
          end
        end
      end
      release_events = interceptions.empty? ? 0 : releases
      {"interceptions" => interceptions.length,
       "examples" => interceptions.uniq.first(5),
       "release_skill" => release_events.positive? ? "bumper_rush/1" : nil,
       "release_condition" => release_events.positive? ? "bumper_rush_cast" : nil,
       "release_events" => release_events}
    end

    def aggregate(fixture, variant, runs, recordings)
      outcomes = runs.map { |run| run.fetch("outcome") }
      durations = runs.map { |run| run.fetch("simulated_seconds") }
      hp = runs.map { |run| run.fetch("remaining_party_hp") }
      survivors = runs.map { |run| run.fetch("survivors") }
      expected = variant["expected_wins"]
      victories = outcomes.count("hero_victory")
      caps = outcomes.count("beat_cap")
      learned = runs.flat_map { |run| learned_skills(run) }
      retained = runs.flat_map { |run| retained_skills(run) }
      final_loadouts = runs.flat_map do |run|
        Array(run["final_party_state"]).flat_map { |member| Array(member["loadout"]) }
      end
      mechanics = {
        "observe" => {
          "successful_observations" => learned.length,
          "skills_learned" => learned.tally,
          "skills_retained" => retained.tally,
          "final_equipped_skills" => final_loadouts.tally
        },
        "brace" => {
          "applications" => sum_encounters(runs, "brace_applications"),
          "consumptions" => sum_encounters(runs, "brace_consumptions"),
          "qualifying_hits_mitigated" => sum_encounters(runs, "brace_mitigated_hits"),
          "damage_prevented" => sum_encounters(runs, "brace_mitigated_damage")
        },
        "bleed" => {
          "eviscerate_casts" => sum_encounters(runs, "eviscerate_casts"),
          "applications" => sum_encounters(runs, "bleed_applications"),
          "ticks" => sum_encounters(runs, "bleed_ticks"),
          "damage" => sum_encounters(runs, "bleed_tick_damage"),
          "deaths" => sum_encounters(runs, "bleed_deaths")
        },
        "cover" => cover_metrics(recordings)
      }
      win_assertion = expected && victories.between?(expected[0], expected[1]) && caps.zero?
      semantic_values = {
        "brace_learned" => mechanics.dig("observe", "skills_learned").fetch("brace/1", 0),
        "brace_retained" => mechanics.dig("observe", "skills_retained").fetch("brace/1", 0),
        "brace_equipped" => mechanics.dig("observe", "final_equipped_skills").fetch("brace/1", 0),
        "brace_applied" => mechanics.dig("brace", "applications"),
        "brace_consumed" => mechanics.dig("brace", "consumptions"),
        "brace_mitigated" => mechanics.dig("brace", "damage_prevented"),
        "eviscerate_cast" => mechanics.dig("bleed", "eviscerate_casts"),
        "bleed_applied" => mechanics.dig("bleed", "applications"),
        "bleed_ticked" => mechanics.dig("bleed", "ticks"),
        "bleed_death" => mechanics.dig("bleed", "deaths"),
        "cover_intercepted" => mechanics.dig("cover", "interceptions"),
        "cover_released" => mechanics.dig("cover", "release_events")
      }
      semantic_assertions = variant.fetch("assertions", []).map do |id|
        {"id" => id, "passed" => semantic_values.fetch(id).positive?,
         "observed" => semantic_values.fetch(id)}
      end
      all_assertions = (win_assertion.nil? || win_assertion) &&
                       semantic_assertions.all? { |assertion| assertion.fetch("passed") }
      {
        "fixture" => fixture.fetch("id"), "variant" => variant.fetch("id"),
        "seeds" => runs.length, "victories" => victories,
        "losses" => runs.length - victories - caps, "beat_caps" => caps,
        "duration_seconds" => {"mean" => mean(durations), **range(durations)},
        "remaining_party_hp" => {"mean" => mean(hp), **range(hp)},
        "survivors" => {"mean" => mean(survivors), **range(survivors)},
        "seed_results" => runs.map do |run|
          {"seed" => run.fetch("seed"), "outcome" => run.fetch("outcome"),
           "simulated_seconds" => run.fetch("simulated_seconds"),
           "remaining_party_hp" => run.fetch("remaining_party_hp"),
           "survivors" => run.fetch("survivors")}
        end,
        "correctness" => {"declared_win_range" => expected,
                           "win_assertion_passed" => win_assertion,
                           "semantic_assertions" => semantic_assertions,
                           "all_declared_assertions_passed" => all_assertions},
        "mechanics" => mechanics
      }
    end

    def comparisons(variants)
      return [] if variants.length < 2

      baseline = variants.first
      variants.drop(1).map do |candidate|
        {"baseline" => baseline.fetch("variant"), "candidate" => candidate.fetch("variant"),
         "delta" => {
           "victories" => candidate.fetch("victories") - baseline.fetch("victories"),
           "mean_duration_seconds" => candidate.dig("duration_seconds", "mean") - baseline.dig("duration_seconds", "mean"),
           "mean_remaining_party_hp" => candidate.dig("remaining_party_hp", "mean") - baseline.dig("remaining_party_hp", "mean"),
           "mean_survivors" => candidate.dig("survivors", "mean") - baseline.dig("survivors", "mean")
         }}
      end
    end
  end

  class Harness
    attr_reader :root, :build_dir, :runner

    def initialize(root:, build_dir: "build-linux-debug", runner: CommandRunner.new)
      @root = File.expand_path(root)
      @build_dir = File.expand_path(build_dir, @root)
      @runner = runner
    end

    def manifest_path = File.join(build_dir, "generated/fairlanes_content/content_manifest.json")
    def artifact_root = File.join(build_dir, "regression-artifacts")
    def binary = File.join(build_dir, "fairlanes_regression")

    def build
      runner.run(["cmake", "--build", build_dir, "-j2"],
                 log_path: File.join(artifact_root, "logs/build.log"), chdir: root)
    end

    def catalog = Catalog.new(manifest_path)

    def experiment(fixture_id, requested_variants: nil, build_first: true)
      build if build_first
      fixture = catalog.fixture(fixture_id)
      selected = catalog.variants(fixture, requested_variants)
      variants = selected.map { |variant| execute_variant(fixture, variant) }
      {"schema" => SUMMARY_SCHEMA, "version" => SUMMARY_VERSION,
       "fixture" => fixture_id, "variants" => variants,
       "comparisons" => Metrics.comparisons(variants),
       "success" => variants.all? { |variant| variant.dig("correctness", "all_declared_assertions_passed") },
       "artifacts" => File.join(artifact_root, fixture_id)}
    end

    def execute_variant(fixture, variant)
      fixture_id = fixture.fetch("id")
      variant_id = variant.fetch("id")
      base = File.join(artifact_root, fixture_id, variant_id)
      recordings_dir = File.join(base, "recordings")
      summaries_dir = File.join(base, "summaries")
      command = [binary, "--fixture", fixture_id, "--variant", variant_id, "--all-seeds",
                 "--record-dir", recordings_dir, "--summary-dir", summaries_dir,
                 "--assert-expected"]
      runner.run(command, log_path: File.join(base, "run.log"), chdir: root)
      runs = fixture.fetch("seeds").map do |seed|
        read_json(File.join(summaries_dir, "#{fixture_id}.#{variant_id}.seed#{seed}.summary.json"))
      end
      recordings = fixture.fetch("seeds").map do |seed|
        read_json(File.join(recordings_dir, "#{fixture_id}.#{variant_id}.seed#{seed}.json"))
      end
      Metrics.aggregate(fixture, variant, runs, recordings)
    end

    def read_json(path)
      JSON.parse(File.read(path))
    rescue Errno::ENOENT
      raise Error, "recording or summary is missing: #{path}"
    rescue JSON::ParserError => e
      raise Error, "malformed JSON: #{path}: #{e.message}"
    end

    def inspect_recording(path, encounter: nil, skill: nil, mechanic: nil, limit: 60)
      recording = read_json(path)
      events = recording.fetch("events")
      events = events.select { |event| (event["encounter_index"] || 0) == encounter } if encounter
      events = events.select { |event| event["skill"] == skill || event["type"].include?(skill) } if skill
      if mechanic
        types = {"brace" => %w[brace_mitigated state action hit],
                 "bleed" => %w[dire_bleed_applied dire_bleed_tick dire_bleed_cleared cast_start],
                 "cover" => %w[action hit]}[mechanic] || raise(Error, "unknown mechanic: #{mechanic}")
        events = events.select { |event| types.include?(event["type"]) }
      end
      {"fixture" => recording["fixture"], "seed" => recording["gameplay_seed"],
       "outcome" => recording.dig("result", "outcome"),
       "events" => events.first(limit), "truncated" => events.length > limit,
       "recording" => path}
    rescue KeyError => e
      raise Error, "malformed recording: #{path}: missing #{e.key}"
    end

    def verify_recording(path)
      absolute = File.expand_path(path, root)
      raise Error, "recording is missing: #{path}" unless File.file?(absolute)

      log_path = File.join(artifact_root, "logs", "inspect_replay_#{File.basename(path)}.log")
      runner.run([File.join(build_dir, "fairlanes"), "--replay", absolute, "--no-ui"],
                 log_path: log_path, chdir: root)
      {"recording" => absolute, "replay" => "matched", "log" => log_path}
    end

    def verify
      build
      steps = [
        ["regression tests", [File.join(build_dir, "fairlanes_tests"), "[regression]"]],
        ["Ruby specs", ["ruby", "-Ilib", "-S", "rspec"],
         File.join(root, "ruby/fairlanes_content")],
        ["content generation", ["cmake", "--build", build_dir, "--target", "fairlanes_content_check", "-j2"]]
      ]
      passed = ["build"]
      steps.each do |name, command, chdir|
        runner.run(command, log_path: File.join(artifact_root, "logs/#{name.tr(' ', '_')}.log"),
                   chdir: chdir || root)
        passed << name
      end
      Dir[File.join(root, "samples/regression/*.json")].sort.each do |path|
        runner.run([File.join(build_dir, "fairlanes"), "--replay", path, "--no-ui"],
                   log_path: File.join(artifact_root, "logs/replay_#{File.basename(path)}.log"),
                   chdir: root)
      end
      passed << "#{Dir[File.join(root, 'samples/regression/*.json')].length} pinned replays"
      passed
    end
  end
end
