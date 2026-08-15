# HTH Level Format Version 1

`.hthlevel` is the minimal text representation used to persist the current
static `HTHWorld` model. Format version 1 stores one default spawn and an
ordered sequence of static AABBs with the flags and diagnostic visual classes
that World already understands. It is not the engine version and is not a
commitment that final Hertharian maps will remain AABB-based.

## Encoding, Whitespace, and Comments

Syntax tokens are ASCII. Spaces, tabs, LF, CRLF, blank lines, and bare CR are
accepted as separators; CRLF counts as one newline. A final newline is
optional. `#` starts a comment that continues through the end of its line or
through EOF, including after a declaration. Non-ASCII bytes may occur inside
comments because the scanner simply skips comment bytes, but the format makes
no UTF-8 validation guarantee.

UTF-8 BOM and embedded NUL bytes are errors. `//` and `/* */` are not comment
forms. Keywords and semantic tokens are unquoted ASCII identifiers with no
escape syntax.

## Grammar and Order

The first significant declaration must be the exact header `hthlevel 1`,
followed by exactly one spawn. Zero or more static-object blocks follow:

```text
file          := header spawn static_object* EOF
header        := "hthlevel" "1"
spawn         := "spawn" float float float float
static_object := "static_object"
                 "bounds" float float float float float float
                 "flags" flag_form
                 "visual" visual_class
                 "end"
```

Order is mandatory. Unknown keywords, missing or duplicate declarations,
nested objects, and non-comment trailing tokens are errors. Version 1 is the
only accepted version; there is no negotiation, silent upgrade, or forward
compatibility for unknown declarations.

## Decimal Floats

Floats use a decimal grammar with an optional sign, decimal point, and base-10
exponent. Examples include `0`, `-0`, `0.5`, `-0.25`, `1e3`, and `1.0e-3`.
The entire token must match the grammar and convert to a finite `float` without
range error. `nan`, `inf`, hexadecimal floats, overflow, underflow, and tokens
such as `1foo` are rejected.

Conversion uses a private C numeric locale object. It never changes the
process-global locale, and `.` therefore remains the decimal separator
regardless of `LC_NUMERIC`.

## Spawn and Bounds

```text
spawn x y z yaw_radians
bounds min_x min_y min_z max_x max_y max_z
```

There is exactly one spawn and it precedes every object. Bounds map directly
to `HTHAABB`; the parser converts the numbers, while World remains authoritative
for rejecting non-finite, degenerate, or inverted boxes.

## Object Flags

The only canonical forms are:

```text
flags none
flags visible
flags collidable
flags collidable visible
```

`visible collidable`, duplicate flags, combining `none` with another flag, and
unknown flags are errors. Words map explicitly to the existing World flags;
numeric bitmasks are not part of the format.

## Diagnostic Visual Classes

Every object has exactly one visual token. The version 1 mappings are:

```text
none
floor
wall
box
low_step
limit_step
high_ledge
platform
corner
corridor_corner
```

These map explicitly to `HTHWorldVisualClass`, never to its numeric enum
values. An unknown class is an error. Visual classes remain temporary
bootstrap diagnostics rather than materials or gameplay types.

## Example

```text
hthlevel 1

# Default player spawn.
spawn 0 0.05 3 0

static_object
bounds -20 -1 -20 20 0 20
flags collidable visible
visual floor
end
```

## Parsing and Errors

The parser consumes an explicit byte pointer and size and never requires a NUL
terminator. Tokens are slices into the input during parsing; semantic values
are copied into an owned intermediate description. Parse failures are
transactional and carry one-based line and column plus a bounded reason.

Version 1 has no names, strings, entities, dynamic objects, materials,
textures, models, audio, lights, scripts, references to other resources, or
serialization. A future authoring format and map compiler may produce BSP or
another runtime representation and supersede or evolve this bootstrap format.
