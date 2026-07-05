# frozen_string_literal: true

require "awesome_print"
require "factory_bot"
require "tmpdir"

require "fairlanes_content"

Dir[File.join(__dir__, "support", "**", "*.rb")].sort.each { |file| require file }

RSpec.configure do |config|
  config.include FactoryBot::Syntax::Methods

  config.expect_with :rspec do |expectations|
    expectations.syntax = :expect
  end

  config.before(:suite) do
    FactoryBot.find_definitions
  end
end
