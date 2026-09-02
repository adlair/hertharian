# Enemy Target Foundation

Hertharian v0.3.5 represents an Enemy's current target as an internal runtime
relationship from one exact Enemy Entity generation to one exact target Entity
generation. The target is an `HTHEntityHandle`; there is no separate target
identity, object, definition, or persistent reference.

The relationship means only:

```text
This Enemy currently references this Entity.
```

It does not mean the target is visible, perceivable, hostile, reachable, alive
in a gameplay sense, or selected by AI. Target storage and spatial perception
are independent modules.

## Store Contract

The opaque Enemy Target Store is internal to gameplay. Its array is indexed by
owner Entity index. Each entry contains the owner generation, the complete
target handle, and a presence flag. It retains no pointers to Registry, Actor,
Enemy, Spatial, Health, Body, or Entity state. Correct Store/Registry pairing
remains a caller contract.

Setting a target requires the owner to be a current Enemy and the target to be
any live Entity. Target Actor, Enemy, Spatial, Body, Health, or Player state is
irrelevant. Self-target is allowed. Setting another valid target replaces the
old relationship; setting the same target is idempotent. An invalid replacement
fails before mutation, preserving the previous relationship.

Semantic `has` and `get` require the current owner generation to remain an
Enemy and the exact target generation to remain alive. Enemy removal hides a
retained relationship; same-generation Enemy reattachment reveals it again.
Owner or target index reuse never transfers a relationship to a replacement
generation. Failed `get` writes the canonical invalid Entity handle when an
output pointer is supplied.

Clear requires only the exact live owner generation, allowing physical cleanup
after Enemy removal. It clears no other Store and destroys neither endpoint.
Target death, owner death, Enemy removal, Actor removal, and component removal
do not cascade into physical relationship cleanup.

## Storage and Iteration

The Store starts with 64 absent entries and grows deterministically by doubling
to 128, 256, and onward. Allocation occurs only at Store creation and growth.
Set is amortized O(1); has, get, and clear are O(1); iteration is O(capacity).

The value iterator yields `{enemy, target}` pairs in ascending owner Entity
index and filters stale owners, owners without current Enemy visibility, dead
targets, and stale target generations. Spatial is not required. Mutation during
iteration is unsupported; the iterator is neither a snapshot nor mutation-safe.

## Engine Ownership and Deferred Scope

Each Engine owns one private empty Enemy Target Store, initialized after Health
and destroyed before Health. Production establishes zero target relationships
and performs zero target work per frame in v0.3.5.

Automatic acquisition, nearest-target search, priority, threat, timeout,
teams, Player-specific targeting, AI, locomotion, combat, persistence,
networking, scripting, and generic relationship/ECS infrastructure remain
deferred.

As of v0.3.7, a Target relationship does not imply geometric line of sight,
and an Enemy LOS query neither creates, clears, nor replaces that relationship.

As of v0.3.8, explicit Target Selection may set the winning relationship
through this Store API; the Enemy Target Store itself remains policy-free.

As of v0.3.9, Enemy Decision reads the current semantically valid relationship
without clearing, replacing, acquiring, or otherwise mutating it.

As of v0.3.12, Pursuit Runtime preserves a valid relationship and requests
Selection only when none is semantically visible; it never clears on lost LOS
or range.

As of v0.3.13, Enemy Runtime Despawn clears the removed Enemy's outgoing
relation only. It does not scan incoming relations; Entity liveness and
generation checks make references to the despawned Target semantically absent.
