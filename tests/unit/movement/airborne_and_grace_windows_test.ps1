$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$characterHeader = Join-Path $repoRoot "Moon/Source/Moon/Character/MoonCharacterBase.h"
$characterCpp = Join-Path $repoRoot "Moon/Source/Moon/Character/MoonCharacterBase.cpp"

function Assert-Matches {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Message
    )

    if ($Text -notmatch $Pattern) {
        throw $Message
    }
}

function Get-FunctionBody {
    param(
        [string]$Text,
        [string]$SignaturePattern
    )

    $match = [regex]::Match($Text, $SignaturePattern)
    if (-not $match.Success) {
        throw "Could not find function matching pattern: $SignaturePattern"
    }

    $start = $Text.IndexOf("{", $match.Index)
    if ($start -lt 0) {
        throw "Could not find opening brace for function matching pattern: $SignaturePattern"
    }

    $depth = 0
    for ($i = $start; $i -lt $Text.Length; $i++) {
        if ($Text[$i] -eq "{") {
            $depth++
        }
        elseif ($Text[$i] -eq "}") {
            $depth--
            if ($depth -eq 0) {
                return $Text.Substring($start, $i - $start + 1)
            }
        }
    }

    throw "Could not find closing brace for function matching pattern: $SignaturePattern"
}

# --- Runtime model mirrors (must match AMoonCharacterBase exactly) ---
#
# JumpInputBufferTimer/CoyoteTimeTimer are REMAINING-time countdowns: armed at
# JumpGraceWindowSeconds (0.150s) and decremented by DeltaTime each tick, so a timer's stored
# value after E seconds have elapsed since arming is (JumpGraceWindowSeconds - E), NOT E itself.
# Injecting the AC's raw millisecond values (0.149, 0.150, 0.1505, 0.151) directly as the stored
# Timer value would test the wrong thing — those numbers are ELAPSED time, and the code checks
# REMAINING time. Get-RemainingTimerAfterElapsed performs that conversion so the boundary test
# below verifies what the runtime actually holds in the field at each elapsed instant.
$Script:GraceWindowSeconds = 0.150
$Script:UnarmedTimerSentinel = -1.0

function Get-RemainingTimerAfterElapsed {
    param([double]$ElapsedSeconds)
    return $Script:GraceWindowSeconds - $ElapsedSeconds
}

# Mirrors AMoonCharacterBase::IsWithinGraceWindow(float) exactly:
# TimerSeconds >= 0.0f && TimerSeconds <= JumpGraceWindowSeconds.
function Test-WithinGraceWindow {
    param([double]$TimerSeconds)
    return ($TimerSeconds -ge 0.0) -and ($TimerSeconds -le $Script:GraceWindowSeconds)
}

# --- AC-1: Falling -> Ascending re-entry (TR-mov-003) ---
function test_tick_derives_airborne_substate_from_velocity_z_sign_after_super_tick {
    param([string]$Cpp)

    # Arrange
    $tickBody = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::Tick\s*\(\s*float\s+DeltaTime\s*\)"

    # Act
    $superTickMatch = [regex]::Match($tickBody, "Super::Tick\s*\(\s*DeltaTime\s*\)\s*;")
    $subStateMatch = [regex]::Match($tickBody, "AirborneSubState\s*=\s*\([^)]*Velocity\.Z\s*>\s*0\.0f[^)]*\)\s*\?\s*EMoonAirborneSubState::Ascending\s*:\s*EMoonAirborneSubState::Falling\s*;")

    # Assert
    if (-not $superTickMatch.Success) {
        throw "Tick() must call Super::Tick(DeltaTime)."
    }
    if (-not $subStateMatch.Success) {
        throw "Tick() must derive AirborneSubState from a Velocity.Z > 0 sign check (Ascending/Falling)."
    }
    if ($subStateMatch.Index -le $superTickMatch.Index) {
        throw "AirborneSubState derivation must occur AFTER Super::Tick() so this frame's CMC result is visible (stale-value risk)."
    }
}

function test_airborne_substate_never_uses_a_custom_movement_mode {
    param([string]$Cpp)

    # Arrange / Act
    $usesForbiddenApi = $Cpp -match "SetMovementModeWithCustomMode"

    # Assert
    if ($usesForbiddenApi) {
        throw "Airborne substate must never use SetMovementModeWithCustomMode() (ADR-0009 Decision 2 / Alternative 3 rejection)."
    }
}

# --- AC-2/AC-3: coyote time / jump buffer inclusive <=150ms elapsed boundary (TR-mov-007) ---
function test_grace_window_constant_and_check_use_the_documented_inclusive_operators {
    param([string]$Header)

    # Assert (the header IS the arrange/act here — verifying the exact operators mandated by the
    # ADR/story are the ones actually written into the boundary check, not just any semantically
    # equivalent form)
    Assert-Matches $Header "static\s+constexpr\s+float\s+JumpGraceWindowSeconds\s*=\s*0\.150f\s*;" "JumpGraceWindowSeconds must be exactly 0.150f."
    Assert-Matches $Header "static\s+constexpr\s+float\s+UnarmedTimerSentinel\s*=\s*-1\.0f\s*;" "UnarmedTimerSentinel must be a distinct negative value, never reachable by a real countdown from 0.150f to 0."
    Assert-Matches $Header "TimerSeconds\s*>=\s*0\.0f\s*&&\s*TimerSeconds\s*<=\s*JumpGraceWindowSeconds" "The grace-window check must be an inclusive [0.0f, JumpGraceWindowSeconds] range — NOT a bare > 0.0f check, which would wrongly reject the exact-150ms-elapsed (remaining == 0.0f) case."
}

function test_grace_window_boundary_149ms_and_150ms_elapsed_pass_1505ms_and_151ms_elapsed_fail {
    # Arrange: convert each AC elapsed-time scenario into the REMAINING value the runtime would
    # actually be holding in JumpInputBufferTimer/CoyoteTimeTimer at that instant.
    $remainingAt149msElapsed = Get-RemainingTimerAfterElapsed -ElapsedSeconds 0.149
    $remainingAt150msElapsed = Get-RemainingTimerAfterElapsed -ElapsedSeconds 0.150
    $remainingAt1505msElapsed = Get-RemainingTimerAfterElapsed -ElapsedSeconds 0.1505
    $remainingAt151msElapsed = Get-RemainingTimerAfterElapsed -ElapsedSeconds 0.151

    # Act
    $result149 = Test-WithinGraceWindow -TimerSeconds $remainingAt149msElapsed
    $result150 = Test-WithinGraceWindow -TimerSeconds $remainingAt150msElapsed
    $result1505 = Test-WithinGraceWindow -TimerSeconds $remainingAt1505msElapsed
    $result151 = Test-WithinGraceWindow -TimerSeconds $remainingAt151msElapsed

    # Assert
    if (-not $result149) {
        throw "149ms elapsed (remaining=$remainingAt149msElapsed) must pass the grace window boundary check."
    }
    if (-not $result150) {
        throw "150ms elapsed (remaining=$remainingAt150msElapsed) must pass the grace window boundary check (inclusive boundary)."
    }
    if ($result1505) {
        throw "150.5ms elapsed (remaining=$remainingAt1505msElapsed) must fail the grace window boundary check."
    }
    if ($result151) {
        throw "151ms elapsed (remaining=$remainingAt151msElapsed) must fail the grace window boundary check."
    }
}

# Regression test for the exact bug flagged by review: a naive `TimerSeconds > 0.0f` lower bound
# would reject remaining == 0.0f (the exact-150ms-elapsed case), even though AC-2/AC-3 explicitly
# require 150ms elapsed to PASS. This test fails under the old `> 0.0f` formula and passes under
# the corrected `>= 0.0f` formula, so it would have caught the original bug.
function test_exact_150ms_elapsed_boundary_passes_not_just_values_strictly_below_it {
    # Arrange
    $remainingAtExactly150msElapsed = Get-RemainingTimerAfterElapsed -ElapsedSeconds 0.150

    # Act
    $isWithinWindow = Test-WithinGraceWindow -TimerSeconds $remainingAtExactly150msElapsed

    # Assert
    if ($remainingAtExactly150msElapsed -ne 0.0) {
        throw "Test setup assumption broken: remaining at exactly 150ms elapsed should be 0.0, was $remainingAtExactly150msElapsed."
    }
    if (-not $isWithinWindow) {
        throw "REGRESSION: remaining == 0.0f (exactly 150ms elapsed) must PASS the grace window check. A lower bound of '> 0.0f' instead of '>= 0.0f' would wrongly fail this exact boundary case."
    }
}

function test_unarmed_sentinel_fails_the_grace_window_check {
    # Arrange / Act
    $isWithinWindow = Test-WithinGraceWindow -TimerSeconds $Script:UnarmedTimerSentinel

    # Assert
    if ($isWithinWindow) {
        throw "The never-armed sentinel ($($Script:UnarmedTimerSentinel)) must fail the grace window boundary check."
    }
}

function test_timers_default_to_the_unarmed_sentinel_not_zero {
    param([string]$Header)

    # Assert: 0.f can no longer be the default/reset value (see UnarmedTimerSentinel comment) —
    # both fields must default to the sentinel constant.
    Assert-Matches $Header "float\s+JumpInputBufferTimer\s*=\s*UnarmedTimerSentinel\s*;" "JumpInputBufferTimer must default to UnarmedTimerSentinel, not 0.f (0.f is a valid passing remaining value at the exact 150ms boundary)."
    Assert-Matches $Header "float\s+CoyoteTimeTimer\s*=\s*UnarmedTimerSentinel\s*;" "CoyoteTimeTimer must default to UnarmedTimerSentinel, not 0.f (0.f is a valid passing remaining value at the exact 150ms boundary)."
}

function test_timers_only_count_down_while_positive {
    param([string]$Cpp)

    # Arrange — extracted from Tick() into UpdateJumpTimers() during the 2026-08-12 /code-review
    # pass (same call-per-frame contract, verified by the Tick()-calls-it check below).
    $timersBody = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::UpdateJumpTimers\s*\(\s*float\s+DeltaTime\s*\)"

    # Assert
    Assert-Matches $timersBody "if\s*\(\s*JumpInputBufferTimer\s*>\s*0\.0f\s*\)\s*\{\s*JumpInputBufferTimer\s*=\s*FMath::Max\s*\(\s*JumpInputBufferTimer\s*-\s*DeltaTime\s*,\s*UnarmedTimerSentinel\s*\)\s*;" "JumpInputBufferTimer must count down via -= DeltaTime only while > 0, clamped at UnarmedTimerSentinel."
    Assert-Matches $timersBody "if\s*\(\s*CoyoteTimeTimer\s*>\s*0\.0f\s*\)\s*\{\s*CoyoteTimeTimer\s*=\s*FMath::Max\s*\(\s*CoyoteTimeTimer\s*-\s*DeltaTime\s*,\s*UnarmedTimerSentinel\s*\)\s*;" "CoyoteTimeTimer must count down via -= DeltaTime only while > 0, clamped at UnarmedTimerSentinel."

    $tickBody = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::Tick\s*\(\s*float\s+DeltaTime\s*\)"
    Assert-Matches $tickBody "UpdateJumpTimers\s*\(\s*DeltaTime\s*\)\s*;" "Tick() must call UpdateJumpTimers(DeltaTime) once per frame."
}

function test_coyote_time_is_armed_only_when_leaving_ground_without_jumping {
    param([string]$Cpp)

    # Arrange — extracted from Tick() into UpdateJumpAnimState() during the 2026-08-12 /code-review
    # pass (same call-per-frame contract, verified by the Tick()-calls-it check below).
    $jumpAnimBody = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::UpdateJumpAnimState\s*\(\s*float\s+DeltaTime\s*\)"

    # Assert
    Assert-Matches $jumpAnimBody "JumpCurrentCount\s*==\s*0\s*\)\s*\{\s*CoyoteTimeTimer\s*=\s*JumpGraceWindowSeconds\s*;" "CoyoteTimeTimer must only be armed (to JumpGraceWindowSeconds) when JumpCurrentCount==0 (left ground without jumping)."

    $tickBody = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::Tick\s*\(\s*float\s+DeltaTime\s*\)"
    Assert-Matches $tickBody "UpdateJumpAnimState\s*\(\s*DeltaTime\s*\)\s*;" "Tick() must call UpdateJumpAnimState(DeltaTime) once per frame."
}

function test_jump_input_buffer_armed_while_airborne_and_consumed_on_landing {
    param([string]$Cpp)

    # Arrange
    $inputJumpBody = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::Input_Jump\s*\(\s*\)"
    $landedBody = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::Landed\s*\(\s*const\s+FHitResult&\s*Hit\s*\)"

    # Assert
    Assert-Matches $inputJumpBody "JumpInputBufferTimer\s*=\s*JumpGraceWindowSeconds\s*;" "Input_Jump() must arm JumpInputBufferTimer while airborne."
    Assert-Matches $landedBody "IsWithinGraceWindow\s*\(\s*JumpInputBufferTimer\s*\)" "Landed() must consume JumpInputBufferTimer through the shared grace-window boundary check."
    Assert-Matches $landedBody "JumpInputBufferTimer\s*=\s*UnarmedTimerSentinel\s*;" "Landed() must reset JumpInputBufferTimer to UnarmedTimerSentinel (not 0.0f) after consuming it, so it cannot be re-consumed as a false-passing 'exact boundary' value on a later frame."
    Assert-Matches $landedBody "Jump\s*\(\s*\)\s*;" "Landed() must trigger a jump when a buffered jump input is within the grace window."
}

function test_coyote_jump_bypasses_native_canjump_grounded_gate {
    param([string]$Cpp)

    # Arrange
    $inputJumpBody = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::Input_Jump\s*\(\s*\)"

    # Assert
    Assert-Matches $inputJumpBody "IsWithinGraceWindow\s*\(\s*CoyoteTimeTimer\s*\)" "Input_Jump() must consume CoyoteTimeTimer through the shared grace-window boundary check."
    Assert-Matches $inputJumpBody "CoyoteTimeTimer\s*=\s*UnarmedTimerSentinel\s*;" "Input_Jump() must reset CoyoteTimeTimer to UnarmedTimerSentinel (not 0.0f) after consuming it, for the same false-passing-boundary reason as Landed()."
    # Native ACharacter::CanJump()/Jump() refuses when JumpCurrentCount==0 while airborne, so the
    # coyote-time jump must not rely on a plain Jump() call to actually launch the character.
    Assert-Matches $inputJumpBody "LaunchCharacter\s*\(" "Coyote-time jump must bypass the native grounded-jump gate directly (LaunchCharacter), since Jump()/CanJump() refuses JumpCurrentCount==0 while airborne."
}

$header = Get-Content -LiteralPath $characterHeader -Raw
$cpp = Get-Content -LiteralPath $characterCpp -Raw

test_tick_derives_airborne_substate_from_velocity_z_sign_after_super_tick $cpp
test_airborne_substate_never_uses_a_custom_movement_mode $cpp
test_grace_window_constant_and_check_use_the_documented_inclusive_operators $header
test_grace_window_boundary_149ms_and_150ms_elapsed_pass_1505ms_and_151ms_elapsed_fail
test_exact_150ms_elapsed_boundary_passes_not_just_values_strictly_below_it
test_unarmed_sentinel_fails_the_grace_window_check
test_timers_default_to_the_unarmed_sentinel_not_zero $header
test_timers_only_count_down_while_positive $cpp
test_coyote_time_is_armed_only_when_leaving_ground_without_jumping $cpp
test_jump_input_buffer_armed_while_airborne_and_consumed_on_landing $cpp
test_coyote_jump_bypasses_native_canjump_grounded_gate $cpp

Write-Host "airborne substate / jump buffer / coyote time unit checks passed"
