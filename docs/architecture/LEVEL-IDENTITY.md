# Runtime Level Identity and Startup Selection

Hertharian v0.2.6 gives each startup level a logical Level ID independent from
storage, parsing, and World representation:

```text
runtime default or --level
        ↓
logical Level ID
        ↓ Level Selection
canonical Resource ID
        ↓ Resource System
raw bytes → Level v2 Description → World
```

Level ID, Resource ID, `HTHLevelDescription`, and `HTHWorld` are distinct
identities. World represents resolved spatial content and retains none of the
selection or storage identity.

## Grammar and Mapping

A Level ID is a non-empty string matching `[a-z0-9_-]+`. Valid examples include
`bootstrap`, `selection_test`, `crypt-01`, `forest_intro`, `level2`, and
`a_b-c9`. Uppercase, whitespace, dots, extensions, slashes, backslashes,
control characters, non-ASCII bytes, and path-like values such as `../foo` are
invalid. Input is never trimmed, case-folded, or normalized.

Level Selection performs the pure deterministic mapping:

```text
R(L) = "levels/" + L + ".hthlevel"
```

For example, `bootstrap` maps to `levels/bootstrap.hthlevel`, while
`selection_test` maps to `levels/selection_test.hthlevel`. Mapping validates
the Level ID, checks all size arithmetic, and dynamically allocates the exact
owned strings. It performs no filesystem access and has no Resource System,
Level parser, World, SDL, X11, or OpenGL dependency.

## Runtime Policy and CLI

The single startup default is logical ID `bootstrap`. `--level <id>` overrides
it, independent of the ordering of `--headless`, `--frames`, and
`--debug-fps-input`. A duplicate `--level`, a missing value, or an invalid ID is
an options error. A syntactically valid but absent ID, such as
`does_not_exist`, creates a valid selection and fails later when Resource System
cannot load `levels/does_not_exist.hthlevel`. Syntax validity and resource
existence are deliberately separate.

## Ownership and Lifecycle

Runtime parsing may temporarily reference command-line storage. During Engine
initialization, `HTHLevelSelection` copies the Level ID and constructs its own
Resource ID. Engine retains that private selection through Resource loading,
Level parsing, World construction, and runtime operation, then destroys both
strings during shutdown. Initialization failure paths release any selection
and storage already acquired. No mutable global, static scratch buffer,
`PATH_MAX`, fixed path buffer, or borrowed `argv` pointer becomes Engine state.

There is no level registry or directory scan: any syntactically valid Level ID
is accepted and Resource loading determines existence. v0.2.6 adds no metadata,
display name, menu, runtime transition, reload, cache, campaign, savegame, or
network synchronization. Retaining logical identity prepares a clean boundary
for such future consumers without implementing them now.
