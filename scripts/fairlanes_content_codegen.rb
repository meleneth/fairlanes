#!/usr/bin/env ruby
# frozen_string_literal: true

$LOAD_PATH.unshift(File.expand_path("../ruby/fairlanes_content/lib", __dir__))

require "fairlanes_content"

declarations_file = File.expand_path("fairlanes_content_declarations.rb", __dir__)
exit FairlanesContent::CLI.run(ARGV, declarations_file: declarations_file)
