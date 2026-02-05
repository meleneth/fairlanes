#pragma once
#include "sr/atb_events.hpp"
#include "sr/paired_bus.hpp"
#include "sr/variant_bus.hpp"
#include <variant>


namespace fl::combat {

using PartyInEvent = std::variant<seerin::Beat>;

// Start small, expand later.
using PartyOutEvent =
    std::variant<seerin::BecameReady /*, seerin::ApplyEffect, ... */>;

using PartyInBus = seerin::VariantBus<PartyInEvent>;
using PartyOutBus = seerin::VariantBus<PartyOutEvent>;

using PartyBus = seerin::PairedBus<PartyInBus, PartyOutBus>;

} // namespace fl::combat
