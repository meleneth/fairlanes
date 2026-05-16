# Event Flow

Back to [Tour Home](./index.md)

For the complete current event inventory, see [Event Reference](./event-reference.md).

## Request / operation flow

Use this page to explain one representative path through the system.

Questions this page should answer:

- what starts the operation?
- what objects coordinate it?
- where is state read?
- where is state written?
- what side effects happen?

## Sequence

```mermaid
sequenceDiagram
  participant U as User / Caller
  participant E as Entrypoint
  participant S as Service
  participant D as Domain
  participant I as Infrastructure

  U->>E: invoke operation
  E->>S: translate / dispatch
  S->>D: execute core logic
  D->>I: read/write external state
  I-->>D: result
  D-->>S: outcome
  S-->>E: response
  E-->>U: final result
```

## What belongs here

A good page here usually names real classes, real files, and real invariants.
This should become the "trace one thing end to end" page.
