// fl/ecs/components/encounter.hpp
#pragma once

#include <memory>
#include <utility>

#include <entt/entt.hpp>

#include "fl/primitives/encounter_data.hpp" // path to your EncounterData/InEncounter decl

namespace fl::ecs::components {

struct Encounter {
public:
  Encounter(entt::entity encounter_id,
            fl::primitives::EncounterData *encounter_data)
      : encounter_id_{encounter_id}, encounter_data_{encounter_data} {}

  Encounter(Encounter &&) noexcept = default;
  Encounter &operator=(Encounter &&) noexcept = default;

  Encounter(const Encounter &) = delete;
  Encounter &operator=(const Encounter &) = delete;

  // ---- identity ----
  entt::entity encounter_id() const noexcept { return encounter_id_; }

  fl::primitives::EncounterData &encounter_data() { return *encounter_data_; }
  const fl::primitives::EncounterData &encounter_data() const {
    return *encounter_data_;
  }

  fl::primitives::EncounterData *encounter_data_ptr() noexcept {
    return encounter_data_;
  }
  const fl::primitives::EncounterData *encounter_data_ptr() const noexcept {
    return encounter_data_;
  }

private:
  entt::entity encounter_id_{entt::null};
  fl::primitives::EncounterData *encounter_data_ = nullptr;
};

} // namespace fl::ecs::components
