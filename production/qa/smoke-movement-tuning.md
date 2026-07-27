# Smoke Check — Movement Tuning Clamp & AirTime Joint Bound (Story 002)

> **Story**: `production/epics/player-movement-foundation-fixes/story-002-data-driven-movement-tuning-and-clamp-enforcement.md`
> **Date**: 2026-07-27
> **Type**: Config/Data smoke check (kept per the story's Test Evidence section even though the
> story was reclassified Config/Data → Logic — the reclassification added a BLOCKING automated-test
> requirement, it did not remove this advisory note).
> **Automated evidence (BLOCKING)**: `tests/unit/movement/tuning_clamp_and_joint_bound_test.ps1` — PASS (see report).

## What was checked

- `AMoonCharacterBase::ValidateAndClampMovementTuning()` (called once from `BeginPlay()`) clamps
  `MaxWalkSpeed`, `JumpZVelocity`, `GravityScale`, `MaxAcceleration`, `BrakingDecelerationWalking`,
  `GroundFriction` on `GetCharacterMovement()` to the GDD's exact hard minimums (100 / 100 / 0.1 /
  1000 / 1000 / 1), with a `UE_LOG(LogTemp, Warning, ...)` on every clamp.
- The AirTime joint bound (`[0.5s, 3.0s]`) is validated after the individual clamps, using
  `AMoonCharacterBase::ComputeAirTime()` (`(2 * JumpZVelocity) / (GravityScale * abs(WorldGravityZ))`,
  `WorldGravityZ` from `GetWorld()->GetGravityZ()`, not the CMC's GravityScale-baked variant).
- Revert policy: on a joint-bound violation, both values revert to the stored
  `LastValidJumpZVelocity`/`LastValidGravityScale` pair (not to engine/GDD defaults) — except at
  bootstrap (no previous valid pair yet), which falls back to the GDD's own documented defaults
  (`JumpZVelocity=600`, `GravityScale=1.0`) as the last resort and logs an error.
- `AMoonCharacterBase::InjectZImpulse(float ZVelocity)` (TR-mov-005) added as a public,
  `BlueprintCallable` Z-axis impulse hook (`LaunchCharacter` with `bXYOverride=false`,
  `bZOverride=true`) — no caller wired up (Dash/Evasion, Arena Morphing epics own that).

## Manual/PIE verification

Not performed this pass — no compile/PIE run was executed as part of this story (C++-only change,
verified via the static/unit PowerShell suite above; UBT compile not run in this session). Recommend
a full compile + one BeginPlay in PIE with the log filter `MoonMovementTuning` before merging, to
confirm the log lines fire as expected against the actual `BP_MoonCharacter` CDO values and that no
compile errors exist in the new code paths.

## Gap found during review, fixed same session (user-approved scope extension)

`AMoonCharacterBase::Tick()`'s pre-existing (Story 003/004-era) asymmetric jump-feel effect used to
unconditionally **overwrite** `GetCharacterMovement()->GravityScale` every frame:

```cpp
const bool bDescending = MoveComp->IsFalling() && GetVelocity().Z < 0.0f;
MoveComp->GravityScale = bDescending ? FallingGravityScaleMultiplier : 1.0f;
```

Its own doc comment said "multiplies CharacterMovement's base GravityScale" but the code assigned,
not multiplied — discarding whatever `ValidateAndClampMovementTuning()` had just clamped/joint-bound
-validated, meaning the guarantee only held for the instant between `BeginPlay()` and the first
`Tick()`. Flagged during implementation review; the user reviewed the exact code and approved
extending this story's scope to fix it rather than deferring:

- Added a `BaseGravityScale` member — the actual TR-mov-004-validated source of truth, distinct from
  the live, Tick()-derived `GetCharacterMovement()->GravityScale`.
- `Tick()` now computes `MoveComp->GravityScale = BaseGravityScale * (bDescending ?
  FallingGravityScaleMultiplier : 1.0f)` — a real multiply, matching the original comment's intent
  and never discarding the validated base.
- `FallingGravityScaleMultiplier` is now itself clamped to `MinGravityScale` (0.1) in
  `ValidateAndClampMovementTuning()`, since an unclamped multiplier could still zero/invert descent
  gravity even with a safe base value.
- Default-tuning numeric behavior is unchanged (`BaseGravityScale` defaults to `1.0`, so
  `1.0 * FallingGravityScaleMultiplier` reproduces the old output exactly for the shipped default
  config) — this is a structural fix, not a feel/tuning change.
- Regression coverage added: `test_tick_multiplies_base_gravity_scale_instead_of_overwriting_it`,
  `test_falling_gravity_scale_multiplier_is_clamped`,
  `test_base_gravity_scale_is_the_validated_source_of_truth` in
  `tests/unit/movement/tuning_clamp_and_joint_bound_test.ps1` — all PASS.

## Result

**PASS** — automated test suite green (including the three new regression tests above), full UBT
build succeeded. No PIE runtime verification performed this session (no Unreal MCP/Editor control
tool available) — recommend one BeginPlay pass in PIE with the log filter `MoonMovementTuning`
before considering this fully closed, to confirm the log lines fire as expected against the actual
`BP_MoonCharacter` CDO values.
