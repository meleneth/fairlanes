We use [EnTT](https://github.com/skypjack/entt) as our [Entity Component System](../foundations/ecs).

ECS lets us build entities out of small, composable pieces. Instead of large inheritance hierarchies, we attach components to entities and operate on them based on what they have.

In practice, this means:
- we can query for entities that have specific components
- each component holds its own entity-local state
- behavior is driven by systems acting on sets of components

A good serious entry point is [Stats](https://github.com/meleneth/fairlanes/blob/89571fb5f3e6396d5e191a557602c2b25c79a31c/src/fl/ecs/components/stats.hpp#L18), which is one of the most important gameplay components.

A good fun entry point is [Color Override](https://github.com/meleneth/fairlanes/blob/main/src/fl/ecs/components/color_override.hpp), which shows that not every component exists to hold grim combat math.
