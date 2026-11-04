# frozen_string_literal: true

require "digest"

module FairlanesContent
  class FloorResolver
    def initialize(declarations)
      @declarations = declarations
    end

    # Validation runs before resolution. The digest is scoped to one biome and
    # floor, so edits to another band cannot consume or shift random draws.
    def resolve(biome)
      pools = @declarations.encounter_pools.to_h { |pool| [pool.id, pool] }
      patterns = @declarations.tower_patterns.to_h { |pattern| [pattern.id, pattern] }
      overrides = biome.overrides.to_h { |entry| [entry.floor, entry] }
      (1..biome.floors).map do |number|
        override = overrides[number]
        if override
          percent = scale_percent(patterns[biome.pattern], number, biome.floors)
          { floor: number, id: floor_id(biome.id, number), source: "override",
            lineup: override.lineup, level_percent: percent,
            hp_percent: percent, attack_percent: percent }
        elsif biome.pattern
          pattern = patterns.fetch(biome.pattern)
          percent = pattern.scale_percent(number, biome.floors)
          { floor: number, id: floor_id(biome.id, number), source: pattern.id.to_s,
            lineup: pattern.lineup(number), level_percent: percent,
            hp_percent: percent, attack_percent: percent }
        else
          band = biome.bands.find { |entry| (entry.from..entry.to).cover?(number) }
          lineups = pools.fetch(band.pool).lineups
          digest = Digest::SHA256.digest("#{biome.id}/#{number}")
          index = digest.unpack1("Q>") % lineups.size
          { floor: number, id: floor_id(biome.id, number), source: band.pool.to_s,
            lineup: lineups[index], level_percent: percent_for(number, biome.floors),
            hp_percent: percent_for(number, biome.floors),
            attack_percent: percent_for(number, biome.floors) }
        end
      end
    end

    private

    def scale_percent(pattern, floor, total_floors)
      pattern ? pattern.scale_percent(floor, total_floors) : percent_for(floor, total_floors)
    end

    def percent_for(floor, total_floors)
      step = ((floor - 1) / 5) + 1
      total_steps = (total_floors.to_f / 5).ceil
      (step * 100.0 / total_steps).ceil
    end

    def floor_id(biome_id, number)
      format("%s/%03d", biome_id, number)
    end
  end
end
