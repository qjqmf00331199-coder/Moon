<!-- STATUS -->
Epic: Moon Fragment Hunt — DDD Expansion
Feature: Systems Design > Enemy AI (base) GDD
Task: All 8 sections + UI/Open Questions written and approved (solo mode, no specialist agents consulted). Status: Designed (pending review). NEXT: run /design-review design/gdd/enemy-ai-base.md in a fresh session, then design Spell Casting (base).
<!-- /STATUS -->

# Session State

## Cross-tool note (added 2026-07-16, updated 2026-07-16 — 3rd worker added)

This project now rotates across **three** workers: **Claude Code** and **Antigravity CLI**
(Gemini) do the real design/implementation work session to session; **Ollama** runs unattended
overnight on a small queue of low-risk, easy-to-verify tasks (its output is never trusted
directly — see below). Whichever tool picks up next:

- Read this file first, always — it's the shared handoff point for all three workers.
- **Check `production/overnight-output/` before starting real work** — if Ollama ran overnight,
  review its drafts against `production/ollama-instructions.md`'s checklists (a couple minutes),
  promote what's good into the real files, discard the rest, then clear the directory. Full
  protocol: `production/overnight-protocol.md`. Ollama has no file/tool access — it only ever
  produces draft text that a human or one of you reviews; it never edits `design/gdd/`, `docs/`,
  or `entities.yaml` directly.
- Antigravity CLI now has its own project context mirror at [AGENTS.md](file:///D:/moon-fragment-hunt/AGENTS.md)
  (Claude Code keeps reading `CLAUDE.md` + `.claude/` as before — unchanged).
- `unreal-mcp` is now registered for **both** tools: Claude Code via `.mcp.json` /
  `Moon/.mcp.json` (already existed), Antigravity CLI via the newly created
  [.agents/mcp_config.json](file:///D:/moon-fragment-hunt/.agents/mcp_config.json) (same server,
  `http://127.0.0.1:8000/mcp`, just a different config key: `serverUrl` instead of `type`+`url`).
- Claude Code has guided slash-command skills (`/design-system`, `/design-review`, etc.)
  that Antigravity CLI doesn't have equivalents for. If Antigravity is doing the next design
  pass, follow the section list + rules written directly into `AGENTS.md` instead of expecting
  a slash command; the output should still satisfy the same 8-section GDD standard so either
  tool can review the other's work.
- Whoever finishes a session should update **this section's task pointer above and the
  "To resume in a new session" list below** before ending, so the other tool doesn't stall
  waiting for missing context.

## Current task — READY FOR HANDOFF TO A NEW SESSION

- **Camera System (base) GDD**: Approved.
- **Enemy AI (base) GDD**: Drafted (solo mode, all 8 sections + UI/Open Questions). Status: Designed (pending review) — not yet independently reviewed.

**➤ NEXT COMMAND TO RUN (fresh session, either tool):**
```
/design-review design/gdd/enemy-ai-base.md
```
Then continue design order: Spell Casting (base) → Dash/Evasion → Combo/Tension Gauge → Luna Overdrive → Combat HUD.

Do not begin implementation until this GDD is approved.

**Key decisions baked into Enemy AI (base) (carry forward when designing dependents):**
- MVP scope is 2 archetypes only: Grunt (melee, MaxHealth 30) and Ranged (MaxHealth 20, keeps a
  400–1200uu engage band and retreats if the player closes inside 400uu).
- Perception uses Sight (cone) + Hearing (`MakeNoise`-driven) — any system causing a loud event
  (destructible collapse, spell impact, player landing) should call `MakeNoise` with a
  0.0–1.0 loudness value; this doc doesn't enumerate all noise sources itself.
- Attack telegraphs are anim-driven: `OnAttackTelegraphed` (windup start AnimNotify) and
  `OnAttackCommitted` (strike-frame AnimNotify, this is when damage actually applies). The gap
  between them (`TelegraphWindow`) must stay >= 0.4s (`min_telegraph_window`, registered in
  entities.yaml) or the attack isn't fairly dodgeable — **Dash/Evasion must subscribe to these
  two delegates** for its just-dodge timing, not invent its own detection.
- This doc does NOT grant `State.Invulnerable`/`State.Executable` (Dash/Evasion and Enemy Elite
  Shield own that) — it only reads them, and confirmed those tags never block its own state
  transitions (they're overlays).
- Stagger is a slot only — Super Armor / CC Interrupt (undesigned) owns exactly when it triggers;
  this doc just exposes `TriggerStagger()`/`ClearStagger()`.
- Archetype identity exposed via GameplayTag (`Enemy.Archetype.Grunt`/`Enemy.Archetype.Ranged`)
  for Enemy Elite Shield (undesigned) to key off of.
- Registered in `design/registry/entities.yaml`: entities `grunt`/`ranged`, constants
  `max_health_grunt`(30)/`max_health_ranged`(20)/`min_engage_range`(400)/`max_engage_range`(1200)/
  `min_telegraph_window`(0.4), formulas `attack_telegraph_window`/`noise_detection_radius`/
  `ranged_engage_distance_check`.
- Open risk carried forward: large-swarm (dozens of enemies) AIPerception/BT tick performance is
  unmeasured — flag before Production if Environmental Chain-Destruction scale grows. Also
  `docs/engine-reference/unreal/modules/navigation.md` is only verified against UE5.7, not the
  pinned 5.8 — re-verify against real 5.8 headers at implementation time.

**Key decisions baked into Health/Damage Core (carry forward when designing dependents):**
- Single damage entry point (`ApplyDamage`/GameplayEffect) for all sources — no separate paths.
- No generic Defense/Armor stat in MVP scope — only a boolean shield/no-shield intercept hook
  (owned by future Enemy Elite Shield GDD) and a `bBypassDefense` true-damage flag.
- Two GameplayTags this doc owns and gates access to: `State.Invulnerable` (100% damage block —
  Dash/Evasion, Status Effect, Core Extraction Execution will grant it) and `State.Executable`
  (F-key execution eligibility — **only** Dash/Evasion just-dodge and Enemy Elite Shield break
  may grant it; Core Extraction Execution only consumes it via `TryExecute()`). Both tags are
  reference-counted (loose tag count), not plain booleans — two grantors won't stomp each other.
  `bBypassDefense` bypasses shields, never bypasses `State.Invulnerable` — kept those two
  concepts strictly separate to avoid overdrive-tier damage trivializing i-frames.
- `OnHealthPercentCrossed(threshold)` event exposed now specifically because Boss Phase (later
  in the design order) will need it for phase-transition triggers.
- No player HP regen; player death = instant checkpoint respawn (no game-over screen), exact
  checkpoint mechanism deferred to a not-yet-designed Persistence system.
- **Cross-doc gap found and fixed**: `systems-index.md`'s Dependency Map was missing
  Dash/Evasion → Health/Damage Core (Dash/Evasion consumes the two tags above). Added it to
  both the enumeration table and the Core Layer dependency list.
- Player Movement carryovers still open: `MovementLocked` flag ownership (Status Effect only),
  Camera System mutual dependency, and the missing Progression system for double-jump unlock —
  none of these blocked Health/Damage Core, still open for whoever designs those systems.

**To resume in a new session:**
1. Run design drafting for `enemy-ai-base` GDD (`/design-system enemy-ai-base`).
2. Continue design order: **Enemy AI (base)** next → Spell Casting (base) → Dash/Evasion → Combo/Tension Gauge → Luna Overdrive → Combat HUD.

## What changed this session (enemy-ai-base.md drafting, /design-system enemy-ai-base)
- Completed the 8-section GDD for Enemy AI (base) in [enemy-ai-base.md](file:///D:/moon-fragment-hunt/design/gdd/enemy-ai-base.md), plus UI Requirements and Open Questions. Solo review mode — no specialist agents (systems-designer, ai-programmer, qa-lead, art-director) consulted; needs manual review before Production.
- MVP archetype scope: Grunt (melee, 30 HP) + Ranged (20 HP, 400–1200uu engage band).
- Defined the anim-driven attack telegraph contract (`OnAttackTelegraphed`/`OnAttackCommitted`, min 0.4s window) that Dash/Evasion (not yet designed) must consume for just-dodge timing — this is the key interface this GDD hands off to the next system in the design order.
- Updated [systems-index.md](file:///D:/moon-fragment-hunt/design/gdd/systems-index.md): row #4 → "Designed (pending review)", started/MVP-designed counts incremented to 4.
- Updated [entities.yaml](file:///D:/moon-fragment-hunt/design/registry/entities.yaml): added `grunt`/`ranged` entities, 5 new constants, 3 new formulas.
- Did not run `/design-review` in this session (by design — must run in a fresh session per protocol).

## What changed this session (camera-system-base.md review pass)
- Reviewed and **APPROVED** `design/gdd/camera-system-base.md` end-to-end (independent review session).
- Generated complete design review artifact [design_review_camera_system_base.md](file:///C:/Users/qjqmf/.gemini/antigravity-cli/brain/c423aba1-458b-4f03-b7a1-016fed13de9e/design_review_camera_system_base.md).
- Updated `camera-system-base.md` status to `Approved`.
- Updated `systems-index.md` row #2 to `Approved`, incremented progress metrics (reviewed 3, approved 3, MVP designed 3/9).

## What changed this session (camera-system-base GDD drafting)
- Completed the 8-section GDD for the Camera System (base) in [camera-system-base.md](file:///D:/moon-fragment-hunt/design/gdd/camera-system-base.md).
- Solved the mutual dependency with Player Movement (camera-relative input, controller rotation alignment).
- Enforced rules and edge cases for viewport collision, particularly ignoring destructible geometry fragments to avoid camera snapping jitter.
- Defined tuning knobs (spring arm length, lag speeds, FOV parameters) and QA acceptance criteria.
- Updated [systems-index.md](file:///D:/moon-fragment-hunt/design/gdd/systems-index.md) status to Draft and incremented started design docs count to 3.

## What changed this session (health-damage-core.md review pass)
- Reviewed and **APPROVED** `design/gdd/health-damage-core.md` end-to-end (independent review session).
- Generated complete design review artifact [design_review_health_damage_core.md](file:///C:/Users/qjqmf/.gemini/antigravity-cli/brain/2877398e-99d2-47a1-86e2-5facfecb8b31/design_review_health_damage_core.md).
- Updated `health-damage-core.md` status to `Approved`.
- Updated `systems-index.md` row #3 to `Approved`, incremented progress metrics (reviewed 2, approved 2).
- Resolved a dependency map gap: updated `Super Armor / CC Interrupt` in `systems-index.md` to show its dependency on `Health/Damage Core`.

## What changed this session (housekeeping, prior session)
- Completed `/design-review` for [player-movement.md](file:///D:/moon-fragment-hunt/design/gdd/player-movement.md). Created [design_review_player_movement.md](file:///C:/Users/qjqmf/.gemini/antigravity-cli/brain/b627c449-b2fc-46cd-87b3-a87ac137d6d7/design_review_player_movement.md) with Gemini (Antigravity Design Review Agent) reviewer stamp.
- Updated status headers in [player-movement.md](file:///D:/moon-fragment-hunt/design/gdd/player-movement.md) and [systems-index.md](file:///D:/moon-fragment-hunt/design/gdd/systems-index.md) to reflect `Approved` status.
- `production/session-state/plan.md` still holds an **unrelated, unfinished** checklist (Enemy BP tree: BP_Enemy_Base done, BP_Spider/BP_SpiderKing/player-extension/map-placement not done). Left untouched on purpose — not part of this thread, don't overwrite or treat as done.

## Progress so far (this thread)
- Reviewed the full DDD Expansion concept doc (Blood Moon Overdrive, Spell Weaving, environment chain-destruction, Core Extraction execution, Arena Morphing boss finale) — assumed **solo PC, no multiplayer**.
- Completed Player Movement GDD design review and marked it as APPROVED.
- UE5.8 feasibility pass on all 6 systems: 5 are standard/well-supported patterns (GAS, Chaos Destruction, Niagara, Enhanced Input). Arena Morphing was the one open risk (Nanite + Geometry Collection interaction, undocumented in our engine-reference snapshots).
- Ran a mid-production spike to close that risk (see below) — **CONFIRMED YES**, compatible.
- Set up `unreal-mcp` direct editor scripting this session: added `AllToolsets` plugin to
  `Moon.uproject` (was only `MCPClientToolset` + `ModelContextProtocol` before — those alone
  only exposed a trivial `AgentSkillToolset`). After an editor restart, ~20 toolsets became
  available (scene/actor/asset/static-mesh/Dataflow/etc.) — this unlocks scripted editor
  control for future sessions too, not just this spike.

## Spike result (2026-07-16, concluded)

**CONFIRMED YES**: UE5.8 Geometry Collection supports Nanite per-fragment — does NOT force
non-Nanite fallback. Verified via `unreal-mcp` scripting: created `/Game/_Spikes/GC_SpikeFloor`
from a Nanite-enabled cube using the `DF_GC_Template_StaticMesh` Dataflow template, fractured to
38 pieces (Voronoi), confirmed `enableNanite: true` + `bEnableNaniteFallback: false` +
`LogStaticMesh: NaniteBuild` in the output log at bake time. Visually confirmed fractured seams
in viewport screenshot.

Gotcha for future reference: the Dataflow graph's "Evaluate" toolbar button does NOT reliably
trigger the bake when driven headlessly — the bake actually committed when the asset editor tab
was **closed** (temp Dataflow cache → real asset). Not an issue for normal interactive use (Fracture
Mode UI still works normally); only came up because of headless MCP scripting.

**Still open**: 60fps perf budget under full production load (40+ pieces + 5 rising platforms +
player air-dash simultaneously) — not measured this session. Follow up before committing Arena
Morphing to production if scale grows significantly beyond this test.

Full writeup: `prototypes/arena-morphing-spike-2026-07-16/SPIKE-NOTE.md`

## Open questions
- Perf budget test (see above) — the only remaining unknown for Arena Morphing feasibility.
