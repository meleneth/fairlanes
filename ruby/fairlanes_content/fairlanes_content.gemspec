# frozen_string_literal: true

Gem::Specification.new do |spec|
  spec.name = "fairlanes_content"
  spec.version = "0.1.0"
  spec.summary = "Fairlanes content declaration generator"
  spec.authors = ["Fairlanes"]
  spec.files = Dir["lib/**/*.rb"]
  spec.require_paths = ["lib"]
  spec.required_ruby_version = ">= 3.2"

  spec.add_development_dependency "awesome_print", "~> 1.9"
  spec.add_development_dependency "factory_bot", "~> 6.5"
  spec.add_development_dependency "rspec", "~> 3.13"
end
