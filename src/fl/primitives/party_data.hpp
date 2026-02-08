#pragma once

#include <deque>
#include <memory>
#include <string>

#include <entt/entt.hpp>

#include "fl/events/party_bus.hpp"
#include "fl/fsm/party_loop_ctx.hpp"
#include "fl/fwd.hpp"
#include "member_data.hpp"
#include "sr/beat_bus.hpp"


namespace fl::primitives {

struct PartyData {
public:
  explicit PartyData(entt::entity party_id);

  PartyData(PartyData &&) noexcept = default;
  PartyData &operator=(PartyData &&) noexcept = default;

  PartyData(const PartyData &) = delete;
  PartyData &operator=(const PartyData &) = delete;

  // ---- accessors ----
  entt::entity party_id() const noexcept { return party_id_; }

  fl::widgets::FancyLog &log() const { return *log_; } // assumes initialized
  void set_log(std::unique_ptr<fl::widgets::FancyLog> log) {
    log_ = std::move(log);
  }
  bool has_log() const noexcept { return static_cast<bool>(log_); }

  std::deque<fl::primitives::MemberData> &members() noexcept {
    return members_;
  }
  const std::deque<fl::primitives::MemberData> &members() const noexcept {
    return members_;
  }

  fl::events::PartyBus &party_bus() noexcept { return party_bus_; }
  const fl::events::PartyBus &party_bus() const noexcept { return party_bus_; }

  seerin::BeatBus &party_beat_bus() noexcept { return party_beat_bus_; }
  const seerin::BeatBus &party_beat_bus() const noexcept {
    return party_beat_bus_;
  }

  // ---- behavior ----
  void init_party(fl::fsm::PartyLoopCtx &party_loop_ctx, std::string name);
  void hook_to_beat(seerin::BeatBus &gc_beat_bus);

private:
  entt::entity party_id_{entt::null};

  std::unique_ptr<fl::widgets::FancyLog> log_;
  std::deque<fl::primitives::MemberData> members_;
  fl::events::PartyBus party_bus_;

  seerin::BeatBus party_beat_bus_{};
  seerin::BeatSub gc_forward_sub_{};
};

} // namespace fl::primitives
