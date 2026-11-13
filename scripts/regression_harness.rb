# frozen_string_literal: true

require "fileutils"
require "json"
require "pathname"
require_relative "quiet_command"

module FairlanesRegression
  SUMMARY_SCHEMA = "fairlanes-regression-experiment-summary"
  SUMMARY_VERSION = 2

  module Classifications
    SCHEMA = "fairlanes-combat-classification"
    VERSION = 1
    IDS = %w[no_progress stalemate_candidate control_lock resource_exhaustion attrition burst_dominance].freeze
    POLICIES = %w[required forbidden allowed observe].freeze
    NO_PROGRESS_UWU = 144_000
    CONTROL_LOCK_UWU = 28_800
    ATTRITION_UWU = 57_600
    BURST_UWU = 28_800

    module_function

    def result(id, encounter_index, status, evidence, scope: nil)
      {"schema" => SCHEMA, "version" => VERSION, "id" => id,
       "scope" => scope || {"kind" => "encounter", "encounter_index" => encounter_index},
       "status" => status, "evidence" => evidence}
    end

    def unavailable(encounter_index, missing)
      IDS.map do |id|
        result(id, encounter_index, "indeterminate",
               {"unavailable" => Array(missing).sort, "reason" => "required telemetry unavailable"})
      end
    end

    def analyze_encounter(encounter, index: nil)
      encounter_index = index || encounter["encounter_index"] || 0
      telemetry = encounter["telemetry"]
      return unavailable(encounter_index, ["telemetry"]) unless telemetry.is_a?(Hash)
      unless telemetry["schema"] == "fairlanes-combat-telemetry" && telemetry["version"] == 1
        return unavailable(encounter_index, ["supported_telemetry_schema"])
      end

      progress = telemetry["progress"]
      actions = telemetry["actions"]
      actors = telemetry["actors"]
      required = {"duration_uwu" => telemetry["duration_uwu"], "progress" => progress,
                  "actions" => actions, "actors" => actors}
      missing = required.filter_map { |key, value| key if value.nil? }
      return unavailable(encounter_index, missing) unless missing.empty?

      duration = telemetry.fetch("duration_uwu")
      changes = progress.fetch("meaningful_changes", nil)
      longest_idle = progress.fetch("longest_no_progress_uwu", nil)
      return unavailable(encounter_index, %w[meaningful_changes longest_no_progress_uwu]) if changes.nil? || longest_idle.nil?

      found = []
      resources = telemetry["resources"]
      resource_changes = resources.is_a?(Hash) && resources["available"] ? resources.fetch("state_changes", 0) : 0
      total_changes = changes + resource_changes
      no_progress = duration >= NO_PROGRESS_UWU && total_changes.zero?
      if no_progress
        found << result("no_progress", encounter_index, "observed",
                        {"interval_uwu" => {"from" => 0, "to" => duration},
                         "meaningful_changes" => total_changes,
                         "resource_state_changes" => resource_changes,
                         "longest_no_progress_uwu" => longest_idle})
      end

      completed = actions.fetch("completed", 0)
      nonproductive = actions.fetch("nonproductive", 0)
      if encounter["outcome"] == "beat_cap" && no_progress && completed >= 2 && nonproductive == completed
        found << result("stalemate_candidate", encounter_index, "suspected",
                        {"interval_uwu" => {"from" => 0, "to" => duration},
                         "terminal_outcome" => "beat_cap", "completed_actions" => completed,
                         "nonproductive_actions" => nonproductive,
                         "limitation" => "candidate only; future reachable states were not analyzed"})
      end

      locked = Array(actors).select do |actor|
        actor.fetch("longest_control_denial_uwu", 0) >= CONTROL_LOCK_UWU &&
          actor.fetch("control_denied_uwu", 0) * 2 >= duration
      end
      unless locked.empty?
        actor_ids = locked.map { |actor| actor.fetch("id") }.sort
        found << result("control_lock", encounter_index, "observed",
                        {"actors" => locked.map { |actor| actor.fetch("id") }.sort,
                         "interval_uwu" => {"minimum_duration" => CONTROL_LOCK_UWU},
                         "longest_denial_uwu" => locked.to_h { |actor| [actor.fetch("id"), actor.fetch("longest_control_denial_uwu")] }},
                        scope: {"kind" => "actors", "encounter_index" => encounter_index,
                                "actor_ids" => actor_ids})
      end

      if !resources.is_a?(Hash) || !resources["available"]
        found << result("resource_exhaustion", encounter_index, "indeterminate",
                        {"unavailable" => ["resource_availability"],
                         "reason" => resources&.fetch("reason", nil) || "resource telemetry unavailable"})
      elsif Array(resources["exhausted_actors"]).any?
        exhausted = Array(resources["exhausted_actors"]).sort
        found << result("resource_exhaustion", encounter_index, "observed",
                        {"actors" => exhausted, "resources" => Array(resources["resource_ids"]).sort},
                        scope: {"kind" => "actors", "encounter_index" => encounter_index,
                                "actor_ids" => exhausted})
      end

      hp_lost = progress.fetch("hp_lost", 0)
      if duration >= ATTRITION_UWU && hp_lost.positive? && total_changes >= 3 && longest_idle * 2 < duration
        found << result("attrition", encounter_index, "observed",
                        {"interval_uwu" => {"from" => 0, "to" => duration}, "hp_lost" => hp_lost,
                         "meaningful_changes" => total_changes, "longest_no_progress_uwu" => longest_idle})
      end

      victory = %w[hero_victory enemy_victory mouse_victory hare_victory].include?(encounter["outcome"])
      productive_sides = Array(actors).select { |actor| actor.fetch("productive_actions", 0).positive? }
                                     .map { |actor| actor.fetch("side") }.uniq.sort
      if victory && duration <= BURST_UWU && hp_lost.positive? && productive_sides.length == 1
        found << result("burst_dominance", encounter_index, "observed",
                        {"interval_uwu" => {"from" => 0, "to" => duration},
                         "productive_side" => productive_sides.first, "hp_lost" => hp_lost,
                         "productive_actions_by_opposition" => 0},
                        scope: {"kind" => "side", "encounter_index" => encounter_index,
                                "side" => productive_sides.first})
      end
      found.sort_by { |entry| [IDS.index(entry.fetch("id")), entry.dig("scope", "encounter_index")] }
    rescue KeyError => e
      unavailable(encounter_index, [e.key.to_s])
    end

    def analyze_run(run)
      encounters = Array(run["encounters"])
      encounters = [run["final_encounter"]].compact if encounters.empty?
      encounters.each_with_index.flat_map { |encounter, index| analyze_encounter(encounter, index:) }
    end

    def expectations(variant, classifications)
      declarations = variant.fetch("classification_expectations", {})
      IDS.filter_map do |id|
        policy = declarations.fetch(id, "observe")
        next if policy == "observe" && !declarations.key?(id)
        matches = classifications.select { |entry| entry["id"] == id && entry["status"] != "indeterminate" }
        passed = case policy
                 when "required" then !matches.empty?
                 when "forbidden" then matches.empty?
                 else true
                 end
        {"id" => id, "policy" => policy, "passed" => passed,
         "observed" => matches.length}
      end
    end
  end

  class Error < StandardError
    attr_reader :status

    def initialize(message, status: 1)
      super(message)
      @status = status
    end
  end

  class CommandRunner
    def run(command, log_path:, chdir:)
      FairlanesQuietCommand.run(command, log_path:, chdir:)
      ""
    rescue FairlanesQuietCommand::Failure => e
      raise Error.new(e.message, status: e.result.exitstatus)
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
      initial_stats = runs.flat_map do |run|
        Array(run["initial_party_state"]).filter_map { |member| member["stats"] }
      end
      stat_means = %w[attack armor max_hp].to_h do |stat|
        [stat, mean(initial_stats.map { |entry| entry.fetch(stat) })]
      end
      resistance_means = %w[physical magical fire ice lightning].to_h do |channel|
        [channel, mean(initial_stats.map { |entry| entry.fetch("resistances").fetch(channel) })]
      end
      combat = {
        "mean_damage_dealt" => mean(runs.map { |run| Array(run["encounters"]).sum { |entry| entry.fetch("outgoing_damage", 0) } }),
        "mean_damage_received" => mean(runs.map { |run| Array(run["encounters"]).sum { |entry| entry.fetch("incoming_damage", 0) } }),
        "mean_effective_stats" => stat_means.merge("resistances" => resistance_means),
        "monsters" => recordings.flat_map do |recording|
          Array(recording["initial_state"]).select { |actor| actor["kind"] == "monster" }.map do |actor|
            {"name" => actor.fetch("name"), "level" => actor.fetch("level"),
             "affinities" => actor.fetch("affinities"), "stats" => actor.fetch("stats")}
          end
        end.uniq
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
      classifications = runs.flat_map do |run|
        Classifications.analyze_run(run).map { |entry| entry.merge("seed" => run.fetch("seed")) }
      end
      classification_expectations = Classifications.expectations(variant, classifications)
      production_rows = runs.map do |run|
        Array(run["encounters"]).last&.dig("telemetry", "production") || {}
      end
      production_keys = %w[materials_acquired materials_consumed products_created
                           profession_xp_gained study_beats blocked_operations items_distributed
                           production_beats tower_progression_beats_forgone]
      production = production_keys.to_h do |key|
        [key, production_rows.sum { |row| row.fetch(key, 0) }]
      end
      all_assertions = (win_assertion.nil? || win_assertion) &&
                       semantic_assertions.all? { |assertion| assertion.fetch("passed") } &&
                       classification_expectations.all? { |expectation| expectation.fetch("passed") }
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
                           "classification_expectations" => classification_expectations,
                           "all_declared_assertions_passed" => all_assertions},
        "classifications" => {
          "counts" => classifications.group_by { |entry| [entry.fetch("id"), entry.fetch("status")] }
                                         .sort_by { |(id, status), _| [Classifications::IDS.index(id), status] }
                                         .to_h { |(id, status), entries| ["#{id}:#{status}", entries.length] },
          "evidence" => classifications.reject { |entry| entry["status"] == "indeterminate" }.first(12),
          "indeterminate" => classifications.count { |entry| entry["status"] == "indeterminate" }
        },
        "mechanics" => mechanics,
        "combat" => combat,
        "production" => production
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
                 "--record-dir", recordings_dir, "--summary-dir", summaries_dir]
      command << "--assert-expected" if variant["expected_wins"]
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

    def inspect_recording(path, encounter: nil, skill: nil, mechanic: nil, classification: nil, limit: 60)
      recording = read_json(path)
      if classification
        unless Classifications::IDS.include?(classification)
          raise Error, "unknown classification: #{classification}"
        end
        results = Array(recording["regression_telemetry"])
        results = Array(recording["encounter_results"]) if results.empty?
        results = [recording["result"]].compact if results.empty?
        evidence = results.each_with_index.flat_map do |result, index|
          Classifications.analyze_encounter(result, index:)
        end.select { |entry| entry["id"] == classification }
        evidence.select! { |entry| entry.dig("scope", "encounter_index") == encounter } if encounter
        return {"schema" => "fairlanes-regression-classification-inspection", "version" => 1,
                "fixture" => recording["fixture"], "seed" => recording["gameplay_seed"],
                "outcome" => recording.dig("result", "outcome"),
                "classification" => classification, "evidence" => evidence,
                "recording" => path}
      end
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

    def recruitment_report(build_first: true)
      build if build_first
      entries = catalog.manifest.fetch("recruitment_opportunities")
      runner.run([File.join(build_dir, "fairlanes_tests"), "[recruitment]"],
                 log_path: File.join(artifact_root, "logs/recruitment.log"), chdir: root)
      milestones = entries.count { |entry| entry["floor"] }
      party_five = entries.count do |entry|
        entry["destination_party"] == 5 && entry["unlocked_by_party_completion"] == 4
      end
      {"schema" => "fairlanes-recruitment-progression-summary", "version" => 1,
       "milestones" => {"declared" => milestones, "expected" => 20},
       "party_five_opportunities" => {"declared" => party_five, "expected" => 5},
       "checks" => {"floor_clear_commits" => true, "opportunity_idempotency" => true,
                    "recruitment_authority" => true, "out_of_order_biomes" => true,
                    "party_capacity" => true, "fifth_party_special_unlock" => true,
                    "production_integration" => true},
       "log" => File.join(artifact_root, "logs/recruitment.log")}
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
