# Contexts

## What it is

Fairlanes uses small context types to define **what a piece of code is allowed to see**.

At the API level, they read like crisp bundles of references: `reg()`, `rng()`, `log()`, `party_data()`, `attacker()`, and so on.

At the representation level, they are mostly stored as **raw pointers** and exposed as **reference-returning accessors**.

That combination is intentional.

It gives us:
- lightweight, copyable context objects
- clean callsites that work with references instead of nullable pointers
- explicit construction boundaries
- scope that is visible in the type, not buried in comments or argument soup

These are not “service locator lite.” They are scoped views over the world.

## Where we use it

- `EntityCtx`
- `BuildCtx`
- `PartyCtx`
- `AccountCtx`
- `AttackCtx`
- `WorldCoreCtx`

## Shared spine

Most Fairlanes contexts expose the same core world access:

| Capability | Meaning |
|------------|---------|
| `reg()` | ECS world access |
| `rng()` | randomness for gameplay and simulation |
| `log()` | human-facing trace / debugging surface |

## Context-specific surface

| Context | Special access | Why it exists |
|---------|----------------|---------------|
| `EntityCtx` | `self()` | entity-local work |
| `BuildCtx` | none beyond the shared spine | setup / construction before an entity-bound scope exists |
| `PartyCtx` | `account_data()`, `party_data()`, `bus()`, `self()` | party-scoped gameplay work and party-level event flow |
| `AccountCtx` | `account_data()`, plus `party_context(...)` and `entity_context(...)` | account-level navigation into narrower scopes |
| `AttackCtx` | `attacker()`, `defender()`, `damage()` | transient combat action state |
| `WorldCoreCtx` | concept only | generic code that just needs the common world surface |

## What to notice

### Pointer-backed, reference-shaped

Contexts store raw pointers internally but expose references externally.

This means:
- storage is flexible and cheap
- usage is clean and assumes valid invariants

## What that gets us

- Types communicate scope clearly
- Call sites stay readable
- Construction-time flexibility does not leak into usage
- Scope widening is visible and reviewable

## Fairlanes take

Fairlanes contexts encode authority in types while keeping usage ergonomic.

## What this means when adding code

- Use the narrowest context possible
- Prefer deriving narrower contexts
- Treat pointer storage as implementation detail
- Target `WorldCoreCtx` when only core capabilities are needed
