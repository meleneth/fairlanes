# Grand Central

[GrandCentral](https://github.com/meleneth/fairlanes/blob/89571fb5f3e6396d5e191a557602c2b25c79a31c/src/fl/grand_central.hpp#L73) is the **Big God Object**.

It owns everything.

## The rule

Nobody is allowed to know its name.

We do not enforce this technically. You *could* reach for it.  
We simply do not do that anywhere in the codebase.

The only place that directly interacts with `GrandCentral` is the [main function](https://github.com/meleneth/fairlanes/blob/89571fb5f3e6396d5e191a557602c2b25c79a31c/src/fairlanes.cpp#L7).

## What it actually does

- Owns the world state
- Owns top-level systems
- Drives the [main loop](https://github.com/meleneth/fairlanes/blob/89571fb5f3e6396d5e191a557602c2b25c79a31c/src/fl/grand_central.cpp#L139)

It is the root of reality.

And then we pretend it does not exist.

## Why this exists

If `GrandCentral` were passed around freely, everything would depend on everything.

That leads to:
- invisible coupling
- untestable systems
- “just grab it from the god object” shortcuts
- loss of architectural boundaries

Instead, Fairlanes forces all code to operate through **context objects**.

See: [Contexts](./context.md)

## What that gets us

- Systems declare their scope via context types
- Dependencies are explicit instead of ambient
- Unit tests can construct minimal worlds without bootstrapping everything
- Call sites remain readable and constrained

## The design tension

`GrandCentral` still exists because:

- something has to own the world
- something has to drive the main loop

But by **refusing to pass it around**, we separate:

- *ownership* (centralized)
- *access* (scoped via contexts)

## Fairlanes take

We allow a god object to exist.

We do not allow it to be used.

## Implication for contributors

If you feel the urge to reach for `GrandCentral`:

> you probably need a better context instead
