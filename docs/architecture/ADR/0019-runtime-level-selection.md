# ADR-0019: Introduce Runtime Level Identity and Startup Selection

- Status: Accepted
- Milestone: v0.2.6

## Context

Engine previously loaded `levels/bootstrap.hthlevel` directly. This coupled the
startup policy to one storage identifier and provided no logical identity that
could remain stable independently from Resource paths, serialized bytes, or the
resolved World.

## Decision

Runtime selects a logical Level ID at startup, using `bootstrap` by default or
the value of `--level <id>`. Internal Level Selection validates the exact
`[a-z0-9_-]+` grammar, owns the selected ID, and deterministically maps it to
`"levels/" + ID + ".hthlevel"`. Engine retains the owned selection and passes
only the resulting canonical Resource ID to Resource System.

Level ID, Resource ID, `HTHLevelDescription`, and `HTHWorld` remain distinct.
Resource owns filesystem access, Level owns parsing, and World owns resolved
spatial representation. Neither Level format v2 nor World stores logical
identity, and no layer beneath Engine selection policy knows which ID is the
default.

## Consequences

The bootstrap Resource path is no longer hardcoded in Engine, while startup
without `--level` preserves the bootstrap World. The executable can load
additional ordinary external levels such as `selection_test` without a rebuild
or production special case. Invalid IDs fail before Resource lookup; valid but
missing IDs fail at the storage boundary with both identities available for
diagnosis.

This decision adds no registry, catalog, directory enumeration, metadata,
runtime transition, reload, cache, menu, campaign, savegame, asynchronous
loading, or Level format change. Those remain future work.
