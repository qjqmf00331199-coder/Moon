$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$managerHeader = Join-Path $repoRoot "Moon/Source/Moon/Camera/MoonPlayerCameraManager.h"
$managerCpp = Join-Path $repoRoot "Moon/Source/Moon/Camera/MoonPlayerCameraManager.cpp"
$movementCpp = Join-Path $repoRoot "Moon/Source/Moon/Character/MoonCharacterBase.cpp"

# NOTE on test framework (same convention established by Story 001's
# camera-settings-foundation_test.ps1): this repo's real compiled Automation Spec test for this
# story's AC-2/AC-3 is Moon/Source/Moon/Tests/MoonPlayerCameraManagerTests.cpp, which calls the
# actual engine LimitViewPitch() path with extreme-delta inputs and is the authoritative evidence
# for "clamp holds at the boundary, no overshoot on a single large-delta frame". The story's Test
# Evidence section names this file `pitch-clamp_test.cpp`; per Story 001's established sibling
# convention (nothing under repo-root tests/ compiles in this project), the stem is kept and the
# extension changed to `.ps1`. This file is a STATIC source-level regression guard only: it proves
# the clamp is sourced from CameraSettings (not hardcoded) and that the story's Forbidden Approach
# (clamping pitch in CMC/SpringArm) was not introduced — it does not execute the clamp math itself,
# that's the compiled test's job.

function Assert-Matches {
    param([string]$Text, [string]$Pattern, [string]$Message)
    if ($Text -notmatch $Pattern) {
        throw $Message
    }
}

function Assert-NotMatches {
    param([string]$Text, [string]$Pattern, [string]$Message)
    if ($Text -match $Pattern) {
        throw $Message
    }
}

function test_pitch_clamp_sourced_from_camera_settings_not_hardcoded {
    param([string]$Cpp)

    Assert-Matches $Cpp "ViewPitchMin\s*=\s*CameraSettings->CameraPitchMin\s*;" "AC-3: ViewPitchMin must be assigned from CameraSettings->CameraPitchMin, not a literal."
    Assert-Matches $Cpp "ViewPitchMax\s*=\s*CameraSettings->CameraPitchMax\s*;" "AC-3: ViewPitchMax must be assigned from CameraSettings->CameraPitchMax, not a literal."
}

function test_null_asset_guard_present {
    param([string]$Cpp)

    Assert-Matches $Cpp "if\s*\(\s*!CameraSettings\s*\)" "AC-3 edge case: ApplyPitchClamp must null-guard CameraSettings before use."
}

function test_pitch_clamp_applied_via_playercameramanager_override_not_tick {
    param([string]$Cpp)

    Assert-Matches $Cpp "void\s+AMoonPlayerCameraManager::InitializeFor\s*\(" "Pitch clamp must be wired through a PlayerCameraManager override (InitializeFor), not a bespoke Tick hack."
    Assert-NotMatches $Cpp "void\s+AMoonPlayerCameraManager::Tick\s*\(" "Story 002 explicitly forbids a Tick-based clamp hack — no Tick override should exist on this class for pitch clamping."
}

function test_pitch_never_clamped_in_cmc_or_springarm {
    param([string]$MovementCpp)

    # ADR-0005 Alternative 3 rejection: pitch clamp ownership must never leak into
    # CharacterMovementComponent or the SpringArm's ad hoc code.
    Assert-NotMatches $MovementCpp "CameraBoom->.*Pitch.*Clamp" "Forbidden (ADR-0005 Alternative 3): pitch clamp must not be applied to the SpringArm."
    Assert-NotMatches $MovementCpp "GetCharacterMovement\(\)->.*Pitch" "Forbidden (ADR-0005 Alternative 3): pitch clamp must not be applied via CharacterMovementComponent."
}

if (-not (Test-Path -LiteralPath $managerHeader)) {
    throw "AMoonPlayerCameraManager header not found at expected path: $managerHeader"
}

$header = Get-Content -LiteralPath $managerHeader -Raw
$cpp = Get-Content -LiteralPath $managerCpp -Raw
$movementCpp = Get-Content -LiteralPath $movementCpp -Raw

Assert-Matches $header "class\s+MOON_API\s+AMoonPlayerCameraManager\s*:\s*public\s+APlayerCameraManager" "AMoonPlayerCameraManager must subclass APlayerCameraManager."

test_pitch_clamp_sourced_from_camera_settings_not_hardcoded $cpp
test_null_asset_guard_present $cpp
test_pitch_clamp_applied_via_playercameramanager_override_not_tick $cpp
test_pitch_never_clamped_in_cmc_or_springarm $movementCpp

Write-Host "pitch clamp static checks passed (AC-3 source-and-fallback verified; AC-2 boundary/no-overshoot behavior covered by the compiled Moon.Camera.PlayerCameraManager.PitchClampHoldsBoundary Automation test, not re-verified here)"
