// grand_central.hpp
#pragma once

#include <entt/entt.hpp>
#include <vector>

#include "fl/fwd.hpp"

#include "fl/events/party_bus.hpp"
#include "member_data.hpp"
namespace fl::primitives {

struct PartyData {
  entt::entity party_id_;
  std::shared_ptr<fl::widgets::FancyLog> log_;
  std::deque<fl::primitives::MemberData> members_;
  fl::events::PartyBus bus_;
  // Assuming your AccountCtx can provide PartyCtx for this PartyData:

  PartyData(fl::context::AccountCtx &ctx, std::string name);

  PartyData(PartyData &&) noexcept = default;
  PartyData &operator=(PartyData &&) noexcept = default;

  PartyData(const PartyData &) = delete;
  PartyData &operator=(const PartyData &) = delete;
};

} // namespace fl::primitives
