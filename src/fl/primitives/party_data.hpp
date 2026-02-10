// party_data.hpp
#pragma once

#include <deque>
#include <functional> // std::invoke
#include <memory>
#include <string>
#include <utility>

#include <entt/entt.hpp>

#include "fl/context.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/fsm/party_loop_machine.hpp"
#include "fl/fwd.hpp"
#include "fl/primitives/encounter_data.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "member_data.hpp"
#include "sr/beat_bus.hpp"

namespace fl::primitives {

struct PartyData {
public:
  explicit PartyData(entt::entity party_id,
                     fl::context::AccountCtx &account_ctx, std::string name);

  PartyData(PartyData &&) noexcept = default;
  PartyData &operator=(PartyData &&) noexcept = default;

  PartyData(const PartyData &) = delete;
  PartyData &operator=(const PartyData &) = delete;

  // ---- accessors ----
  entt::entity party_id() const noexcept { return party_id_; }

  fl::widgets::FancyLog &log() const { return *log_; }
  bool has_log() const noexcept { return static_cast<bool>(log_); }

  std::deque<fl::primitives::MemberData> &members() noexcept {
    return members_;
  }
  const std::deque<fl::primitives::MemberData> &members() const noexcept {
    return members_;
  }

  fl::primitives::EncounterData &encounter_data() { return *encounter_data_; }
  const fl::primitives::EncounterData &encounter_data() const {
    return *encounter_data_;
  }

  fl::events::PartyBus &party_bus() noexcept { return party_bus_; }
  const fl::events::PartyBus &party_bus() const noexcept { return party_bus_; }

  seerin::BeatBus &party_beat_bus() noexcept { return party_beat_bus_; }
  const seerin::BeatBus &party_beat_bus() const noexcept {
    return party_beat_bus_;
  }

  fl::context::PartyCtx &loop_ctx() noexcept { return party_ctx_; }
  const fl::context::PartyCtx &loop_ctx() const noexcept { return party_ctx_; }

  fl::fsm::PartyLoopMachine &loop_machine() { return *party_loop_machine_; }
  const fl::fsm::PartyLoopMachine &loop_machine() const {
    return *party_loop_machine_;
  }
  EncounterData &create_encounter();
  bool has_encounter() const noexcept { return encounter_data_ != nullptr; }
  bool in_combat() const noexcept {
    return encounter_data_ != nullptr && !encounter_data_->is_over();
  }
  // ---- behavior ----
  void hook_to_beat(seerin::BeatBus &gc_beat_bus);

  bool needs_town() { return false; }

  void add_party_member(entt::entity member) { (void)member; }

  template <class Fn> void for_each_member(Fn &&fn) const {
    for (const auto &member : members_) {
      std::invoke(std::forward<Fn>(fn), member.member_id());
    }
  }

private:
  entt::entity party_id_{entt::null};
  std::string name_;
  entt::entity account_id_{entt::null};
  std::unique_ptr<fl::widgets::FancyLog> log_;
  fl::context::PartyCtx party_ctx_;
  std::unique_ptr<fl::fsm::PartyLoopMachine> party_loop_machine_;
  std::unique_ptr<fl::primitives::EncounterData> encounter_data_{nullptr};
  std::deque<fl::primitives::MemberData> members_;
  fl::events::PartyBus party_bus_;

  seerin::BeatBus party_beat_bus_{};
  seerin::BeatSub gc_forward_sub_{};
};

} // namespace fl::primitives
