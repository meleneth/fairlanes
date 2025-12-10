// grand_central.hpp
#pragma once

#include <entt/entt.hpp>
#include <vector>

#include "fl/events/party_bus.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "logging.hpp"
#include "random_hub.hpp"

namespace fl::primitives {

struct MemberData {
  entt::entity member_id_;
  std::string name_; // TODO WHAT THE FUCK
  std::shared_ptr<fl::widgets::FancyLog> log_;

  PartyBus bus_;

  MemberData(entt::entity member_id, const std::string &name)
      : member_id_(member_id), name_(name),
        log_(std::make_shared<fl::widgets::FancyLog>()) {}

  MemberData(MemberData &&) noexcept = default;
  MemberData &operator=(MemberData &&) noexcept = default;

  MemberData(const MemberData &) = delete;
  MemberData &operator=(const MemberData &) = delete;
};

} // namespace fl::primitives
