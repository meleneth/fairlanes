// tests/test_grand_central.cpp

#include <catch2/catch_test_macros.hpp>
#include <entt/entt.hpp>

#include "fl/context.hpp"
#include "fl/ecs/components/closet.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/grand_central.hpp"
#include "fl/primitives/party_data.hpp"

#include <catch2/catch_test_macros.hpp>
#include <entt/entt.hpp>

namespace Catch {
template <> struct StringMaker<entt::entity> {
  static std::string convert(entt::entity value) {
    using underlying = std::underlying_type_t<entt::entity>;
    auto id = static_cast<underlying>(value);
    if (value == entt::null) {
      return std::string{"entt::null"};
    }
    return std::string{"entt::entity("} + std::to_string(id) + ")";
  }
};
} // namespace Catch

TEST_CASE("GrandCentral basic structure is initialized",
          "[grand_central][smoke]") {
  // 2 accounts, 3 parties per account
  fl::GrandCentral gc{2, 3, 1};

  // We expect exactly 2 accounts
  // REQUIRE(gc.accounts_.size() == 2);

  for (std::size_t a = 0; a < gc.accounts().size(); ++a) {
    auto &acc = gc.accounts()[a];

    // And exactly 3 parties
    REQUIRE(acc.parties().size() == 3);

    for (std::size_t p = 0; p < acc.parties().size(); ++p) {
      auto &party = acc.parties()[p];

      // Each party should also have a valid entity
      // REQUIRE(party.party_id_ != entt::null);
    }
  }
}

TEST_CASE("GrandCentral context helpers work",
          "[grand_central][context][smoke]") {
  fl::GrandCentral gc{1, 2, 1}; // 1 account, 2 parties

  // Account context
  auto acc_ctx = gc.account_context(0);

  // Registry and RNG should be usable
  entt::entity ent = acc_ctx.reg().create();
  // REQUIRE(ent != entt::null);
  // Account should own at least one party
  REQUIRE(acc_ctx.account_data().parties().size() == 2);

  // Party context
  auto party_ctx = acc_ctx.party_context(0);
  // REQUIRE(party_ctx.party.party_id_);

  // EntityCtx from PartyCtx
  auto entity_ctx = party_ctx.entity_context(party_ctx.party_data().party_id());
  // REQUIRE(entity_ctx.self == party_ctx.party.party_id_);

  // Smoke test logging + bus usage — we don't assert behavior, just that it
  // compiles and runs Log: append something trivial (assuming FancyLog has an
  // Append-like API or operator()) If your API is different, adjust this block
  // accordingly. For now we just ensure the log reference is obtainable.
}

TEST_CASE("GrandCentral creates each player with a gear closet",
          "[grand_central][closet]") {
  fl::GrandCentral gc{2, 3, 5};
  auto &reg = gc.reg();

  for (auto &account : gc.accounts()) {
    for (auto &party : account.parties()) {
      for (auto &member_data : party.members()) {
        auto member = member_data.member_id();
        REQUIRE(reg.valid(member));

        auto *party_member =
            reg.try_get<fl::ecs::components::PartyMember>(member);
        REQUIRE(party_member != nullptr);

        auto closet = party_member->closet_entity_id();
        const bool has_closet = closet != entt::null;
        REQUIRE(has_closet);
        REQUIRE(reg.valid(closet));
        REQUIRE(reg.all_of<fl::ecs::components::Closet>(closet));
      }
    }
  }
}
