#pragma once
#include "atb_events.hpp"
#include "variant_bus.hpp"

namespace seerin {
using AtbInBus = VariantBus<AtbInEvent>;
using AtbOutBus = VariantBus<AtbOutEvent>;
} // namespace seerin
