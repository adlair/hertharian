# Resource Path Foundation

v0.2.2 introduced the internal Resource System as the boundary between runtime
consumers and external storage. It implements only synchronous filesystem
storage to raw bytes. Level Loading became its first typed consumer in v0.2.3;
as of v0.2.4, material and texture-image loading consume the same boundary.
Resource still interprets none of those formats.

## Terminology and Boundary

An asset is external source content. A Resource ID is its stable runtime
identity, such as `levels/bootstrap.hthlevel`. Resource Data is a caller-owned
blob containing the bytes loaded for that ID. A Resource ID is not a native or
absolute filesystem path:

```text
canonical Resource ID → Resource System → filesystem root → raw bytes
```

Typed format loaders belong above this boundary. Level, Material, and Image
loaders consume `HTHResourceData` without opening files themselves. Level
Selection is another external consumer: it produces a canonical Resource ID
from a logical Level ID, then Engine passes that Resource ID to Resource
System. Resource neither receives nor interprets logical Level IDs. A future
cached Asset Manager could likewise sit above Resources; Resource System is not
that manager.

## Resource Root and CWD Independence

Each `HTHResourceSystem` owns an immutable copy of one Resource Root. Init
requires an existing directory. The configured path is resolved to a stable
absolute representation during init, so later current-working-directory
changes do not affect resolution. Harmless root representation details such as
a trailing separator may be normalized by this process; Resource IDs are never
normalized.

The engine's current development configuration points at the repository's
`assets/` directory through a build-generated absolute path. This is not
install-root autodetection and no consumer sees it. Packaging/install root
selection remains future work. Runtime does not require launching from the
repository directory.

## Canonical Resource IDs

Resource IDs are non-empty relative paths using only lowercase ASCII `a-z`,
digits, `_`, `-`, `.`, and `/`. Every segment must be non-empty, may not be `.`
or `..`, and may not begin with `.`. Leading, trailing, and duplicate slashes,
backslashes, uppercase, whitespace, control/non-ASCII bytes, absolute paths,
and all other punctuation are rejected.

The validator performs no I/O and no silent canonicalization. It does not
lowercase names, collapse slashes, or translate separators. Extensions remain
part of the ID and carry no meaning to Resource System.

## Resolution and Traversal

A validated ID is appended to the stable root using a precisely sized dynamic
allocation. Resolution has no `PATH_MAX` or fixed 256-byte contract and checks
all size arithmetic before allocation. Invalid IDs fail before filesystem
access, providing syntactic prevention of `.`/`..` traversal and absolute-path
injection.

This is not a filesystem sandbox. Filesystem metadata lookup follows symlinks,
so a symlink inside the root may resolve outside it. Symlink containment is an
explicit non-goal of v0.2.2.

## Binary Loading and Ownership

`hth_resource_load` accepts only an empty output (`data == NULL`, `size == 0`),
opens a resolved regular file in binary mode, determines a checked size,
allocates exactly that many bytes, and requires an exact whole-file read.
Directories and other non-regular objects are rejected. Missing files, short
reads, size/allocation failures, and close errors return false with an empty
output and complete cleanup.

No NUL terminator is appended. A zero-byte file succeeds with `data == NULL`
and `size == 0`. On success the caller owns the independent blob, which remains
valid after Resource System shutdown. `hth_resource_data_release` frees and
resets it and is safe for zero-initialized or already released data. Resource
System retains no pointer, handle, cache entry, or reference count; every load
reads the file again.

## Lifecycle and Execution Model

Engine establishes and retains Level Selection before initializing Resources,
Level Loading, and World. The Level parser consumes raw bytes without depending
on Resource System, and World and Renderer remain unaware of identity and
storage. During shutdown, current runtime consumers and World are destroyed
before Resources and the owned selection, then Platform shuts down.

Loading is whole-file, synchronous, and designed for the current single-thread
engine. It makes no thread-safety guarantee, performs no asynchronous I/O, and
exposes no file handle.

## Explicit Non-goals

Resource System includes no typed parsing, cache, handles, hashing, manifest,
registry, VFS, pak/archive support, compression, streaming, async work, hot
reload, watcher, asset pipeline, symlink sandbox, or install-root autodetection.
Filesystem is the initial storage backend; the canonical Resource ID boundary
allows a future backend to change without passing absolute paths to typed
loaders.
