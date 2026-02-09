#pragma once

#include <entt/entt.hpp>

namespace fl::context {
struct PartyCtx;
}

namespace fl::widgets {
class FancyLog;
}

namespace fl::primitives {
struct RandomHub;
struct AccountData;
struct PartyData;
} // namespace fl::primitives

namespace fl::fsm {

struct PartyLoopCtx {
public:
  explicit PartyLoopCtx(fl::context::PartyCtx &ctx);

  entt::registry &reg() const { return *reg_; }
  fl::primitives::RandomHub &rng() const { return *rng_; }
  fl::primitives::AccountData &account_data() const { return *account_data_; }
  fl::primitives::PartyData &party() const { return *party_; }
  fl::widgets::FancyLog &log() const { return *log_; }

  entt::entity self() const;
  fl::context::PartyCtx party_context() const;

private:
  entt::registry *reg_{};
  fl::primitives::RandomHub *rng_{};
  fl::primitives::AccountData *account_data_{};
  fl::primitives::PartyData *party_{};
  fl::widgets::FancyLog *log_{};
};

} // namespace fl::fsm
