// fl/ecs/components/is_account.hpp
#pragma once
#include <entt/entity/entity.hpp>

#include "fl/primitives/account_data.hpp"

namespace fl::widgets {
class FancyLog;
}

namespace fl::ecs::components {

struct IsAccount {
  explicit IsAccount(entt::entity self, fl::widgets::FancyLog &log,
                     fl::primitives::AccountData &account_data)
      : self_(self), log_(&log), account_data_(&account_data) {}

  entt::entity self() const { return self_; }

  fl::widgets::FancyLog &log() const { return *log_; }

  fl::primitives::AccountData &account_data() const { return *account_data_; }

private:
  entt::entity self_{entt::null};
  fl::widgets::FancyLog *log_{nullptr};
  fl::primitives::AccountData *account_data_{nullptr};
};

} // namespace fl::ecs::components
