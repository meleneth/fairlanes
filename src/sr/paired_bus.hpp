// sr/paired_bus.hpp
#pragma once
namespace seerin {
template <class InBus, class OutBus> struct PairedBus {
  InBus in;
  OutBus out;
};
} // namespace seerin
