#pragma once
#include <memory>
#include <string>

namespace fl::context {
struct PartyCtx;

} // namespace fl::context

namespace fl::fsm {

class PartyLoopMachine {
public:
  explicit PartyLoopMachine(fl::context::PartyCtx &ctx);
  ~PartyLoopMachine();

  PartyLoopMachine(PartyLoopMachine &&) noexcept;
  PartyLoopMachine &operator=(PartyLoopMachine &&) noexcept;

  PartyLoopMachine(const PartyLoopMachine &) = delete;
  PartyLoopMachine &operator=(const PartyLoopMachine &) = delete;

  void start(std::string party_name);
  void on_beat();
  void dispatch_party_bus();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace fl::fsm
