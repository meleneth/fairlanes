#include "party_loop_ctx.hpp"

#include <entt/entt.hpp>

#include "fl/context.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/random_hub.hpp"

namespace fl::fsm {

fl::context::PartyCtx PartyLoopCtx::party_context() const {
  // This returns your reference-y PartyCtx by value,
  // internally binding refs to these pointers.
  return fl::context::PartyCtx{*reg_, *rng_, *account_, *party_};
}

entt::entity PartyLoopCtx::self_() const { return party_->party_id_; }

PartyLoopCtx::PartyLoopCtx(fl::context::PartyCtx &ctx)
    : reg_(&ctx.reg()), rng_(&ctx.rng_), account_(&ctx.account_data_),
      party_(ctx.party_data_), log_(ctx.log()) {}

} // namespace fl::fsm
