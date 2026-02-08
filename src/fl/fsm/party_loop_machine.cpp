#include "fl/fsm/party_loop_machine.hpp"
#include "fl/fsm/party_loop.hpp"
#include "fl/fsm/party_loop_ctx.hpp"

// include boost::sml + your event types here
#include <boost/sml.hpp>

namespace fl::fsm {

struct PartyLoopMachine::Impl {
  PartyLoopCtx *ctx{};
  boost::sml::sm<fl::fsm::PartyLoop> sm_;
  explicit Impl(PartyLoopCtx &c) : ctx(&c), sm_(c) {}
};

PartyLoopMachine::PartyLoopMachine(PartyLoopCtx &ctx)
    : impl_(std::make_unique<Impl>(ctx)) {}

PartyLoopMachine::~PartyLoopMachine() = default;
PartyLoopMachine::PartyLoopMachine(PartyLoopMachine &&) noexcept = default;
PartyLoopMachine &
PartyLoopMachine::operator=(PartyLoopMachine &&) noexcept = default;

void PartyLoopMachine::start(std::string party_name) {
  // impl_->ctx->... maybe store name
  (void)party_name;
  // impl_->sm.process_event(/* ev_start{std::move(party_name)} */);
}

void PartyLoopMachine::on_beat() { impl_->sm_.process_event(seerin::Beat{}); }

void PartyLoopMachine::dispatch_party_bus() {
  // optional if you have pumping logic that is currently in IsParty
}

} // namespace fl::fsm
