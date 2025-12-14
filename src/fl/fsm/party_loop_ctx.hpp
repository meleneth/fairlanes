// fl/fsm/party_loop_ctx.hpp
#pragma once

#include <entt/entt.hpp>

#include "fl/context.hpp" // for PartyCtx
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/random_hub.hpp"

namespace fl::fsm {

struct PartyLoopCtx {
  entt::registry *reg_{};
  fl::primitives::RandomHub *rng_{};
  fl::primitives::AccountData *account_{};
  fl::primitives::PartyData *party_{};
  fl::widgets::FancyLog &log_;

  entt::registry &reg() const { return *reg_; }

  entt::entity self_() const { return party_->party_id_; }

  fl::context::PartyCtx party_ctx() const;
};

} // namespace fl::fsm
