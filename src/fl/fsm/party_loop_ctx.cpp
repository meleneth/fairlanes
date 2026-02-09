#include "fl/fsm/party_loop_ctx.hpp"

#include "fl/context.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/random_hub.hpp"
#include "fl/widgets/fancy_log.hpp" // if FancyLog is a concrete class header

namespace fl::fsm {

PartyLoopCtx::PartyLoopCtx(fl::context::PartyCtx &ctx) {
  // Adjust these to whatever PartyCtx exposes.
  reg_ = &ctx.reg();
  rng_ = &ctx.rng();
  account_data_ = &ctx.account_data();
  party_ = &ctx.party_data();
  log_ = &ctx.log();
}

entt::entity PartyLoopCtx::self() const { return party().party_id(); }

fl::context::PartyCtx PartyLoopCtx::party_context() const {
  // Adjust to PartyCtx's constructor/factory.
  return fl::context::PartyCtx{reg(), rng(), account_data(), party()};
}

} // namespace fl::fsm
