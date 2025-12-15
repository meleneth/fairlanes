// grand_central.hpp
#pragma once

#include <entt/entt.hpp>
#include <vector>

#include "fl/fwd.hpp"

#include "fl/events/party_bus.hpp"
#include "fl/fsm/party_loop_ctx.hpp"
#include "member_data.hpp"
namespace fl::primitives {

struct PartyData {
  entt::entity party_id_;
  std::shared_ptr<fl::widgets::FancyLog> log_;
  std::deque<fl::primitives::MemberData> members_;
  fl::events::PartyBus bus_;
  // Assuming your AccountCtx can provide PartyCtx for this PartyData:
  void init_party(fl::fsm::PartyLoopCtx &party_loop_ctx, std::string name);

  PartyData(entt::entity party_id);

  PartyData(PartyData &&) noexcept = default;
  PartyData &operator=(PartyData &&) noexcept = default;

  PartyData(const PartyData &) = delete;
  PartyData &operator=(const PartyData &) = delete;
};

} // namespace fl::primitives
