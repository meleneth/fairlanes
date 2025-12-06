// grand_central.hpp
#pragma once

#include <entt/entt.hpp>
#include <vector>

#include "fl/bus_types.hpp"
#include "fl/random_hub.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl {

struct PartyData {
  entt::entity party_id_{entt::null};
  fl::widgets::FancyLog log_;
  PartyBus bus_;

  PartyData() = default;

  // move-only is fine, vector will use move
  PartyData(PartyData &&) noexcept = default;
  PartyData &operator=(PartyData &&) noexcept = default;

  PartyData(const PartyData &) = delete;
  PartyData &operator=(const PartyData &) = delete;
};

} // namespace fl
