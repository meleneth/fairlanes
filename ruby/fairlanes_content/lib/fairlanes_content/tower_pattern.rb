# frozen_string_literal: true

module FairlanesContent
  class TowerPattern
    COMPOSITIONS = [
      [0, 0, 0, 0, 0],
      [0, 0, 0, 0, 1],
      [0, 0, 0, 1, 1],
      [0, 0, 1, 1, 2],
      [0, 1, 2, 3, 4]
    ].freeze

    attr_reader :id, :monsters

    def initialize(id:, monsters:)
      @id = id
      @monsters = monsters
    end

    def lineup(floor)
      COMPOSITIONS.fetch((floor - 1) % COMPOSITIONS.size).map do |index|
        monsters.fetch([index, monsters.size - 1].min)
      end
    end

    def scale_percent(floor, total_floors)
      step = ((floor - 1) / COMPOSITIONS.size) + 1
      total_steps = (total_floors.to_f / COMPOSITIONS.size).ceil
      (step * 100.0 / total_steps).ceil
    end
  end
end
