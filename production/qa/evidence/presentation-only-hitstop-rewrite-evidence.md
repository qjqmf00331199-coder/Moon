# Test Evidence: Presentation-Only Hitstop Rewrite (Story 004)

> **Epic**: Player Movement Foundation Fixes
> **Story**: `production/epics/player-movement-foundation-fixes/story-004-presentation-only-hitstop-rewrite.md`
> **Requirement**: TR-mov-008 / ADR-0009 Decision 5 (V-1) / `player-movement.md` Core Rule 9
> **Story Type**: Visual/Feel — evidence below is **advisory**, not a blocking merge gate, per
> `.claude/docs/coding-standards.md`'s Testing Standards table. The static check in this story
> (`tests/static/hitstop_no_time_dilation_check.ps1`) is a separate, blocking, automated piece —
> see that script for the automated result.

## What changed

`AMoonCharacterBase::TriggerHitStop()` / `EndHitStop()` were rewritten from a `CustomTimeDilation`
implementation to a capture-and-blend presentation freeze, and a new per-tick step
(`UpdateHitStopPresentation()`, called from `Tick()`) was added to drive it. No other function's
logic was changed except `Landed()`'s call-site ordering (see "Landed() ordering change" below).

### Why the old approach was wrong

The previous implementation set `CustomTimeDilation = FMath::Clamp(DilationScale, 0.001f, 1.0f)`
on the actor and restored it to `1.0f` via a timer. `CustomTimeDilation` dilates the **actor's own
Tick/AnimInstance update rate** — during "hitstop," `AMoonCharacterBase::Tick()` itself (including
`Super::Tick()`, which drives `CharacterMovementComponent`) would have been running at ~5% of
normal speed for the freeze duration. This directly violates player-movement.md Core Rule 9
("어떤 형태의 Time Dilation도 사용하지 않음") and ADR-0009 Decision 5 / finding V-1: gameplay
movement itself was being slowed by "hitstop," not just its visual presentation — exactly the
"judgment and presentation are different pipelines" invariant Core Rules 8/9/11 all state.

### The replacement mechanism

1. **Trigger** (`TriggerHitStop()`): captures `FreezeStartMeshTransform` (the mesh's world
   transform) and `CapturedMeshRelativeTransform` (the mesh's relative-to-Capsule offset, captured
   only when starting from a fully natural/non-frozen state — see "Re-trigger correctness" below).
   Sets `GetMesh()->bPauseAnims = true` to hold anim playback. Starts a world-clock `FTimerHandle`
   (`HitStopTimerHandle`) for `RealDuration` — this timer is **not** dilated by anything, because
   nothing in this path dilates time at all.
2. **Freezing** (`UpdateHitStopPresentation()`, called every `Tick()`, after `Super::Tick()`):
   forces `GetMesh()->SetWorldTransform(FreezeStartMeshTransform, ...)` every tick. The Capsule
   (driven by `CharacterMovementComponent` at 100% normal tick rate) keeps moving underneath —
   this is what makes the freeze visual-only. `Super::Tick()` and all of `AMoonCharacterBase`'s
   other per-tick logic (airborne substate, jump buffer/coyote timers, movement input) are
   completely unaffected.
3. **Unfreeze** (`EndHitStop()`, fired by the world-clock timer): resumes anim playback
   (`bPauseAnims = false`) and transitions to `BlendingOut` — it does **not** instantly restore the
   transform.
4. **BlendingOut** (`UpdateHitStopPresentation()`): interpolates the mesh from the held freeze
   transform toward its **natural** (Capsule-following) transform using `FMath::VInterpTo`
   (location) and `FQuat::Slerp` (rotation, delta-time-scaled alpha) — a fast blend, not an instant
   snap, matching Core Rule 9's explicit requirement to avoid the "camera/character snaps on
   unfreeze" artifact. The blend ends either when the distance to target is below
   `KINDA_SMALL_NUMBER`, or when `HitStopBlendElapsed >= HitStopBlendOutDuration` (default
   `0.033f`, "1-2 frames" per the ADR/GDD) — the elapsed-time cap is required because the blend
   *target* keeps moving (the Capsule never stopped), so a pure distance-convergence check would
   settle at a nonzero steady-state lag and never fire on its own.
5. On blend completion, `GetMesh()->SetRelativeTransform(CapturedMeshRelativeTransform)` restores
   the mesh's exact original relative offset (see "Re-trigger correctness" below), and
   `HitStopPhase` returns to `Inactive` — the mesh follows the Capsule normally again from the next
   tick onward.

### Re-trigger correctness (self-review finding, not in the original ADR text)

`SetWorldTransform()` on an attached component (the mesh is attached to the Capsule) recomputes
and overwrites the component's *stored relative transform* every time it's called. If
`TriggerHitStop()` naively recaptured `CapturedMeshRelativeTransform` on every call — including a
re-trigger while a previous hitstop is still Freezing/BlendingOut (e.g. dash-cancelling into a
second hitstop mid-freeze, which is exactly the AC-3 test scenario) — it would capture an
already-corrupted transient relative offset as "the original," permanently baking in whatever
positional drift existed at that moment and accumulating further on every subsequent hitstop.
`TriggerHitStop()` therefore only (re)captures `CapturedMeshRelativeTransform` when
`HitStopPhase == Inactive` (a fully natural, non-frozen state); a re-trigger only refreshes
`FreezeStartMeshTransform` (where to hold visually) and keeps the last known-good relative offset
as the eventual restore target. This is the mechanism that guarantees zero residual mesh drift
across repeated hitstops.

### `Landed()` ordering change

The previous code called `TriggerHitStop(0.055f)` **before** setting up `JumpLandAnim`, so the
pose held during the freeze was the pre-landing airborne pose, and the landing clip (with its
notifies) only started playing after the freeze ended. This call was moved to **after** the
`JumpLandAnim` setup block, so the pose frozen during hitstop is the landing-impact pose itself —
matching the design intent ("sell impact on landing") and reducing (though not eliminating —
see AC-4 below) the AnimNotify-slip risk, since the landing anim is now already selected and
`bPauseAnims` is applied to it directly rather than to whatever was playing beforehand. This is a
one-line, hitstop-scoped reordering; the jump-input-buffer logic above it (Story 003) was not
touched or reordered.

### `Dash` call site

`Moon/Source/Moon/GAS/MoonGameplayAbility_Dash.cpp:211` (`Character->TriggerHitStop(0.055f);`) was
**not modified**. It only ever passed one argument, so removing `TriggerHitStop`'s now-unused
`DilationScale` default parameter does not change this call site's compilation or semantics.

### Blueprint reference check

`TriggerHitStop` is `BlueprintCallable`. Searched `Moon/Content/Moon` (`*.uasset`, recursive) for
literal references to `TriggerHitStop` — **none found**. A full-`Moon/Content` grep (including
`Free_Magic`/`Free_Spells`, large third-party marketplace packs) repeatedly timed out (ripgrep
20s limit) rather than returning a clean negative result; this is a tooling limitation, not a
positive finding, and is reported as an open item rather than a false "confirmed clean." Given
`Free_Magic`/`Free_Spells` are vendor VFX asset packs with no plausible reason to reference a
project-specific gameplay function, risk is assessed as low, but this was not exhaustively
confirmed.

## Deferred to human reviewer (cannot be automated)

Per `.claude/docs/coding-standards.md`, this story is Type: Visual/Feel — the following require an
actual PIE/build playtest and are **not** faked here:

- **AC-1 / Velocity continuity**: automated static check confirms `Tick()` calls
  `UpdateHitStopPresentation()` and that the function never writes
  `CharacterMovementComponent::Velocity` or calls `SetActorLocation`/`SetActorTransform` — but
  actually capturing a real velocity trace during a landing hitstop vs. a non-hitstop control
  frame (the GDD's own Insights-trace-based positive assertion, `player-movement.md` line ~265)
  requires a running build and was not performed in this session.
- **AC-3 / Visual artifact (pose sliding, snap-back)**: requires a human reviewer to trigger
  landing hitstop (and dash-cancel/move during the freeze) in PIE, capture a clip at normal and
  slow playback, and confirm no visible pose sliding or unfreeze pop. **Not performed — deferred.**
  Reviewer checklist:
  1. Trigger a landing while holding movement input in a direction; confirm the character visually
     freezes in place for ~55ms while camera/scene context continues normally.
  2. During that freeze window, dash-cancel or continue moving; confirm no visible "ghosting" or
     the mesh sliding away from its frozen pose.
  3. At unfreeze, confirm the mesh eases back into its real position over a couple of frames
     rather than popping/snapping instantly.
  4. Repeat 3-5 times in a row (rapid landings) to check for any accumulated positional drift in
     the mesh relative to the Capsule — this is the specific regression the re-trigger-correctness
     fix above is meant to prevent.
- **AC-4 / AnimNotify slip**: explicitly advisory/non-blocking per the story's own text
  ("Also add ... AnimNotify landing SFX timing (mentioned in AC-4) is advisory-only per the story
  — note it in your report, don't build new sound-scheduling infrastructure for it"). No new
  sound-scheduling infrastructure was added. The `Landed()` reordering above (hitstop now fires
  after `JumpLandAnim` is selected, and `bPauseAnims` applies to the landing clip itself rather
  than a stale prior clip) is a partial, incidental mitigation, not a fix — any `AnimNotify` placed
  early inside `JumpLandAnim`'s freeze-covered window will still not fire until `bPauseAnims`
  clears. `sound-designer`/`technical-artist` should confirm at implementation time whether landing
  SFX notifies need to be placed after the freeze window, or scheduled independently of the anim
  timeline, per the GDD's own note (Core Rule 9, final sentence).

## Camera / ADR-0005 cross-check (not a scope violation — documented per the story's instructions)

Per this story's Implementation Notes ("cross-check ADR-0005 before implementing") and ADR-0009's
Risks section, `ADR-0005-camera-system-springarm.md` was read in full before implementing. The
camera (`CameraBoom`/`FollowCamera`) is **not** frozen or otherwise touched by this rewrite — only
`GetMesh()`'s world transform and anim-pause state are affected. This was a deliberate decision,
not an oversight:

- ADR-0009 Decision 5 itself frames camera freeze as conditional ("포즈/카메라... 캡처... 카메라도
  얼릴 경우") — not mandatory.
- The `SpringArm`'s existing camera lag (`CameraLagSpeed=18.0`, `bEnableCameraLag=true`) already
  smooths Capsule motion for the camera; since the camera continues following the Capsule normally
  (undilated, untouched) throughout the freeze, no new interaction with `ResetCameraLag()` or lag
  tuning is introduced by this change.
- AC-1 (velocity continuity) and AC-2 (no Time Dilation) do not require touching the camera at all.
- AC-3 (visual artifact) is scoped to "pose sliding and snap-back" on the character mesh; whether
  the camera *also* needs a freeze/compensation pass is exactly the open question ADR-0009's Risks
  section flags for future cross-check with ADR-0005's owner — this session did not determine that
  a camera-side change is required, so per the story's own instruction ("if you find you need to
  touch anything under ADR-0005's ownership... STOP and report it as a scope question"), no stop
  was triggered and no camera/SpringArm code was modified.
- If the deferred human playtest above (AC-3) finds the camera itself needs freeze compensation,
  that is a follow-up cross-check with ADR-0005's owner, not something to silently add to this
  story.

## Pre-existing discrepancy noted, not fixed (out of scope)

`player-movement.md`'s Tuning Knobs table specifies a 40ms landing hitstop duration, but both call
sites (`Landed()` and `MoonGameplayAbility_Dash.cpp`'s `OnDashFinished()`) pass `0.055f` (55ms).
This mismatch predates this story (it was already present in the shipped `CustomTimeDilation`
implementation) and is not something this story's scope (TR-mov-008 / the freeze *mechanism*)
covers — it is a numeric tuning value, not a mechanism, and was left unchanged.

## Engine-API uncertainty flagged (UE 5.8, LLM knowledge cutoff May 2025)

Per `docs/engine-reference/unreal/VERSION.md`, the following APIs used in this rewrite are
long-stable (pre-date the cutoff by multiple years) but are **not documented in this project's
curated `docs/engine-reference/unreal/` library**, so they are flagged as unverified against real
UE5.8 headers specifically — the same treatment `docs/architecture/0005-camera-system-springarm.md`
already applies to `bUseCameraLagSubstepping`:

- `USkeletalMeshComponent::bPauseAnims` — used to hold anim playback during the freeze. If this
  needs re-verifying at compile time and doesn't behave as expected, the documented fallback (noted
  in-code) is `GetMesh()->GetSingleNodeInstance()->SetPlaying(false)`, since this character's
  locomotion already goes through the single-node anim path (`Animation/AnimSingleNodeInstance.h`
  is already included in this file).
- `USceneComponent::SetWorldTransform()` / `GetRelativeTransform()` / `SetRelativeTransform()` /
  `GetAttachParent()` — standard, long-stable `SceneComponent` API, not previously used anywhere
  else in this codebase (first usage introduced by this story).
- `FTransform::operator*` composition order (`Relative * ParentWorld = World`) — standard UE
  convention, not version-specific, unaffected by any 5.4-5.8 change documented in
  `breaking-changes.md`/`deprecated-apis.md`.
- `FMath::VInterpTo`, `FQuat::Slerp` — long-stable Core math utilities, not flagged in either
  reference doc.

None of the above appear in `docs/engine-reference/unreal/breaking-changes.md` or
`deprecated-apis.md`'s HIGH/MEDIUM risk sections. Risk is assessed as **low** (stable pre-cutoff
APIs) but formally **unverified against this project's own UE5.8 header set** — a compile pass in
the actual engine is the remaining verification step, not performed in this session (no engine
build was run).

## Automated static test result

`tests/static/hitstop_no_time_dilation_check.ps1` — **PASS**. Verifies (negative) zero
`CustomTimeDilation`/`SetGlobalTimeDilation` references in `MoonCharacterBase.h`/`.cpp`, and
(positive, per the GDD's own note that a negative-only grep was insufficient) that
`TriggerHitStop()` captures the mesh transform and pauses anims, `EndHitStop()` starts a blend
rather than an instant restore, `Tick()` calls the per-tick presentation step every frame, and
`UpdateHitStopPresentation()` only ever moves the mesh component (never the actor/Capsule, never
`CharacterMovementComponent::Velocity` directly).

Regression checks (Story 001/003, same file touched): `movement_foundation_contract.ps1`,
`movement_independence_check.ps1`, `airborne_and_grace_windows_test.ps1`,
`movement_lock_contract_test.ps1`, `camera_yaw_facing_test.ps1` — all **PASS**, unchanged.
