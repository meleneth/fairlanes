#include <catch2/catch_test_macros.hpp>
#include <entt/entt.hpp>

#include "fl/events/ready_queue.hpp"

using fl::events::Decision;
using fl::events::ReadyQueue;

TEST_CASE("ReadyQueue preserves FIFO order", "[ready_queue]") {
  ReadyQueue q;

  entt::entity actor1 = entt::entity{1};
  entt::entity actor2 = entt::entity{2};
  entt::entity actor3 = entt::entity{3};

  q.push(Decision{actor1, entt::null, entt::null, 1});
  q.push(Decision{actor2, entt::null, entt::null, 2});
  q.push(Decision{actor3, entt::null, entt::null, 3});

  REQUIRE_FALSE(q.empty());
  REQUIRE(q.size() == 3);

  REQUIRE(q.front().actor == actor1);
  REQUIRE(q.front().nonce == 1);

  q.pop_front();
  REQUIRE(q.size() == 2);
  REQUIRE(q.front().actor == actor2);
  REQUIRE(q.front().nonce == 2);

  q.pop_front();
  REQUIRE(q.size() == 1);
  REQUIRE(q.front().actor == actor3);
  REQUIRE(q.front().nonce == 3);

  q.pop_front();
  REQUIRE(q.empty());
}

TEST_CASE("ReadyQueue allows interleaved push/pop", "[ready_queue]") {
  ReadyQueue q;

  entt::entity actor1 = entt::entity{10};
  entt::entity actor2 = entt::entity{20};

  q.push(Decision{actor1, entt::null, entt::null, 42});
  REQUIRE(q.front().actor == actor1);

  q.pop_front();
  REQUIRE(q.empty());

  q.push(Decision{actor2, entt::null, entt::null, 99});
  REQUIRE_FALSE(q.empty());
  REQUIRE(q.front().actor == actor2);
  REQUIRE(q.front().nonce == 99);
}
