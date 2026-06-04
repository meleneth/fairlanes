#pragma once

#include <cstdint>

namespace fl::ecs::components {

// Written back each ATB beat from EncounterData so widgets can read charge
// without touching AtbEngine internals.
struct AtbCharge {
  int64_t charge{0};
  int64_t max_charge{4800};
  int64_t charge_per_beat{80};
};

} // namespace fl::ecs::components
