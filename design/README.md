# Fairlanes canonical game design

This directory is Fairlanes' canonical game-design authority, informally the
**Word of God**. It records explicitly accepted gameplay intent. Gameplay
implementation should follow these documents unless an explicit design change
is approved.

Canonical intent and implementation status are separate. These labels are used
throughout this directory:

- **Decided:** accepted design, whether or not it exists in the game yet.
- **Implemented:** current behavior satisfies the accepted design.
- **Planned:** accepted design that is not yet implemented.
- **Unresolved:** another explicit design decision is required.

Current code, declarations, tests, and generated reports are evidence of
implementation; they are not proof of intended design. Generated documentation
describes the declarations or implementation that produced it and does not
override this directory.

## Document ownership

Each canonical rule has one primary home. Other documents should link to that
home rather than restating a divergent copy.

- [Characters](characters.md): character identity, role formation, and skill
  availability.
- [Progression](progression.md): levels, XP, and progression philosophy.
- [Parties and recruitment](parties-and-recruitment.md): roster structure,
  recruitment milestones, and event operation.
- [Combat](combat.md): established combat-design principles and teaching roles.
- [Cycles and biomes](cycles-and-biomes.md): cycle and tower structure, biome
  relationships, and cross-biome gates.

`brainstorms/` is exploratory. It may contain competing proposals, balance
hypotheses, calculations, experiments, rejected ideas, and open questions.
Brainstorms may contradict one another and never override canonical design.

## Changing canonical design

Canonical decisions can evolve, but the change must be deliberate:

1. Identify the existing decision.
2. Explain the proposed change and its consequences.
3. Obtain explicit design approval.
4. Update the primary canonical document.
5. Update dependent design documentation.
6. Implement and test the change separately when appropriate.

Do not silently change canonical rules during refactoring, bug fixing, or
balancing.

## Agent behavior

Coding agents may identify gaps, report contradictions, suggest alternatives,
conduct experiments, and document unresolved questions. They must not invent
canonical gameplay rules, promote brainstorm material without approval, treat
implementation as proof of intent, weaken a decision because it is difficult to
implement, or present speculative balance numbers as requirements.

This is intentionally a lightweight convention. The directory and explicit
approval are the governance mechanism; no additional ceremony is required.
