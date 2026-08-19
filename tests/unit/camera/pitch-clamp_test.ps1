$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$managerHeader = Join-Path $repoRoot "Moon/Source/Moon/Camera/MoonPlayerCameraManager.h"
$managerCpp = Join-Path $repoRoot "Moon/Source/Moon/Camera/MoonPlayerCameraManager.cpp"
$movementCpp = Join-Path $repoRoot "Moon/Source/Moon/Character/MoonCharacterBase.cpp"
$controllerCpp = Join-Path $repoRoot "Moon/Source/Moon/Character/MoonPlayerController.cpp"
$gameModeCpp = Join-Path $repoRoot "Moon/Source/Moon/Character/MoonGameMode.cpp"
$engineConfig = Join-Path $repoRoot "Moon/Config/DefaultEngine.ini"

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

    Assert-Matches $Cpp "IsWithinSafeRanges\(FailureReason\)" "Pitch settings must use the shared camera-settings validation contract."
    Assert-Matches $Cpp "EffectiveSettings\s*=\s*GetDefault<UMoonCameraSettings>\(\)" "Missing or invalid assets must fall back to safe class defaults."
    Assert-Matches $Cpp "ViewPitchMin\s*=\s*EffectiveSettings->CameraPitchMin\s*;" "AC-3: ViewPitchMin must be assigned from validated CameraSettings, not a literal."
    Assert-Matches $Cpp "ViewPitchMax\s*=\s*EffectiveSettings->CameraPitchMax\s*;" "AC-3: ViewPitchMax must be assigned from validated CameraSettings, not a literal."
}

function test_player_camera_manager_loads_shared_production_asset {
    param([string]$Cpp)

    Assert-Matches $Cpp '/Game/Moon/Camera/DA_MoonCameraSettings\.DA_MoonCameraSettings' "PlayerCameraManager must load the same production CameraSettings asset as AMoonCharacterBase."
}

function test_null_asset_guard_present {
    param([string]$Cpp)

    Assert-Matches $Cpp "if\s*\(\s*!IsValid\(EffectiveSettings\)\s*\)" "AC-3 edge case: ApplyPitchClamp must guard missing or stale settings before use."
}

function test_zero_look_input_is_a_noop {
    param([string]$MovementCpp)

    Assert-Matches $MovementCpp "LookAxisVector\.IsNearlyZero\(\)[\s\S]*return\s*;" "AC-1 edge case: zero look input must return before controller input calls."
}

function test_player_camera_manager_is_wired_at_runtime {
    param([string]$ControllerCpp, [string]$GameModeCpp, [string]$EngineConfig)

    Assert-Matches $ControllerCpp "PlayerCameraManagerClass\s*=\s*AMoonPlayerCameraManager::StaticClass\(\)" "MoonPlayerController must spawn AMoonPlayerCameraManager."
    Assert-Matches $GameModeCpp "PlayerControllerClass\s*=\s*AMoonPlayerController::StaticClass\(\)" "MoonGameMode must spawn AMoonPlayerController."
    Assert-Matches $EngineConfig "GlobalDefaultGameMode=/Script/Moon\.MoonGameMode" "DefaultEngine.ini must select MoonGameMode so the camera-manager wiring is active."
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
$controllerCpp = Get-Content -LiteralPath $controllerCpp -Raw
$gameModeCpp = Get-Content -LiteralPath $gameModeCpp -Raw
$engineConfig = Get-Content -LiteralPath $engineConfig -Raw

Assert-Matches $header "class\s+MOON_API\s+AMoonPlayerCameraManager\s*:\s*public\s+APlayerCameraManager" "AMoonPlayerCameraManager must subclass APlayerCameraManager."

test_pitch_clamp_sourced_from_camera_settings_not_hardcoded $cpp
test_player_camera_manager_loads_shared_production_asset $cpp
test_null_asset_guard_present $cpp
test_pitch_clamp_applied_via_playercameramanager_override_not_tick $cpp
test_pitch_never_clamped_in_cmc_or_springarm $movementCpp
test_zero_look_input_is_a_noop $movementCpp
test_player_camera_manager_is_wired_at_runtime $controllerCpp $gameModeCpp $engineConfig

Write-Host "pitch clamp static checks passed (AC-3 source-and-fallback verified; AC-2 boundary/no-overshoot behavior covered by the compiled Moon.Camera.PlayerCameraManager.PitchClampHoldsBoundary Automation test, not re-verified here)"
