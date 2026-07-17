<!-- STATUS -->
Epic: Moon Fragment Hunt — DDD Expansion
Feature: Systems Design > Spell Casting (base) GDD
Task: Spell Casting (base) GDD APPROVED (2026-07-17 full /design-review — 5 specialists, initial NEEDS REVISION, blockers fixed in-session: Blackhole ManaCost 100→70 + weave-guarantee cross-constraint, Rule 4/10 GAS contracts pinned, AC5/7/8 rewritten). NEXT: draft Dash/Evasion GDD (/design-system, #6 in design order).
<!-- /STATUS -->

## What changed this session (/design-review spell-casting-base.md — full depth)
- Full adversarial review: game-designer, systems-designer, qa-lead, ue-gas-specialist in
  parallel + creative-director senior synthesis. Initial verdict **NEEDS REVISION** → all 5
  blocking items fixed in-session → **APPROVED**. Log:
  `design/gdd/reviews/spell-casting-base-review-log.md`.
- **Design change**: Blackhole ManaCost 100 → **70**. Cost==MaxMana locked out ALL spells ~3.1s
  after cast, contradicting Rule 4 ("no forced recovery") and sealing the Supernova weave path.
  Registry `mana_cost_blackhole` updated (revised 2026-07-17, old value commented).
- **3 new tuning cross-constraints** (Tuning Knobs): (1) no-infinite-spam
  `ManaCost > Regen×CD`, (2) cooldown-binds-Blackhole, (3) weave-guarantee
  `MaxMana − Cost(BH) ≥ 25` — pillar-critical.
- **GAS contracts pinned** (were blank): Rule 4 — same-frame ability lifecycle, presentation-only
  cancel, no CancelAbility; Rule 10 — skip CommitAbility() when `CostBypass.Active` (documented
  exception to GE-only principle); mana snap Override GE vs periodic regen = benign race.
- **ACs**: AC1/2 renumbered values; AC5 stubbed (test harness applies tag directly); AC7 rewritten
  observable (800uu hearing check); old AC8 (Supernova shield-pierce) removed → deferred to
  Spell Weaving GDD; new AC8 = weave-guarantee execution check.
- **3 new edge cases**: Mana=0 cast, all-3-elements-on-cooldown, bypass-release same-frame race
  (tag change resolves BEFORE gate evaluation — Luna Overdrive GDD must assume this order).
- systems-index row #6 → Approved (reviewed 5, approved 5). Ollama queue: Task 7 (registry
  fact-check spell-casting) + Task 8 (terminology spell-casting vs health-damage-core) appended
  per auto-queueing policy.

<!-- CONSISTENCY-CHECK: 2026-07-17 | GDDs checked: 5 | Conflicts found: 1 (resolved) | Log: docs/consistency-failures.md -->

# Session State

## What changed this session (/design-system spell-casting-base)
- Authored the complete 8-section GDD for Spell Casting (base) at
  [spell-casting-base.md](file:///D:/moon-fragment-hunt/design/gdd/spell-casting-base.md), solo review mode
  (no specialist agents consulted — needs manual review before Production, and an independent
  `/design-review` pass in a fresh session before being marked Approved).
- **MVP scope**: 3 fixed elemental spell slots (Blackhole/Radial Force, Fire, Lightning) — no
  loadout swapping. Instant-cast (no windup/charge), per-element independent cooldowns, single
  shared Mana pool with passive regen + Core Extraction full-refill bonus.
- **Key numbers** (solo-proposed, unplaytested): MaxMana=100, ManaCost Blackhole=100/Fire=25/
  Lightning=25, Cooldown Blackhole=6.0s/Fire=2.0s/Lightning=2.5s, ManaRegenRate=8/s,
  NoiseLoudness Blackhole=1.0/Fire=0.6/Lightning=0.7.
- **Key interface contracts established** (carry forward when designing dependents):
  - Casting NEVER blocks movement (`MovementLocked` not used) — symmetric counterpart to Player
    Movement Core Rule 7, both docs now own this contract from their own side.
  - All spell damage goes through Health/Damage Core's single `ApplyDamage` entry point
    (Rule 1); base 3 spells default `bBypassDefense=false` — only Spell Weaving's synergy
    combos (Supernova etc.) may override to `true`.
  - This doc is the one that defines the Core Extraction Execution mana-refund amount
    (100% / snap to MaxMana on `OnExecuted`) — Health/Damage Core Rule 9 explicitly deferred
    that number to whichever system subscribes to the event, and this is that system.
  - Every spell impact must call `MakeNoise(loudness)` per Enemy AI (base)'s hearing contract —
    `NoiseLoudness` per element is a new registry formula (`spell_noise_loudness`) that chains
    directly into Enemy AI's `noise_detection_radius` formula.
  - Luna Overdrive (undesigned) will consume a single `CostBypass.Active` GameplayTag that
    bypasses BOTH mana cost and cooldown gating simultaneously — no partial-bypass mode exists
    in this MVP scope (Edge Cases explicitly closes that gap).
- **Registered in `design/registry/entities.yaml`**: 8 new constants (max_mana, mana_cost_*,
  cooldown_*, mana_regen_rate), 4 new formulas (effective_mana_cost, cast_gate_check, new_mana,
  spell_noise_loudness). Also appended `design/gdd/spell-casting-base.md` to the existing
  `noise_detection_radius` formula's `referenced_by` (cross-doc formula chain).
- **Updated `systems-index.md`**: row #6 → "Designed (pending review)", Design Doc link added,
  progress counts incremented (started 5, MVP designed 5/9).
- **Open questions flagged, not resolved by this doc** (see GDD's own Open Questions section):
  cast aiming method (auto-aim/skillshot/cursor) undecided, gamepad button mapping for
  3 spells + movement + dash undecided, UE5.8 GAS attribute-init pattern needs
  `ue-gas-specialist` header verification (same gap as health-damage-core.md).

## What changed this session (/design-review enemy-ai-base.md + Ollama pipeline restructure)
- Ran a formal `/design-review` on `design/gdd/enemy-ai-base.md` (solo depth, per project default) —
  **APPROVED**, no blockers, no doc edits needed. Doc's front-matter already said Approved from the
  prior manual pass; this was the first pass through the actual `/design-review` skill.
- Restructured the Ollama overnight pipeline at the user's request:
  - Moved the task queue `production/ollama-instructions.md` → `OLLAMA-INSTRUCTIONS.md` (repo root),
    so the bot reads it directly instead of the (never-actually-implemented) "Gemini scans the
    whole repo" design the docs used to describe.
  - Reformatted every task's "Context to inject" into strict `- {{PLACEHOLDER}}: path` lines and
    made Tasks 3/5/6 fully self-contained (they used to say "same prompt as Task 2/4") — required
    for the new parser, which treats each task block independently.
  - Rewrote `tools/overnight-bot/discord_ollama_bot.py`: removed the hardcoded `TASKS` list, added
    `parse_task_queue()` (regex-parses `OLLAMA-INSTRUCTIONS.md`, only runs `[ ]` blocks, warns in
    Discord about malformed ones instead of silently dropping them). Verified with a standalone
    parse test (all 6 tasks parse correctly) and `python -m py_compile` (syntax OK) — have not
    run the live bot (needs Discord/Gemini/Ollama credentials not available in this session).
  - Corrected the stale "Gemini reads the repo and picks tasks" framing in
    `production/ollama-delegation-criteria.md` and `production/overnight-protocol.md` to match
    reality: Gemini only 3-line-summarizes Ollama's output, never reads the queue or the repo.
  - Updated all path references: `.claude/docs/ollama-delegation.md`, `AGENTS.md`,
    `tools/overnight-bot/README.md`, `tools/overnight-bot/setup-plan-macbook.md`.
  - Note: this was a detour, not a step in the GDD design order below — next real design work is
    still Spell Casting (base).

## Cross-tool note (added 2026-07-16, updated 2026-07-16 — 3rd worker added)

This project now rotates across **three** workers: **Claude Code** and **Antigravity CLI**
(Gemini) do the real design/implementation work session to session; **Ollama** runs unattended
overnight on a small queue of low-risk, easy-to-verify tasks (its output is never trusted
directly — see below). Whichever tool picks up next:

- Read this file first, always — it's the shared handoff point for all three workers.
- **Check `production/overnight-output/` before starting real work** — if Ollama ran overnight,
  review its drafts against `OLLAMA-INSTRUCTIONS.md`'s (repo root) checklists (a couple minutes),
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

- **Enemy AI (base) GDD**: Approved.
- **Spell Casting (base) GDD**: Designed (pending review) — all 8 sections + Visual/Audio/UI/Open
  Questions written to `design/gdd/spell-casting-base.md`. NOT yet run through `/design-review`.

**➤ NEXT COMMAND TO RUN (fresh session, either tool):**
`/design-review design/gdd/spell-casting-base.md` (must run in a session that did NOT author it)

Then continue design order: Dash/Evasion → Combo/Tension Gauge → Luna Overdrive → Combat HUD.

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
1. Run `/design-review design/gdd/spell-casting-base.md` (fresh session — independent of the authoring session).
2. Then draft Dash/Evasion GDD next.
3. Continue design order: Dash/Evasion → Combo/Tension Gauge → Luna Overdrive → Combat HUD.

## What changed this session (enemy-ai-base.md review pass)
- Reviewed and **APPROVED** `design/gdd/enemy-ai-base.md` end-to-end (independent review session).
- Generated complete design review artifact [design_review_enemy_ai_base.md](file:///C:/Users/qjqmf/.gemini/antigravity-cli/brain/9287b538-86da-45d5-a5b0-82928d6b1ac3/design_review_enemy_ai_base.md).
- Updated `enemy-ai-base.md` status to `Approved`.
- Updated `systems-index.md` row #4 to `Approved`, incremented progress metrics (reviewed 4, approved 4, MVP designed 4/9).

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
