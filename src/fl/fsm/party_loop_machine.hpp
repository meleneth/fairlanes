#pragma once
#include <memory>
#include <string>

namespace fl::fsm {
struct PartyLoopCtx;
}

namespace fl::fsm {

class PartyLoopMachine {
public:
  explicit PartyLoopMachine(PartyLoopCtx &ctx);
  ~PartyLoopMachine();

  PartyLoopMachine(PartyLoopMachine &&) noexcept;
  PartyLoopMachine &operator=(PartyLoopMachine &&) noexcept;

  PartyLoopMachine(const PartyLoopMachine &) = delete;
  PartyLoopMachine &operator=(const PartyLoopMachine &) = delete;

  // minimal surface to match what IsParty currently does
  void start(std::string party_name);
  void on_beat();            // or on_beat(const seerin::BeatEvent&) etc
  void dispatch_party_bus(); // optional: if you have a pump step

  // add verbs as needed later:
  // void member_added(entt::entity);
  // void request_action(...);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace fl::fsm
