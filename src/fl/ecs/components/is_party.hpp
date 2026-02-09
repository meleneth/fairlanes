#pragma once
#include <string>
#include <vector>

#include <entt/entt.hpp>

#include "fl/fwd.hpp"

namespace fl::ecs::components {

// Marks an entity as a Party (party itself is an entity)
struct IsParty {
  entt::entity party_id_{entt::null};
  fl::primitives::PartyData *party_data_{nullptr};
  std::vector<entt::entity> party_members_;

  explicit IsParty(entt::entity party_id, fl::primitives::PartyData &party_data)
      : party_id_(party_id), party_data_(&party_data) {}

  template <typename Fn> inline void for_each_member(Fn &&fn) const {
    for (auto e : party_members_) {
      fn(e);
    }
  }

  fl::primitives::PartyData &party_data() const {
    // FL_ASSERT(party_data_);
    return *party_data_;
  }
};

} // namespace fl::ecs::components
