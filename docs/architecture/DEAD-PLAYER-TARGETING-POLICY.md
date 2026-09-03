# Dead Player Targeting Policy

Hertharian v0.3.26 makes only the explicit Player identity untargetable while
the frame's released Player Death snapshot is true. It does not define generic
Actor or Enemy death. The Player remains the same live Entity + Actor + Spatial
+ Health composition owned by Player Target Bridge.

## Ownership and Flow

Engine continues to call `hth_player_death_is_dead()` exactly once, before
Player Movement. The same frame-local `player_dead` value controls movement and
is passed to Bootstrap Enemy Pursuit. After Bridge sync and target lookup,
Bootstrap maps Player-specific policy to a generic runtime value:

```text
excluded_target = player_dead ? player_target : invalid_handle
```

Below Bootstrap, Pursuit Runtime and Target Selection know only the exact
`excluded_target` handle. They receive no Player role, death query, or new
Health dependency. The invalid handle means no exclusion and preserves the
v0.3.25 path.

For every Enemy, Pursuit advances Attack Cadence first, then inspects Current
Target. An exact generation-safe match with `excluded_target` is cleared through
the relation Store API before the missing-Spatial early-out. If no target
remains, Selection scans the historical candidates while skipping every exact
excluded-handle occurrence before Spatial, Perception, LOS, distance, or tie
ranking. Other candidates retain all historical eligibility and ranking rules.

The EnemyTarget Store remains relation-only. Decision, Attack Eligibility,
Attack Execution, DamageIntent, Perception, LOS, Seek, and Chase remain unaware
of Player death. With no alternative, a cleared Enemy performs no Decision,
Seek, Chase, attack build, cadence commit, or damage resolution. Cadence still
advances and is never reset because a target died or changed.

## Snapshot and Recovery Semantics

Targetability is snapshotted once per frame. If Enemy A deals lethal damage in
frame N, every later Enemy still receives that frame's alive snapshot and may
use the Player historically. The target becomes excluded in frame N+1. There is
no mid-loop Health read, death re-query, retroactive clear, or attack refund.

Healing above zero before the next Engine query makes the same Player handle
eligible in that frame. Healing afterward is observed next frame. Selection may
reacquire it normally when the Enemy has no valid Current Target; healing does
not displace a valid alternative or reset cadence.

Full handle equality prevents a stale excluded generation from suppressing a
replacement Entity at the same index. No Registry scan, candidate allocation,
new Enemy scan, PlayerStore, persistent death state, or lifecycle object is
introduced. Bootstrap's incremental work is O(E), auxiliary memory is O(1), and
there are no new per-frame allocations.

Game over, respawn, revive, Downed state, Player destruction, corpse behavior,
death presentation, Enemy/Actor death, factions, hostility, teams, target
memory, and multiplayer targeting remain outside v0.3.26.
