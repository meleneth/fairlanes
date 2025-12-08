// grand_central.hpp
#pragma once

#include <entt/entt.hpp>
#include <vector>

#include "fl/events/party_bus.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "logging.hpp"
#include "random_hub.hpp"

namespace fl::primitives {

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

} // namespace fl::primitives
