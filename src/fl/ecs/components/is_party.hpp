#pragma once
#include <boost/sml.hpp>
#include <string>

// #include "fl/ecs/fwd.hpp"
#include "fl/context.hpp"
#include "fl/fsm/party_loop.hpp"

namespace fl::ecs::components {
struct PartyMember;
namespace sml = boost::sml;

// Marks an entity as a Party (party itself is an entity)
using fl::fsm::PartyLoop;

struct IsParty {
  sml::sm<PartyLoop> sm_;
  entt::entity account_;
  entt::entity self_;
  std::string name_;
  std::vector<entt::entity> party_members_;

  IsParty(fl::context::PartyCtx ctx, std::string name, entt::entity account);

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
