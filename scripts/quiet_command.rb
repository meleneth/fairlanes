# frozen_string_literal: true

require "fileutils"
require "open3"

module FairlanesQuietCommand
  Result = Data.define(:command, :log_path, :status, :duration, :compiled_units,
                       :warnings, :output_lines) do
    def success? = status.success?
    def exitstatus = status.exitstatus || 128 + status.termsig
  end

  class Failure < StandardError
    attr_reader :result

    def initialize(result)
      @result = result
      super(FairlanesQuietCommand.failure_report(result))
    end
  end

  DIAGNOSTIC = /(?:^|\s)(?:FAILED:|error:|fatal error:|collect2: error:|undefined reference|ld: |ninja: error:|CMake Error|GeneratorError|Assertion failed|REQUIRE\(|CHECK\(|\*\*\*Failed|Errors while running CTest|replay divergence)/i
  ANSI = /\e\[[0-9;]*[A-Za-z]/

  module_function

  def run(command, log_path:, chdir:, verbose: false, environment: {})
    FileUtils.mkdir_p(File.dirname(log_path))
    started = Process.clock_gettime(Process::CLOCK_MONOTONIC)
    compiled_units = 0
    warnings = 0
    output_lines = 0
    status = nil

    File.open(log_path, "w") do |log|
      Open3.popen2e(environment, *command, chdir: chdir) do |stdin, output, wait|
        stdin.close
        output.each_line do |line|
          log.write(line)
          $stdout.write(line) if verbose
          plain = line.gsub(ANSI, "")
          compiled_units += 1 if plain.match?(/(?:Building|Compiling) (?:C|CXX) object/)
          warnings += 1 if plain.match?(/\bwarning:/i)
          output_lines += 1
        end
        status = wait.value
      end
    end

    result = Result.new(command:, log_path:, status:,
                        duration: Process.clock_gettime(Process::CLOCK_MONOTONIC) - started,
                        compiled_units:, warnings:, output_lines:)
    raise Failure.new(result) unless result.success?

    result
  end

  def diagnostic_excerpt(log_path, limit: 60, context: 2)
    lines = File.readlines(log_path, chomp: true)
    matches = lines.each_index.select { |index| lines[index].gsub(ANSI, "").match?(DIAGNOSTIC) }
    return lines.last([limit, lines.length].min).join("\n") if matches.empty?

    selected = matches.flat_map do |index|
      (([index - context, 0].max)..([index + context, lines.length - 1].min)).to_a
    end.uniq.first(limit)
    selected.map { |index| lines[index] }.join("\n")
  rescue Errno::ENOENT
    "Complete log was not created."
  end

  def failure_report(result)
    excerpt = diagnostic_excerpt(result.log_path)
    <<~REPORT.chomp
      FAIL
        Exit: #{result.exitstatus}
        Command: #{result.command.join(' ')}
      #{indent(excerpt, 2)}
        Log: #{result.log_path}
    REPORT
  end

  def indent(text, spaces)
    prefix = " " * spaces
    text.lines.map { |line| "#{prefix}#{line}" }.join
  end
end
