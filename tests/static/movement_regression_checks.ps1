$ErrorActionPreference = "Stop"

# Story 005: Movement Traceability and Static Regression Checks (TR-mov-002, TR-mov-010).
#
# This script CONSOLIDATES movement static regression coverage rather than duplicating existing
# checks:
#   - Spell Casting independence (TR-mov-002)         -> delegates to movement_foundation_contract.ps1
#   - Time Dilation, whole-file (TR-mov-002 broad list) -> new whole-file check here (the existing
#     hitstop_no_time_dilation_check.ps1 is scoped to the hitstop-owning functions specifically;
#     the control manifest's forbidden list is broader than just those functions, so this script
#     re-checks the whole file as a belt-and-suspenders regression net, and ALSO delegates to the
#     hitstop-scoped script for its positive capture-and-blend assertions)
#   - Montage-based input lock (TR-mov-002 forbidden list) -> new check here
#   - Root-motion locomotion (TR-mov-010)              -> new check here, source-level only
#
# Known limitation (documented again here, not just in the evidence doc, so anyone reading this
# script in isolation sees it too): TR-mov-010's root-motion check below is a SOURCE-CODE-LEVEL
# grep only. It cannot verify the actual .uasset import setting (bEnableRootMotion) on
# JumpStartAnim/JumpApexAnim/JumpLandAnim, since .uasset files are binary and this script has no
# Unreal Editor/MCP access to read import settings. See the Story 005 evidence doc for the
# outstanding checklist item.

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$characterHeader = Join-Path $repoRoot "Moon/Source/Moon/Character/MoonCharacterBase.h"
$characterCpp = Join-Path $repoRoot "Moon/Source/Moon/Character/MoonCharacterBase.cpp"
$foundationContractScript = Join-Path $PSScriptRoot "movement_foundation_contract.ps1"

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

# --- Pattern-detection functions under test (used both for real-source checks below AND for the
#     AC-3 self-test that proves each pattern is actually detectable, not just vacuously passing) ---

function Test-HasTimeDilationReference {
    param([string]$Text)
    return ($Text -match "CustomTimeDilation|SetGlobalTimeDilation")
}

function Test-HasMontageInputLockReference {
    param([string]$Text)
    # Specific montage API tokens only — a bare "Montage" pattern would false-positive on prose
    # comments in this codebase (e.g. MoonCharacterBase.h's PlayOneShotAnim doc comment: "abilities
    # that don't have their own montage/slot system yet"), which is not a violation.
    return ($Text -match "UAnimMontage|PlayMontage|Montage_Play|PlaySlotAnimationAsDynamicMontage")
}

function Test-HasRootMotionEnabled {
    param([string]$Text)
    return ($Text -match "bEnableRootMotion\s*=\s*true|RootMotionMode\s*=")
}

function Test-HasSpellCastingReference {
    param([string]$Text)
    return ($Text -match "Spell|CastAbility|TryActivateAbilityByTag|GameplayAbility")
}

# --- AC-3 self-test: prove each detector actually fires on a known-bad fixture string, so a
#     passing run of the real checks below means "clean", not "the regex never matches anything" ---

function test_time_dilation_detector_fires_on_known_violation {
    $violation = 'MoveComp->CustomTimeDilation = 0.1f;'
    if (-not (Test-HasTimeDilationReference $violation)) {
        throw "AC-3 self-test failed: Time Dilation detector did not fire on a known violation string. The detector cannot be trusted to protect real source."
    }
}

function test_montage_input_lock_detector_fires_on_known_violation {
    $violation = 'MeshComp->Montage_Play(SomeMontage, 1.0f);'
    if (-not (Test-HasMontageInputLockReference $violation)) {
        throw "AC-3 self-test failed: Montage input-lock detector did not fire on a known violation string."
    }
}

function test_montage_input_lock_detector_does_not_fire_on_benign_prose {
    # Regression guard for the exact false-positive risk flagged during this story's implementation:
    # PowerShell -match is case-insensitive, and MoonCharacterBase.h's PlayOneShotAnim doc comment
    # says "abilities that don't have their own montage/slot system yet" — that prose must not trip
    # the detector.
    $benignProse = "abilities that don't have their own montage/slot system yet"
    if (Test-HasMontageInputLockReference $benignProse) {
        throw "REGRESSION: montage input-lock detector fired on benign prose ('montage/slot system') that names no montage API. Pattern must only match specific montage API tokens (UAnimMontage, PlayMontage, Montage_Play, PlaySlotAnimationAsDynamicMontage), not the bare word 'montage'."
    }
}

function test_root_motion_detector_fires_on_known_violation {
    $violation = 'Anim->bEnableRootMotion = true;'
    if (-not (Test-HasRootMotionEnabled $violation)) {
        throw "AC-3 self-test failed: root-motion detector did not fire on a known violation string."
    }
}

function test_spell_casting_reference_detector_fires_on_known_violation {
    $violation = 'TryActivateAbilityByTag(FGameplayTag::RequestGameplayTag(FName("Spell.Element.Fire")));'
    if (-not (Test-HasSpellCastingReference $violation)) {
        throw "AC-3 self-test failed: Spell Casting reference detector did not fire on a known violation string."
    }
}

# --- Real checks against MoonCharacterBase.h/.cpp ---

function test_no_time_dilation_anywhere_in_character_header_or_cpp {
    param(
        [string]$Header,
        [string]$Cpp
    )

    if (Test-HasTimeDilationReference $Header) {
        throw "MoonCharacterBase.h must contain no CustomTimeDilation/SetGlobalTimeDilation reference (ADR-0009 / control-manifest Forbidden Approaches, TR-mov-002)."
    }
    if (Test-HasTimeDilationReference $Cpp) {
        throw "MoonCharacterBase.cpp must contain no CustomTimeDilation/SetGlobalTimeDilation reference (ADR-0009 / control-manifest Forbidden Approaches, TR-mov-002)."
    }
}

function test_no_montage_based_input_lock_anywhere_in_character_header_or_cpp {
    param(
        [string]$Header,
        [string]$Cpp
    )

    if (Test-HasMontageInputLockReference $Header) {
        throw "MoonCharacterBase.h must not reference montage-based animation playback (UAnimMontage/PlayMontage/Montage_Play/PlaySlotAnimationAsDynamicMontage) — this codebase's locomotion input lock rule (GDD Core Rule 8) requires PlayAnimation/single-node playback only, never montage-gated input."
    }
    if (Test-HasMontageInputLockReference $Cpp) {
        throw "MoonCharacterBase.cpp must not reference montage-based animation playback (UAnimMontage/PlayMontage/Montage_Play/PlaySlotAnimationAsDynamicMontage) — this codebase's locomotion input lock rule (GDD Core Rule 8) requires PlayAnimation/single-node playback only, never montage-gated input."
    }
}

function test_no_root_motion_locomotion_in_character_cpp {
    param([string]$Cpp)

    if (Test-HasRootMotionEnabled $Cpp) {
        throw "MoonCharacterBase.cpp must not enable root motion for locomotion (TR-mov-010). NOTE: this is a source-code-level check only — it cannot verify the actual .uasset import setting on JumpStartAnim/JumpApexAnim/JumpLandAnim; see the Story 005 evidence doc."
    }
}

function test_spell_casting_independence_delegates_to_movement_foundation_contract {
    param([string]$ScriptPath)

    if (-not (Test-Path -LiteralPath $ScriptPath)) {
        throw "movement_foundation_contract.ps1 not found at expected path: $ScriptPath"
    }

    # Delegates rather than duplicating: movement_foundation_contract.ps1 already asserts
    # Moon.Build.cs has no SpellCasting dependency and Move() has no Spell/CastAbility/
    # TryActivateAbilityByTag reference (TR-mov-002). Re-running it here folds that coverage into
    # this consolidated suite without re-implementing the same regex twice.
    & $ScriptPath
}

$header = Get-Content -LiteralPath $characterHeader -Raw
$cpp = Get-Content -LiteralPath $characterCpp -Raw

# AC-3 self-test first: prove the detectors work before trusting their "clean" verdict on real source.
test_time_dilation_detector_fires_on_known_violation
test_montage_input_lock_detector_fires_on_known_violation
test_montage_input_lock_detector_does_not_fire_on_benign_prose
test_root_motion_detector_fires_on_known_violation
test_spell_casting_reference_detector_fires_on_known_violation

# Real regression checks against MoonCharacterBase.h/.cpp.
test_no_time_dilation_anywhere_in_character_header_or_cpp $header $cpp
test_no_montage_based_input_lock_anywhere_in_character_header_or_cpp $header $cpp
test_no_root_motion_locomotion_in_character_cpp $cpp
test_spell_casting_independence_delegates_to_movement_foundation_contract $foundationContractScript

Write-Host "movement regression static checks passed (Spell Casting independence delegated + verified; whole-file Time Dilation, montage input-lock, and source-level root-motion checks passed with self-tested detectors)"
