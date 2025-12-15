#pragma once
#include <boost/sml.hpp>
#include <string>

#include "fl/fsm/party_loop.hpp"
#include "fl/fwd.hpp"

namespace fl::ecs::components {

namespace sml = boost::sml;

// Marks an entity as a Party (party itself is an entity)

struct IsParty {
  fl::fsm::PartyLoopCtx ctx_;
  fl::fsm::PartyLoop machine_;
  sml::sm<fl::fsm::PartyLoop> sm_;
  entt::entity account_;
  entt::entity self_;
  std::string name_;
  std::vector<entt::entity> party_members_;

  IsParty(fl::fsm::PartyLoopCtx context, std::string name,
          entt::entity account);
  void next();
  entt::entity create_member(std::string name);
  bool needs_town();
  bool in_combat();

  // Call `fn(entt::handle)` for each member of this party
  template <typename PM = PartyMember, typename Fn>
  inline void for_each_member(Fn &&fn) {
    for (auto e : party_members_) {
      fn(e);
    }
  }
};
} // namespace fl::ecs::components
