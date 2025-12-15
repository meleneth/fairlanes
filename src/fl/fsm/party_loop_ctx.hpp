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

  PartyLoopCtx(fl::context::PartyCtx &ctx);
  entt::registry &reg() const { return *reg_; }
  fl::widgets::FancyLog &log() const { return log_; };
  fl::primitives::RandomHub &rng() const { return *rng_; }
  entt::entity self_() const;

  fl::context::PartyCtx party_context() const;
};

} // namespace fl::fsm
