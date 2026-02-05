// grand_central.hpp
#pragma once

#include <entt/entt.hpp>
#include <vector>

#include "fl/fwd.hpp"

#include "fl/combat/party_bus.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/fsm/party_loop_ctx.hpp"
#include "member_data.hpp"
#include "sr/beat_bus.hpp"

namespace fl::primitives {

struct PartyData {
  entt::entity party_id_;
  std::shared_ptr<fl::widgets::FancyLog> log_;
  std::deque<fl::primitives::MemberData> members_;
  fl::events::PartyBus party_bus_;

  void init_party(fl::fsm::PartyLoopCtx &party_loop_ctx, std::string name);

  PartyData(entt::entity party_id);

  PartyData(PartyData &&) noexcept = default;
  PartyData &operator=(PartyData &&) noexcept = default;

  PartyData(const PartyData &) = delete;
  PartyData &operator=(const PartyData &) = delete;

  void hook_to_beat(seerin::BeatBus &gc_beat_bus);

  seerin::BeatBus &beat_bus() { return beat_bus_; }
  const seerin::BeatBus &beat_bus() const { return beat_bus_; }

private:
  seerin::BeatBus beat_bus_{};
  seerin::BeatSub beat_sub_{};
};

} // namespace fl::primitives
