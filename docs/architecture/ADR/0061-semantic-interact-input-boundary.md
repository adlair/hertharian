# ADR-0061: Semantic Interact Input Boundary

## Status

Accepted for Hertharian Engine v0.3.37.

## Context

Player Revive Interaction already consumes a semantic held boolean, but the
engine has no gameplay-owned boundary that projects final physical Input into
an Interact action. Released `HTHInput` queries expose all state needed without
changing Input, Platform, Engine, or Revive.

## Decision

INTERACT IS A SEMANTIC GAMEPLAY ACTION.

INTERACT IS NOT REVIVE-SPECIFIC.

SEMANTIC ACTION CONSUMES FINAL HTHINPUT STATE. The private gameplay query maps
the existing key `down`, `pressed`, and `released` values directly and never
reconstructs physical events or edges.

PLATFORM BACKENDS REMAIN UNAWARE. No SDL, X11, Wayland, Platform event, or
keyboard-reconciliation detail enters the semantic module.

BINDING IS CALLER/CONFIG OWNED. NO PRODUCT KEY IS FROZEN. SINGLE HTHKEY BINDING
ONLY IN THIS FOUNDATION. Every query receives its binding, and invalid enum
values are technical failures rather than an unbound policy.

ACTION STATE IS DOWN + PRESSED + RELEASED. PROJECTION IS STATELESS. NO SEMANTIC
ACTION STORE. The value mirrors Input's current frame-latched snapshot and is
not retained by the module.

FOCUS SAFETY IS INHERITED FROM INPUT. REPEAT SEMANTICS MIRROR INPUT. CAPTURE
DOES NOT REDEFINE KEYBOARD ACTION STATE. Focus loss/reconciliation, repeat
handling, and frame boundaries remain exclusively owned by Input; relative
mouse capture is independent.

NO GAMEPAD IMPLEMENTATION YET. NO GENERAL REBINDING SYSTEM YET. Multiple
physical bindings are also deferred because correct aggregate-release meaning
can require persistent aggregate state.

NO ENGINE OR REVIVE INTEGRATION. NO TARGET / CONSUMPTION / ARBITRATION POLICY.
The foundation has no runtime consumer and does not select gameplay context.

ZERO HEAP ALLOCATION. O(1) TIME AND O(1) AUXILIARY MEMORY.

## Consequences

The semantic boundary is reusable by future contextual gameplay without
naming or depending on Revive. A future resolver may support rebinding,
gamepads, or multiple bindings and produce the same state shape, and a future
command frame may consume it. Those systems and all production wiring remain
outside v0.3.37.
