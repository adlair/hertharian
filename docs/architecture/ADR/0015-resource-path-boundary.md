# ADR-0015: Resource Path Boundary

- Status: Accepted
- Milestone: v0.2.2

## Context

Future level and asset loaders need external bytes, but allowing each consumer
to open absolute or current-working-directory-relative paths would couple
runtime identity to developer layout, operating-system conventions, and the
initial filesystem backend. That would hinder packaging, Steam-style
deployment, tests, and future archive/VFS storage.

## Decision

Runtime systems identify external content with canonical lowercase,
resource-relative IDs. The internal Resource System owns a stable Resource
Root, validates IDs, resolves them against the root, and returns caller-owned
raw bytes. Consumers receive neither absolute paths nor `FILE *` handles.

Resource System owns only the storage/path boundary. Typed loaders interpret
the returned bytes above it. The v0.2.3 Level Loader consumes this boundary
rather than performing filesystem I/O directly.

## Consequences

Resource IDs have strict portable identity and malformed IDs are rejected
instead of normalized. Runtime no longer needs the current working directory
to identify content. Filesystem remains replaceable behind the same boundary,
and tests can use isolated roots without developer paths.

The initial implementation is synchronous whole-file filesystem I/O. It does
not sandbox symlinks and makes no thread-safety guarantee. Cache, VFS/pak,
typed resources, async/streaming I/O, format parsing, and Level Loading are
explicitly outside this decision.
