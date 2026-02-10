// grand_central.hpp
#pragma once

#include <string>
#include <utility>

#include <entt/entt.hpp>

#include "fl/events/party_bus.hpp"

namespace fl::primitives {

struct MemberData {
public:
  explicit MemberData(entt::entity member_id, std::string name)
      : member_id_{member_id}, name_{std::move(name)} {}

  MemberData(MemberData &&) noexcept = default;
  MemberData &operator=(MemberData &&) noexcept = default;

  MemberData(const MemberData &) = delete;
  MemberData &operator=(const MemberData &) = delete;

  // ---- accessors ----
  entt::entity member_id() const noexcept { return member_id_; }

  const std::string &name() const noexcept { return name_; }

  fl::events::PartyBus &bus() noexcept { return bus_; }
  const fl::events::PartyBus &bus() const noexcept { return bus_; }

private:
  entt::entity member_id_{entt::null};
  std::string name_;

  fl::events::PartyBus bus_;
};

} // namespace fl::primitives
