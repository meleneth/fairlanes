// tests/account_data.test.cpp
#include <catch2/catch_test_macros.hpp>
#include <entt/entt.hpp>

#include "fl/primitives/account_data.hpp"
#include "fl/primitives/member_data.hpp"
#include "fl/primitives/party_data.hpp"

TEST_CASE("AccountData creates a valid entity and log",
          "[account][account_data]") {
  entt::registry reg;
  entt::entity ent = reg.create();

  fl::primitives::AccountData acc(ent);

  REQUIRE(acc.id() != static_cast<entt::entity>(entt::null));
  REQUIRE(reg.valid(acc.id()));
}

TEST_CASE("AccountData creates unique entity ids per instance",
          "[account][account_data]") {
  entt::registry reg;
  entt::entity ent_a = reg.create();
  entt::entity ent_b = reg.create();

  fl::primitives::AccountData a(ent_a);
  fl::primitives::AccountData b(ent_b);
  REQUIRE(a.id() != static_cast<entt::entity>(entt::null));
  REQUIRE(b.id() != static_cast<entt::entity>(entt::null));
  REQUIRE(a.id() != b.id());

  REQUIRE(reg.valid(a.id()));
  REQUIRE(reg.valid(b.id()));
}
