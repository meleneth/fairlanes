#pragma once
// INTERNAL: Do not include from engine code.

#include <deque>
#include <entt/entt.hpp>

#include "fl/account_data.hpp"
#include "fl/context.hpp"
#include "fl/party_data.hpp"
#include "fl/random_hub.hpp"

namespace fl {

class GrandCentral {
public:
  entt::registry reg;
  fl::RandomHub rng;
  std::deque<fl::AccountData> accounts_;

  // --------------------------------------------------------------------------
  // Constructor: bootstrap the world structure
  // --------------------------------------------------------------------------
  GrandCentral(uint8_t num_accounts = 1, uint8_t parties_per_account = 1) {

    for (uint8_t a = 0; a < num_accounts; ++a) {
      // ---- create account entry --------------------------------------------
      accounts_.emplace_back();
      auto &acc = accounts_.back();

      acc.account_entity = reg.create();

      // ---- create party entries --------------------------------------------
      for (int p = 0; p < parties_per_account; ++p) {
        acc.parties.emplace_back();
        auto &pty = acc.parties.back();

        pty.party_id_ = reg.create();

        // If parties should have a Party component, add here:
        // reg.emplace<PartyTag>(pty.party_entity);
      }

      // If accounts should have an Account component, add here:
      // reg.emplace<AccountTag>(acc.account_entity);
    }
  }

  // --------------------------------------------------------------------------
  // Context builders
  // --------------------------------------------------------------------------
  fl::context::AccountCtx account_context(std::size_t index) {
    return {reg, rng, accounts_.at(index)};
  }

  fl::context::PartyCtx party_context(std::size_t account_index,
                                      std::size_t party_index) {
    auto &acc = accounts_.at(account_index);
    auto &pty = acc.parties.at(party_index);
    return {reg, rng, acc, pty};
  }
};

} // namespace fl
