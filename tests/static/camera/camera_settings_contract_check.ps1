$ErrorActionPreference = "Stop"

# Story 001: Camera Settings and Component Hierarchy (TR-cam-001, TR-cam-009).
# Static contract coverage complements the required PIE/CDO inspection documented in the evidence
# file. It proves the C++ hierarchy, DataAsset schema/defaults/ranges, one-shot BeginPlay apply,
# safe fallback diagnostics, and the story's explicit out-of-scope boundary.

$repoRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$settingsHeaderPath = Join-Path $repoRoot "Moon/Source/Moon/Camera/MoonCameraSettings.h"
$settingsCppPath = Join-Path $repoRoot "Moon/Source/Moon/Camera/MoonCameraSettings.cpp"
$cameraComponentHeaderPath = Join-Path $repoRoot "Moon/Source/Moon/Camera/MoonCameraComponent.h"
$cameraComponentCppPath = Join-Path $repoRoot "Moon/Source/Moon/Camera/MoonCameraComponent.cpp"
$characterHeaderPath = Join-Path $repoRoot "Moon/Source/Moon/Character/MoonCharacterBase.h"
$characterCppPath = Join-Path $repoRoot "Moon/Source/Moon/Character/MoonCharacterBase.cpp"

function Assert-Matches {
    param([string]$Text, [string]$Pattern, [string]$Message)
    if ($Text -notmatch $Pattern) { throw $Message }
}

function Assert-NotMatches {
    param([string]$Text, [string]$Pattern, [string]$Message)
    if ($Text -match $Pattern) { throw $Message }
}

function Get-FunctionBody {
    param([string]$Text, [string]$SignaturePattern)
    $match = [regex]::Match($Text, $SignaturePattern)
    if (-not $match.Success) { throw "Could not find function: $SignaturePattern" }
    $start = $Text.IndexOf("{", $match.Index)
    $depth = 0
    for ($index = $start; $index -lt $Text.Length; $index++) {
        if ($Text[$index] -eq "{") { $depth++ }
        elseif ($Text[$index] -eq "}") {
            $depth--
            if ($depth -eq 0) { return $Text.Substring($start, $index - $start + 1) }
        }
    }
    throw "Could not find closing brace for function: $SignaturePattern"
}

foreach ($requiredPath in @($settingsHeaderPath, $settingsCppPath, $cameraComponentHeaderPath, $cameraComponentCppPath, $characterHeaderPath, $characterCppPath)) {
    if (-not (Test-Path -LiteralPath $requiredPath)) { throw "Required file missing: $requiredPath" }
}

$settingsHeader = Get-Content -LiteralPath $settingsHeaderPath -Raw
$settingsCpp = Get-Content -LiteralPath $settingsCppPath -Raw
$cameraComponentHeader = Get-Content -LiteralPath $cameraComponentHeaderPath -Raw
$cameraComponentCpp = Get-Content -LiteralPath $cameraComponentCppPath -Raw
$characterHeader = Get-Content -LiteralPath $characterHeaderPath -Raw
$characterCpp = Get-Content -LiteralPath $characterCppPath -Raw

function test_component_hierarchy_and_torso_pivot_are_declared {
    $constructor = Get-FunctionBody $characterCpp "AMoonCharacterBase::AMoonCharacterBase\s*\(\s*\)"
    $apply = Get-FunctionBody $characterCpp "void\s+AMoonCharacterBase::ApplyCameraSettings\s*\("
    Assert-Matches $constructor 'CameraBoom\s*=\s*CreateDefaultSubobject<USpringArmComponent>' "CameraBoom must be a character-owned USpringArmComponent."
    Assert-Matches $constructor 'CameraBoom->SetupAttachment\(RootComponent\)' "CameraBoom must attach directly to the Capsule Root."
    Assert-Matches $constructor 'CameraBoom->SetRelativeLocation\(FVector\(0\.0f,\s*0\.0f,\s*60\.0f\)\)' "CameraBoom pivot must be at torso Z=60uu."
    Assert-Matches $constructor 'FollowCamera->SetupAttachment\(CameraBoom,\s*USpringArmComponent::SocketName\)' "FollowCamera must attach to the SpringArm socket."
    Assert-Matches $constructor 'CameraBoom->bUsePawnControlRotation\s*=\s*true' "CameraBoom must use pawn control rotation."
    Assert-Matches $constructor 'CameraBoom->bInheritPitch\s*=\s*true' "CameraBoom must inherit pitch."
    Assert-Matches $constructor 'CameraBoom->bInheritYaw\s*=\s*true' "CameraBoom must inherit yaw."
    Assert-Matches $constructor 'CameraBoom->bInheritRoll\s*=\s*false' "CameraBoom must not inherit roll."
    Assert-Matches $apply 'CameraBoom->SetRelativeLocation\(FVector\(0\.0f,\s*0\.0f,\s*60\.0f\)\)' "BeginPlay camera initialization must reassert the torso pivot against stale Blueprint defaults."
    Assert-Matches $apply 'CameraBoom->TargetOffset\s*=\s*FVector::ZeroVector' "Runtime camera initialization must not retain the historical TargetOffset pivot."
    Assert-Matches $apply 'CameraBoom->bDoCollisionTest\s*=\s*true' "Runtime camera initialization must enable SpringArm collision."
    Assert-Matches $apply 'CameraBoom->ProbeChannel\s*=\s*ECC_Camera' "Runtime camera initialization must use ECC_Camera."
}

function test_data_asset_exposes_exact_story_fields_defaults_and_ranges {
    Assert-Matches $settingsHeader 'class\s+MOON_API\s+UMoonCameraSettings\s*:\s*public\s+UDataAsset' "UMoonCameraSettings must derive from UDataAsset."

    $contracts = @(
        @{ Name = "TargetArmLength"; Default = "450\.0f"; Min = "250\.0"; Max = "500\.0" },
        @{ Name = "CameraPitchMin"; Default = "-60\.0f"; Min = "-80\.0"; Max = "-45\.0" },
        @{ Name = "CameraPitchMax"; Default = "30\.0f"; Min = "15\.0"; Max = "45\.0" },
        @{ Name = "CameraLagSpeed"; Default = "18\.0f"; Min = "5\.0"; Max = "20\.0" },
        @{ Name = "CameraRotationLagSpeed"; Default = "15\.0f"; Min = "8\.0"; Max = "25\.0" },
        @{ Name = "CameraLagMaxDistance"; Default = "60\.0f"; Min = "40\.0"; Max = "120\.0" },
        @{ Name = "BaseFOV"; Default = "90\.0f"; Min = "80\.0"; Max = "100\.0" },
        @{ Name = "OverdriveFOV"; Default = "100\.0f"; Min = "95\.0"; Max = "110\.0" },
        @{ Name = "ExecutionArmLength"; Default = "150\.0f"; Min = "100\.0"; Max = "250\.0" },
        @{ Name = "CameraProbeSize"; Default = "12\.0f"; Min = "5\.0"; Max = "25\.0" }
    )

    foreach ($contract in $contracts) {
        $propertyPattern = "(?s)UPROPERTY\([^\)]*ClampMin\s*=\s*`"$($contract.Min)`"[^\)]*ClampMax\s*=\s*`"$($contract.Max)`"[^\)]*\)+\s*float\s+$($contract.Name)\s*=\s*$($contract.Default)\s*;"
        Assert-Matches $settingsHeader $propertyPattern "$($contract.Name) must expose the approved default and ClampMin/ClampMax metadata."
    }

    Assert-Matches $settingsHeader 'FVector\s+CameraSocketOffset\s*=\s*FVector\(0\.0f,\s*45\.0f,\s*20\.0f\)' "CameraSocketOffset default must be (0,45,20)."
    Assert-Matches $settingsCpp 'CameraSocketOffset\.X[\s\S]*IsWithinInclusive\(CameraSocketOffset\.Y,\s*-50\.0,\s*50\.0\)[\s\S]*IsWithinInclusive\(CameraSocketOffset\.Z,\s*-30\.0,\s*50\.0\)' "CameraSocketOffset X/Y/Z contract must be runtime-validated."

    $declaredFields = [regex]::Matches($settingsHeader, '(?m)^\s*(?:float|FVector)\s+(TargetArmLength|CameraSocketOffset|CameraPitchMin|CameraPitchMax|CameraLagSpeed|CameraRotationLagSpeed|CameraLagMaxDistance|BaseFOV|OverdriveFOV|ExecutionArmLength|CameraProbeSize)\s*=')
    if ($declaredFields.Count -ne 11) { throw "UMoonCameraSettings must declare exactly the 11 Story 001 tuning fields; found $($declaredFields.Count)." }
}

function test_begin_play_applies_valid_settings_once {
	$constructor = Get-FunctionBody $characterCpp "AMoonCharacterBase::AMoonCharacterBase\s*\(\s*\)"
    $beginPlay = Get-FunctionBody $characterCpp "void\s+AMoonCharacterBase::BeginPlay\s*\("
    $tick = Get-FunctionBody $characterCpp "void\s+AMoonCharacterBase::Tick\s*\("
    $apply = Get-FunctionBody $characterCpp "void\s+AMoonCharacterBase::ApplyCameraSettings\s*\("

    Assert-Matches $characterHeader 'TObjectPtr<UMoonCameraSettings>\s+CameraSettings\s*;' "Character must expose an asset reference to UMoonCameraSettings."
	Assert-Matches $constructor '/Game/Moon/Camera/DA_MoonCameraSettings\.DA_MoonCameraSettings' "The native character must assign the shared production camera DataAsset by default."
	Assert-Matches $constructor 'CameraSettings\s*=\s*DefaultCameraSettings\.Object' "The loaded production camera DataAsset must become the character default reference."
    Assert-Matches $beginPlay 'ApplyCameraSettings\s*\(\s*\)\s*;' "BeginPlay must apply the camera DataAsset."
    Assert-NotMatches $tick 'ApplyCameraSettings\s*\(' "Camera settings must not be reapplied every Tick."

    foreach ($assignment in @(
        'TargetArmLength\s*=\s*EffectiveSettings->TargetArmLength',
        'SocketOffset\s*=\s*EffectiveSettings->CameraSocketOffset',
        'CameraLagSpeed\s*=\s*EffectiveSettings->CameraLagSpeed',
        'CameraRotationLagSpeed\s*=\s*EffectiveSettings->CameraRotationLagSpeed',
        'CameraLagMaxDistance\s*=\s*EffectiveSettings->CameraLagMaxDistance',
        'ProbeSize\s*=\s*EffectiveSettings->CameraProbeSize',
        'SetFieldOfView\(EffectiveSettings->BaseFOV\)'
    )) {
        Assert-Matches $apply $assignment "Runtime apply is missing required assignment: $assignment"
    }
}

function test_missing_or_invalid_asset_logs_and_uses_safe_defaults {
    $apply = Get-FunctionBody $characterCpp "void\s+AMoonCharacterBase::ApplyCameraSettings\s*\("
    Assert-Matches $apply '!IsValid\(EffectiveSettings\)' "Missing/stale CameraSettings assets must be detected."
    Assert-Matches $apply 'IsWithinSafeRanges\(FailureReason\)' "Out-of-range CameraSettings assets must be rejected."
    Assert-Matches $apply 'UE_LOG\(LogTemp,\s*Error[\s\S]*DA_MoonCameraSettings' "Missing asset diagnostics must be actionable."
    Assert-Matches $apply 'UE_LOG\(LogTemp,\s*Error[\s\S]*rejected CameraSettings asset' "Invalid asset diagnostics must identify the rejected asset."
    if ([regex]::Matches($apply, 'Get(?:Mutable)?Default<UMoonCameraSettings>\(\)').Count -lt 2) {
        throw "Both missing and invalid asset paths must use UMoonCameraSettings safe defaults."
    }
}

function test_story_does_not_add_out_of_scope_camera_behaviour {
    Assert-NotMatches $characterCpp 'SetOverdriveFOVActive|BeginExecutionCameraBlend|EndExecutionCameraBlend' "Story 001 must not add dynamic FOV or execution blend behavior."
    Assert-NotMatches $characterCpp 'ViewPitchMin|ViewPitchMax' "Character code must not own the PlayerCameraManager pitch clamp."
}

function test_per_frame_camera_reads_validated_settings {
    $tickUpdate = Get-FunctionBody $characterCpp "void\s+AMoonCharacterBase::UpdateCameraCornerDither\s*\("
    $apply = Get-FunctionBody $characterCpp "void\s+AMoonCharacterBase::ApplyCameraSettings\s*\("
    Assert-Matches $characterHeader 'TObjectPtr<UMoonCameraSettings>\s+AppliedCameraSettings\s*;' "Character must retain the validated effective settings."
    Assert-Matches $apply 'AppliedCameraSettings\s*=\s*EffectiveSettings\s*;' "ApplyCameraSettings must retain the validated asset or safe fallback."
    Assert-Matches $tickUpdate 'AppliedCameraSettings' "Per-frame camera presentation must use the validated settings."
    Assert-NotMatches $tickUpdate 'CameraSettings\s*\?' "Per-frame camera presentation must not read a rejected source asset."
}

function test_corner_dither_runs_after_spring_arm_collision_update {
    $actorTick = Get-FunctionBody $characterCpp "void\s+AMoonCharacterBase::Tick\s*\("
    $componentConstructor = Get-FunctionBody $cameraComponentCpp "UMoonCameraComponent::UMoonCameraComponent\s*\("
    $componentTick = Get-FunctionBody $cameraComponentCpp "void\s+UMoonCameraComponent::TickComponent\s*\("
    Assert-NotMatches $actorTick 'UpdateCameraCornerDither\s*\(' "Character's pre-physics Tick must not read SpringArm's previous-frame socket transform."
    Assert-Matches $componentConstructor 'PrimaryComponentTick\.TickGroup\s*=\s*TG_PostPhysics' "Corner dither must run in the SpringArm's post-physics tick group."
    Assert-Matches $characterCpp 'FollowCamera->AddTickPrerequisiteComponent\(CameraBoom\)' "Corner dither must run after CameraBoom updates its collision-resolved socket."
    Assert-Matches $componentTick 'UpdateCameraCornerDither\s*\(DeltaTime\)' "The post-physics camera component tick must drive corner dither."
}

test_component_hierarchy_and_torso_pivot_are_declared
test_data_asset_exposes_exact_story_fields_defaults_and_ranges
test_begin_play_applies_valid_settings_once
test_missing_or_invalid_asset_logs_and_uses_safe_defaults
test_per_frame_camera_reads_validated_settings
test_corner_dither_runs_after_spring_arm_collision_update
test_story_does_not_add_out_of_scope_camera_behaviour

Write-Host "camera settings and component hierarchy contract checks passed (6 test functions)"
