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
      overrides = biome.overrides.to_h { |entry| [entry.floor, entry] }
      (1..biome.floors).map do |number|
        override = overrides[number]
        if override
          { floor: number, id: floor_id(biome.id, number), source: "override",
            lineup: override.lineup }
        else
          band = biome.bands.find { |entry| (entry.from..entry.to).cover?(number) }
          lineups = pools.fetch(band.pool).lineups
          digest = Digest::SHA256.digest("#{biome.id}/#{number}")
          index = digest.unpack1("Q>") % lineups.size
          { floor: number, id: floor_id(biome.id, number), source: band.pool.to_s,
            lineup: lineups[index] }
        end
      end
    end

    private

    def floor_id(biome_id, number)
      format("%s/%03d", biome_id, number)
    end
  end
end
