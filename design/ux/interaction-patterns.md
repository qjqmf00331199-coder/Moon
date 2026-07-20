# Interaction Patterns — Moon Fragment Hunt

> **Status**: Draft (MVP combat interactions — authored for Technical Setup gate)
> **Author**: Antigravity ux-designer pass (solo mode, 2026-07-20)
> **Last Updated**: 2026-07-20
> **Source Docs**: design/gdd/player-movement.md, design/gdd/spell-casting-base.md, design/gdd/dash-evasion.md, design/gdd/combat-hud.md, design/gdd/luna-overdrive.md
> **Scope**: MVP combat interaction patterns only. Menu/inventory/settings patterns deferred.

---

## 1. Pattern Catalog Overview

This document defines the interaction contract for every player-initiated action in the MVP combat loop. Each pattern specifies: trigger, input, expected system response, feedback channels, and failure mode handling.

| Pattern ID | Name | Input | System Owner |
|---|---|---|---|
| IP-MOV-001 | Directional Movement | WASD / Left Stick | Player Movement |
| IP-MOV-002 | Jump | Space / Cross | Player Movement |
| IP-SPELL-001 | Cast Blackhole | 1 / LB | Spell Casting (base) |
| IP-SPELL-002 | Cast Fire | 2 / RB | Spell Casting (base) |
| IP-SPELL-003 | Cast Lightning | 3 / RB+LT | Spell Casting (base) |
| IP-DASH-001 | Dash / Evade | Shift / B | Dash/Evasion |
| IP-EXEC-001 | Execute (Core Extraction) | F / Y (when available) | Core Extraction Execution (미설계 — assumed) |
| IP-CAM-001 | Look / Aim Camera | Mouse / Right Stick | Camera System (base) |

---

## 2. Movement Patterns

### IP-MOV-001: Directional Movement

**Trigger**: Player inputs directional axis (WASD or left stick)  
**Input type**: Analog (0.0–1.0 per axis, magnitude = move speed scalar)

**System Response**:
1. `InputMagnitude × MaxWalkSpeed (600 uu/s)` applied via CMC `AddMovementInput`
2. Character facing rotates toward movement direction (camera-relative)
3. Character facing is **decoupled from camera yaw** — player can look left while moving right

**Feedback Channels**:
- Visual: locomotion animation (blend from idle → walk → run based on `InputMagnitude`)
- Audio: footstep SFX (surface-dependent — deferred to audio GDD)
- No HUD feedback for movement itself

**Edge Cases**:
- Input during spell cast: movement **not locked** (GDD mandate — spell-casting-base.md Rule 2)
- Input during dash: movement input overridden by dash velocity for dash duration
- Input while `MovementLocked` (Status Effect): input silently consumed, no movement applied, no feedback to player yet (Status Effect system undesigned — TODO: add lock indicator)

---

### IP-MOV-002: Jump

**Trigger**: Jump button press (Space / Cross)  
**Input type**: Digital (press event)

**System Response**:
1. CMC jump initiated at `JumpZVelocity = 600 uu/s`
2. Air control active (`AirControl = 0.6`) — full directional control maintained in air
3. Jump buffer (0.15s) and coyote time (0.15s) implemented for responsiveness

**Feedback Channels**:
- Visual: jump animation + brief camera lag release
- Audio: jump SFX (deferred)

**Edge Cases**:
- Jump during dash: depends on timing; both should be executable if inputs are simultaneous (implementation TBD — no GDD rule prohibits this)
- Jump near Arena Morphing platforms: Z-launch from `LaunchCharacter` API — same input, different initiating system

---

## 3. Spell Casting Patterns

### IP-SPELL-001/002/003: Cast Spell (Blackhole / Fire / Lightning)

**Trigger**: Spell key press (1/2/3 or controller mapping)  
**Input type**: Digital (press event — instant cast, no hold)

**System Response**:
1. `CanActivateAbility()` check:
   - If `CostBypass.Active` tag: skip Mana and Cooldown gate → proceed
   - Else: `Mana ≥ ManaCost[Element]` AND `NOT OnCooldown[Element]` → proceed or fail silently (no effect, brief input flash)
   - Cast-rate limiter: `MaxCastsPerSecond = 20` — silently blocks if exceeded (human CPS ~6–10, so this only blocks macros)
2. On gate pass: `GE_SpellCost` applied (deducts Mana), `GE_SpellCooldown` applied (starts cooldown timer), ability effect (Blackhole radial force / Fire impact / Lightning chain) activated
3. `OnSpellCast(Element)` delegate fires → HUD updates cooldown display
4. On spell impact: `ApplyDamage(RawDamage)` → HDC pipeline, `OnSpellHit(Element, Target)` → Tension Gauge gain

**Feedback Channels**:
- Visual: spell VFX at impact point (element-specific — see art-bible Section 4 for color)
- HUD: Cooldown overlay animates from full → empty over cooldown duration; Mana bar decreases
- Tension Gauge: fills proportionally to `TensionGainPerElement` (fire=30, lightning=45, blackhole=70 — from combo-tension-gauge.md)
- Audio: cast SFX + impact SFX (deferred)

**Failure State Feedback**:
- Mana insufficient: brief red flash on Mana bar + controller vibration (short, low intensity). No ability animation.
- On cooldown: brief grey flash on cooldown icon. No ability animation.
- Rate-limited: silent (human CPS never reaches 20; only macros hit this)

**Edge Cases**:
- Cast during Overdrive: bypass active, all three spells instant-cast. Feedback: no Mana drain animation, no cooldown animation (HUD shows ∞ per combat-hud.md Rule 7).
- Same-frame multi-cast (human near-simultaneous): per-element per-frame gate — only one cast per element per frame maximum.

---

## 4. Dash / Evasion Patterns

### IP-DASH-001: Dash

**Trigger**: Dash button press  
**Input type**: Digital (press event)

**System Response**:
1. Consume 1 dash charge (fractional charge accumulation system — `DashChargeAccumulationRate` TBD in dash-evasion.md tuning)
2. `LaunchCharacter` impulse in camera-relative input direction (or facing direction if no input)
3. `State.Invulnerable` tag granted for `dash_invuln_duration = 0.2s`
4. Just-Dodge evaluation (parallel, not sequential):
   - If enemy attack telegraph is active AND player is within dodge direction window within `just_dodge_window = 0.2s`:
     - `State.Executable` tag granted on target for `executable_duration = 3.0s`
     - `OnTagAdded(State.Executable)` fires → Combo/Tension Gauge receives `JustDodgeTensionBonus = +20`
     - Camera shake (brief, telegraphs the just-dodge success)
5. Charge refund: if Just-Dodge succeeds, 1 charge refunded (net cost = 0 for successful just-dodge)

**Feedback Channels**:
- Visual: dash motion blur / velocity lines, brief invulnerability flash (white, see art-bible)
- HUD: Dash charge display decrements; refills over recharge timer
- Just-Dodge success: distinctive camera shake + Tension gauge jump (+20) + audio sting (deferred)
- Executable prompt: appears on-enemy (diegetic) as amber pulsing icon for `executable_duration`

**Failure State**:
- No charges: input consumed, no dash, brief grey flash on Dash charge display

---

## 5. Execution Pattern

### IP-EXEC-001: Core Extraction (F-key Execute)

> **Note**: Core Extraction Execution GDD is not yet designed (Vertical Slice priority). This pattern is based on the assumed interface documented in combat-hud.md Rule 6 and dash-evasion.md.

**Trigger**: F-key / Y-button press when `State.Executable` is active on a nearby enemy  
**Availability window**: 3.0 seconds from `State.Executable` grant

**System Response** (assumed — pending CEE GDD):
1. Camera push-in / close-up on target
2. Extraction animation plays (0.5–1.5s, estimated)
3. Target instant-death
4. `OnExecuted` event: player Mana snaps to `MaxMana = 100` (spell-casting-base.md Core Rule 8)
5. Particle burst + visual payoff

**Feedback Channels**:
- Visual: camera cutscene-style close-up, particle burst, Mana bar snap to full
- HUD: Mana bar flashes full; Executable prompt disappears from enemy
- Audio: execution SFX (deferred — expected to be a high-impact audio moment)

**Failure State**:
- F-key pressed when NOT near `State.Executable` enemy: silently ignored (no input feedback — to avoid polluting other F-key uses in future)
- Executable window expires: prompt disappears, system resets (no penalty)

---

## 6. Camera Interaction Patterns

### IP-CAM-001: Look / Aim

**Trigger**: Mouse movement (KBM) or Right Stick (controller)  
**Input type**: Analog delta

**System Response**:
1. Camera yaw and pitch update at `LookSensitivity` rate (configurable)
2. Pitch clamped to configurable range (default: -60° to +80° from level)
3. Character facing **does NOT rotate with camera** — facing updates only on movement input
4. During execution cutscene: look input suppressed (camera-system-base.md Edge Case 5)

**Feedback Channels**:
- Visual: world rotates around player character (standard 3rd-person response)
- No HUD feedback for camera movement itself

**Edge Cases**:
- Overdrive onset: FOV lerp from 90° to 100° over 0.3s (camera-system-base.md FOV contract)
- Overdrive end: FOV lerp back to 90°
- Execution cutscene start: look input suppressed, camera driven by cutscene animation; player movement input **remains active** (player-movement.md Rule 11 — movement input survives but is effectively moot during the fixed-duration cutscene)

---

## 7. Cross-Pattern Interaction Rules

| Combination | Rule |
|---|---|
| Spell cast + movement | Both simultaneous — no conflict. Movement never locked during cast. |
| Dash + spell cast | Simultaneous allowed. Dash velocity + cast effect both apply. |
| Dash + jump | Simultaneous allowed. Dash direction takes priority for horizontal movement. |
| Execute (F) + dash input | Execute input processed first (if in range); dash input handled in next frame if F fails to trigger. |
| Overdrive active + all spells | All three spell gates bypassed; input handling identical (same keys, same timing, different gate result). |
| Two spells same frame | One cast per element per frame. If Blackhole (1) and Fire (2) pressed same frame: both may trigger if different elements. Implementation detail: process in key-press order (1 before 2). |

---

## 8. Input Response Budget

Per `player-movement.md` AC9: input → velocity latency ≤ 33ms p95.

| Action | Target Latency | Measurement Point |
|---|---|---|
| Directional input → character moves | ≤ 33ms p95 | Input event → first frame of movement |
| Spell cast input → damage applied | ≤ 1 frame (16.7ms at 60fps) | Input event → ApplyDamage call |
| Dash input → invulnerability active | ≤ 1 frame | Input event → State.Invulnerable grant |
| Dash input → velocity change | ≤ 1 frame | Input event → LaunchCharacter call |
| Overdrive trigger → CostBypass.Active | ≤ 1 frame | OnOverdriveTriggered → tag grant |
