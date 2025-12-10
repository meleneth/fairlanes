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
  entt::entity party_id_;
  std::shared_ptr<fl::widgets::FancyLog> log_;

  PartyBus bus_;

  PartyData(entt::entity party_id)
      : party_id_(party_id), log_(std::make_shared<fl::widgets::FancyLog>()) {}

  PartyData(PartyData &&) noexcept = default;
  PartyData &operator=(PartyData &&) noexcept = default;

  PartyData(const PartyData &) = delete;
  PartyData &operator=(const PartyData &) = delete;
};

} // namespace fl::primitives
