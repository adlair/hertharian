# ADR-0003: The Engine Uses a Central Frame Loop

## Status

Accepted

## Context

The engine needs one explicit lifecycle and a predictable ordering point for
the systems that will be introduced in later versions. Distributing ownership
of the frame across platform, client, server, or renderer code would obscure
that order and couple the engine lifecycle to subsystems that do not yet exist.

The intended lifecycle and complete target frame sequence are:

```text
Engine_Init
    ↓
Engine_Run
    ↓
┌──────────────────────────┐
│ Engine_Frame             │
│                          │
│ wait                     │
│ input                    │
│ events / commands        │
│ server                   │
│ events / commands        │
│ client                   │
│ network flush            │
└────────────┬─────────────┘
             │
             └──── repeat

Engine_Shutdown
```

## Decision

The engine owns a central lifecycle composed of initialization, a run loop,
one frame function, and shutdown. The run loop is the sole owner of repeated
frame execution.

The sequence shown above is the architectural target. In v0.1.0, the frame
function only records and reports frame progress; wait, input, command, server,
client, and network-flush stages remain placeholders until their corresponding
subsystems are deliberately introduced.

For deterministic bootstrap runs and automated tests, the engine accepts a
frame limit. A zero limit is reserved internally to mean no automatic limit,
allowing the same loop to support a future run-until-exit mode.

## Consequences

- Subsystems will have a single, documented place in the frame order.
- Bootstrap tests can run a deterministic number of frames and terminate.
- Future exit handling can stop the loop without changing lifecycle ownership.
- v0.1.0 demonstrates lifecycle and ordering only; it does not provide the
  future frame stages as functional subsystems.
