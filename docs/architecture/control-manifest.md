# Control Manifest

> **Engine**: Unreal Engine 5.8
> **Last Updated**: 2026-07-27
> **Manifest Version**: 2026-07-27
> **ADRs Covered**: ADR-0001, ADR-0002, ADR-0003, ADR-0004, ADR-0005, ADR-0006, ADR-0007, ADR-0008, ADR-0009, ADR-0010, ADR-0011
> **Status**: Active - regenerate with `/create-control-manifest update` when ADRs change

This manifest is a programmer quick-reference extracted from Accepted ADRs, `AGENTS.md`, and `docs/engine-reference/unreal/`. `.Codex/docs/technical-preferences.md` is not present in this repository, so naming and commit conventions come from `AGENTS.md`.

---

## Foundation Layer Rules

*Applies to: Player Movement, Health/Damage Core, Runtime Checkpoint.*

### Required Patterns
- Player movement must use camera-relative input, diagonal magnitude normalization, and camera-yaw facing with `bUseControllerRotationYaw=true`, `bOrientRotationToMovement=false`, `bUseControllerRotationPitch=false`, and `bUseControllerRotationRoll=false` - source: ADR-0001, ADR-0005.
- Player movement must remain compile-independent of Spell Casting. Movement-owning code must not include or reference Spell Casting types or state - source: ADR-0009.
- Ascending/Falling are derived from `Velocity.Z` after the CMC tick. Do not store them as custom movement modes - source: ADR-0009.
- `MovementLocked` is owned only by the future Status Effect system. Until that ADR exists, keep the write path private and uncalled - source: ADR-0009.
- Jump input buffer and coyote time are character-owned float timers, delta-time based, with the inclusive `<= 150ms` boundary - source: ADR-0009.
- Hitstop and execution freeze are presentation-only. Capture mesh/camera transforms, keep capsule and CMC ticking normally, then blend back in 1-2 frames - source: ADR-0009.
- Health, MaxHealth, Mana, MaxMana, ManaRegenRate, and TensionGauge live in the central GAS AttributeSet - source: ADR-0001.
- Damage must enter through the Health/Damage Core GameplayEffect pipeline; Death is detected exactly once by Health/Damage Core and exposed through `OnDeath` - source: ADR-0001, ADR-0008.
- Checkpoint restore must call `UMoonAttributeSet::ResetDeathState()` before applying the Health restore GameplayEffect - source: ADR-0002, ADR-0008.

### Forbidden Approaches
- Never use `SetGlobalTimeDilation` or actor `CustomTimeDilation` for hitstop, execution freeze, or movement presentation - source: ADR-0009.
- Never gate movement input from Spell Casting, execution presentation, locomotion montages, or hitstop - source: ADR-0009.
- Never use root-motion locomotion for jump, landing, dash, or basic movement translation - source: ADR-0009.
- Never write raw AttributeSet values during checkpoint restore; restore through the defined GE path - source: ADR-0002, ADR-0008.

### Performance Guardrails
- Movement logic must expose trace scopes for input-to-velocity latency and time-to-95-percent speed checks - source: ADR-0009.
- Movement tick cost must be measured before Production with the GDD's N>=100 actor provisional benchmark - source: player-movement.md, ADR-0009.

---

## Core Layer Rules

*Applies to: Camera System, Enemy AI, Spell Casting, Dash/Evasion.*

### Required Patterns
- Camera configuration must be data-asset driven through `UMoonCameraSettings`; constructor literals are only safe fallbacks for editor preview - source: ADR-0005.
- Camera pitch clamp belongs to `AMoonPlayerCameraManager`, not CMC or SpringArm ad hoc code - source: ADR-0005.
- `ResetCameraLag()` must be called on teleport/checkpoint respawn paths - source: ADR-0005.
- Dash must use an instant swept `SetActorLocation` displacement step for horizontal movement; every dash covers the same distance regardless of prior momentum - source: ADR-0007.
- Air dash must apply the `AirDashZImpulse` tuning knob before the horizontal position step when falling - source: ADR-0007.
- Just-Dodge must query nearby enemies at dash activation time, evaluate telegraph/commit timing, grant `State.Executable` to every qualifying enemy, and refund exactly one dash charge per activation - source: ADR-0007.
- Enemy AI must expose `IsTelegraphingAttack()`, `GetAttackCommittedTime()`, and `MeleeAttackRange` for Dash Just-Dodge queries - source: ADR-0006, ADR-0007.
- Spell Casting must use GAS abilities, independent per-element cooldown tags, and same-frame activation/commit/end semantics - source: ADR-0003.
- Spell cooldown and HUD query surfaces must use the verified UE5.8 ASC cooldown tag/time APIs - source: ADR-0010.

### Forbidden Approaches
- Never reintroduce `LaunchCharacter` or velocity override for the dash's horizontal motion - source: ADR-0007.
- Never use the deprecated `UCharacterMovementComponent::SetMovementMode()` legacy overload in Dash; migrate to the non-deprecated path named in the engine reference - source: ADR-0007, `deprecated-apis.md`.
- Never keep camera tuning hardcoded as the runtime source of truth - source: ADR-0005.
- Never let Dash poll every enemy every frame; Just-Dodge queries run on dash activation only - source: ADR-0007.

### Performance Guardrails
- Just-Dodge costs one overlap query per dash activation, not a per-frame scan - source: ADR-0007.
- Camera SpringArm/FOV/shake work is presentation-side and must not gate movement, damage, or cast judgment - source: ADR-0005, architecture.md.

---

## Feature Layer Rules

*Applies to: Combo/Tension Gauge and Luna Overdrive.*

### Required Patterns
- Luna Overdrive has three states: Inactive, Active, Recovery. Active is a fixed 10s window; Recovery lasts 1.5s by default - source: ADR-0004.
- `CostBypass.Active` is owned only by Luna Overdrive. Spell Casting consumes the state but must not own it - source: ADR-0004.
- Combo/Tension gain, penalty, and decay must resolve deterministically in the ADR-0011 order, with current call sites ordered before `TG_PostUpdateWork` - source: ADR-0011.
- Tension gain is locked to zero while Overdrive is Active or Recovery - source: ADR-0004, ADR-0011.

### Forbidden Approaches
- Never infer Overdrive bypass from tag presence alone on the expiry frame; use the time-state contract - source: ADR-0004.
- Never let latent AbilityTasks or late Blueprint ticks call `AddTension*` after the resolve tick without explicit ordering review - source: ADR-0011, architecture.md.

---

## Presentation Layer Rules

*Applies to: Combat HUD, visual feedback, camera-only effects.*

### Required Patterns
- HUD is read-only presentation. It binds to upstream delegates, attributes, cooldown queries, dash charge state, Overdrive events, and executable tags, but exposes no gameplay-mutating API - source: ADR-0010.
- HUD updates are event-driven by default; ticking is allowed only for active interpolation or cooldown sweeps - source: ADR-0010.
- Input glyph swapping must use the verified UE5.8 unified Enhanced Input/CommonUI path - source: ADR-0010.

### Forbidden Approaches
- Never let interpolated HUD values trigger gameplay-meaningful state, prompts, or Overdrive transitions - source: ADR-0010.
- Never require HUD initialization before gameplay state can advance - source: architecture.md, ADR-0010.

---

## Global Rules

### Naming Conventions
| Element | Convention | Example |
|---|---|---|
| Classes | UE prefixes plus PascalCase | `AMoonCharacterBase`, `UMoonCameraSettings` |
| Variables | PascalCase; bools use `b` prefix | `DashCharges`, `bMovementLocked` |
| Events | PascalCase delegate/event names | `OnDeath`, `OnSpellHit` |
| Commits | Conventional Commits with task/story reference | `fix: align movement yaw flags (story-001)` |

### Performance Budgets
| Target | Value |
|---|---|
| Gameplay frame target | 60fps / 16.6ms frame budget |
| Movement overhead | GDD target <0.1ms per actor, pending hardware confirmation |
| UI idle tick | Zero tick when idle |

### Approved Libraries / Addons
- Enhanced Input - gameplay input.
- Gameplay Ability System - abilities, tags, attributes, costs, cooldowns.
- Lumen and Nanite - rendering stack.
- Niagara and MetaSounds - new VFX/audio work.
- CommonUI - HUD glyph/action presentation where applicable.

### Forbidden APIs (Unreal Engine 5.8)
- `InputComponent->BindAction()` / `BindAxis()` legacy input bindings - use Enhanced Input.
- `PlayerController->GetInputAxisValue()` legacy axis reads - use Enhanced Input action values.
- `UCharacterMovementComponent::SetMovementMode()` legacy overload - use the UE5.8 replacement path noted in `deprecated-apis.md`.
- Legacy GAS attribute set initialization functions - use the updated GAS init pattern after UE5.8 header verification.
- Cascade particle systems - use Niagara.
- Sound Cue for complex procedural logic - use MetaSounds.

### Cross-Cutting Constraints
- Gameplay values must be data-driven where GDD tuning knobs exist.
- Every implementation story must re-check UE5.8 engine references before relying on remembered API behavior.
- Stage only files that belong to the current task; leave unrelated dirty worktree files untouched.
