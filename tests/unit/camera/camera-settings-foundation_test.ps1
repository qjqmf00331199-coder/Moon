$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$characterHeader = Join-Path $repoRoot "Moon/Source/Moon/Character/MoonCharacterBase.h"
$characterCpp = Join-Path $repoRoot "Moon/Source/Moon/Character/MoonCharacterBase.cpp"
$settingsHeader = Join-Path $repoRoot "Moon/Source/Moon/Camera/MoonCameraSettings.h"

# NOTE on test framework (Story 001 implementation-time finding): the story's Test Evidence
# section names this file `camera-settings-foundation_test.cpp`. This repo's actual C++
# Automation Spec tests live in `Moon/Source/Moon/Tests/` and compile into the Moon module (see
# `MoonOverdriveStateTests.cpp`, `MoonCameraSettingsTests.cpp` for this story's AC-3 coverage,
# which needs no World/Actor and IS a real compiled test). AC-1/AC-2/AC-4 below assert facts about
# AMoonCharacterBase's constructor/BeginPlay contract — the sibling convention this repo actually
# uses for that (see `tests/unit/movement/camera_yaw_facing_test.ps1`) is a source-level static
# regression script, not a `.cpp` at this path (nothing under repo-root `tests/` compiles). This
# file follows that established sibling convention; the story's own filename stem is kept for
# evidence traceability, extension changed from `.cpp` to `.ps1`. IMPORTANT LIMITATION: this is
# static source verification only — it proves BeginPlay's code applies CameraSettings' fields and
# guards the null case, it does NOT execute BeginPlay or observe a live SpringArm's runtime value
# (e.g. `CameraBoom->TargetArmLength == 300.0` from AC-4's Given). A live PIE/Automation run remains
# the outstanding gap for full runtime verification of AC-4.

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

function Assert-NotMatches {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Message
    )

    if ($Text -match $Pattern) {
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

function test_camera_hierarchy_pivot_is_z60_not_zero {
    param([string]$Cpp)

    $ctorBody = Get-FunctionBody $Cpp "AMoonCharacterBase::AMoonCharacterBase\s*\("
    Assert-Matches $ctorBody "CameraBoom\s*=\s*CreateDefaultSubobject<USpringArmComponent>" "CameraBoom must be a SpringArm subobject."
    Assert-Matches $ctorBody "CameraBoom->SetupAttachment\s*\(\s*RootComponent\s*\)" "CameraBoom must attach directly to the capsule root."
    # Story 005 AC-4 changed FollowCamera's concrete type from UCameraComponent to its
    # UMoonCameraComponent subclass (near-clip override for corner-dithering) — still a Camera
    # subobject, just no longer the stock engine class.
    Assert-Matches $ctorBody "FollowCamera\s*=\s*CreateDefaultSubobject<(UCameraComponent|UMoonCameraComponent)>" "FollowCamera must be a Camera subobject."
    Assert-Matches $ctorBody "FollowCamera->SetupAttachment\s*\(\s*CameraBoom\s*,\s*USpringArmComponent::SocketName\s*\)" "FollowCamera must attach to CameraBoom's socket."
    Assert-Matches $ctorBody "CameraBoom->SetRelativeLocation\s*\(\s*FVector\s*\(\s*0\.0f\s*,\s*0\.0f\s*,\s*60\.0f\s*\)\s*\)" "AC-1: SpringArm pivot must be set to relative Z=60.0uu."
    Assert-NotMatches $ctorBody "CameraBoom->SetRelativeLocation\s*\(\s*FVector\s*\(\s*0\.0f\s*,\s*0\.0f\s*,\s*0\.0f\s*\)\s*\)" "AC-1 edge case: relative location of exactly 0.0 is the old-pivot regression and must fail."
}

function test_camera_inherit_flags_match_gdd_rule4 {
    param([string]$Cpp)

    $ctorBody = Get-FunctionBody $Cpp "AMoonCharacterBase::AMoonCharacterBase\s*\("
    Assert-Matches $ctorBody "CameraBoom->bUsePawnControlRotation\s*=\s*true\s*;" "AC-2: bUsePawnControlRotation must be true."
    Assert-Matches $ctorBody "CameraBoom->bInheritPitch\s*=\s*true\s*;" "AC-2: bInheritPitch must be true."
    Assert-Matches $ctorBody "CameraBoom->bInheritYaw\s*=\s*true\s*;" "AC-2: bInheritYaw must be true."
    Assert-Matches $ctorBody "CameraBoom->bInheritRoll\s*=\s*false\s*;" "AC-2: bInheritRoll must be explicitly false (horizon-tilt regression guard)."
    Assert-NotMatches $ctorBody "CameraBoom->bInheritRoll\s*=\s*true\s*;" "AC-2 edge case: bInheritRoll=true is the horizon-tilt regression and must fail."
}

function test_camera_settings_dataasset_referenced_on_character {
    param([string]$Header)

    Assert-Matches $Header "TObjectPtr<UMoonCameraSettings>\s+CameraSettings\s*;" "AMoonCharacterBase must hold a UMoonCameraSettings reference property."
}

function test_beginplay_applies_camera_settings_not_constructor_literal {
    param([string]$Cpp)

    $beginPlayBody = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::BeginPlay\s*\("
    Assert-Matches $beginPlayBody "(?m)^\s*ApplyCameraSettings\s*\(\s*\)\s*;" "AC-5: BeginPlay must call ApplyCameraSettings()."

    $applyBody = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::ApplyCameraSettings\s*\("

    # Null/stale-asset guard (AC-4 edge case) must appear before any effective settings field read.
    $guardMatch = [regex]::Match($applyBody, "if\s*\(\s*!IsValid\(EffectiveSettings\)\s*\)")
    if (-not $guardMatch.Success) {
        throw "AC-4 edge case: ApplyCameraSettings must null-guard CameraSettings before use."
    }
    $firstFieldRead = [regex]::Match($applyBody, "EffectiveSettings->TargetArmLength")
    if (-not $firstFieldRead.Success) {
        throw "ApplyCameraSettings must read CameraSettings->TargetArmLength."
    }
    if ($firstFieldRead.Index -lt $guardMatch.Index) {
        throw "AC-4 edge case: CameraSettings fields must not be read before the null guard."
    }

    # AC-4/AC-6: the applied value must be sourced from the asset, not a re-stated constructor literal.
    Assert-Matches $applyBody "CameraBoom->TargetArmLength\s*=\s*EffectiveSettings->TargetArmLength\s*;" "AC-4/AC-6: TargetArmLength must be assigned from the validated settings, not a literal."
    Assert-NotMatches $applyBody "CameraBoom->TargetArmLength\s*=\s*450\.0f\s*;" "AC-6: ApplyCameraSettings must not re-read the constructor's literal 450.0f."
    Assert-Matches $applyBody "CameraBoom->SocketOffset\s*=\s*EffectiveSettings->CameraSocketOffset\s*;" "ApplyCameraSettings must assign SocketOffset from the validated settings."
    Assert-Matches $applyBody "CameraBoom->CameraLagSpeed\s*=\s*EffectiveSettings->CameraLagSpeed\s*;" "ApplyCameraSettings must assign CameraLagSpeed from the validated settings."
    Assert-Matches $applyBody "CameraBoom->CameraLagMaxDistance\s*=\s*EffectiveSettings->CameraLagMaxDistance\s*;" "ApplyCameraSettings must assign CameraLagMaxDistance from the validated settings."
    Assert-Matches $applyBody "FollowCamera->SetFieldOfView\(EffectiveSettings->BaseFOV\)\s*;" "ApplyCameraSettings must assign FieldOfView from the validated settings' BaseFOV."
}

$cpp = Get-Content -LiteralPath $characterCpp -Raw
$header = Get-Content -LiteralPath $characterHeader -Raw
if (-not (Test-Path -LiteralPath $settingsHeader)) {
    throw "UMoonCameraSettings header not found at expected path: $settingsHeader"
}

test_camera_hierarchy_pivot_is_z60_not_zero $cpp
test_camera_inherit_flags_match_gdd_rule4 $cpp
test_camera_settings_dataasset_referenced_on_character $header
test_beginplay_applies_camera_settings_not_constructor_literal $cpp

Write-Host "camera settings foundation static checks passed (AC-1/AC-2/AC-5/AC-6 source-level verified; AC-3 covered by the compiled Moon.Camera.Settings.ShippedDefaults Automation test; AC-4 runtime SpringArm value NOT observed here, static-only)"
