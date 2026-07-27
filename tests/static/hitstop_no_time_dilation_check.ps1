$ErrorActionPreference = "Stop"

# Story 004: Presentation-Only Hitstop Rewrite (TR-mov-008 / ADR-0009 Decision 5).
#
# AC-2 (player-movement.md / GDD line ~273): "GIVEN movement source is searched, WHEN static
# checks run, THEN hitstop paths contain no CustomTimeDilation or global Time Dilation calls."
# The GDD itself notes this absence-only grep was already covered by the pre-rewrite AC and is a
# NEGATIVE verification only (line 265: "기존 AC는 Time Dilation 호출 부재만 그렙으로 확인하는
# 네거티브 검증이었음"). This script therefore also asserts POSITIVE evidence that the replacement
# capture-and-blend mechanism actually exists — not just that the forbidden APIs are gone.

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
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

# --- AC-2 negative check: no Time Dilation anywhere in the hitstop-owning files ---

function test_no_custom_time_dilation_anywhere_in_character_header_or_cpp {
    param(
        [string]$Header,
        [string]$Cpp
    )

    Assert-NotMatches $Header "CustomTimeDilation" "MoonCharacterBase.h must contain no reference to CustomTimeDilation (ADR-0009 Decision 5 / V-1)."
    Assert-NotMatches $Cpp "CustomTimeDilation" "MoonCharacterBase.cpp must contain no reference to CustomTimeDilation (ADR-0009 Decision 5 / V-1)."
    Assert-NotMatches $Header "SetGlobalTimeDilation" "MoonCharacterBase.h must contain no reference to SetGlobalTimeDilation."
    Assert-NotMatches $Cpp "SetGlobalTimeDilation" "MoonCharacterBase.cpp must contain no reference to SetGlobalTimeDilation."
}

# --- Positive checks: the replacement capture-and-blend mechanism actually exists ---

function test_trigger_hit_stop_captures_mesh_transform_instead_of_dilating {
    param([string]$Cpp)

    $body = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::TriggerHitStop\s*\("

    Assert-Matches $body "FreezeStartMeshTransform\s*=\s*MeshComp->GetComponentTransform\s*\(\s*\)\s*;" "TriggerHitStop() must capture the mesh's world transform at freeze start (capture-and-blend, not Time Dilation)."
    Assert-Matches $body "bPauseAnims\s*=\s*true\s*;" "TriggerHitStop() must pause the mesh's anim playback for the freeze duration."
    Assert-NotMatches $body "CustomTimeDilation|SetGlobalTimeDilation" "TriggerHitStop() must not use any form of Time Dilation."
}

function test_end_hit_stop_does_not_restore_time_dilation {
    param([string]$Cpp)

    $body = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::EndHitStop\s*\(\s*\)"

    Assert-Matches $body "bPauseAnims\s*=\s*false\s*;" "EndHitStop() must resume anim playback."
    Assert-Matches $body "BlendingOut" "EndHitStop() must start the blend-out phase rather than instantly restoring the transform (Core Rule 9 forbids an instant snap)."
    Assert-NotMatches $body "CustomTimeDilation|SetGlobalTimeDilation" "EndHitStop() must not restore any form of Time Dilation."
}

function test_tick_drives_the_hitstop_presentation_step_every_frame {
    param([string]$Cpp)

    $tickBody = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::Tick\s*\(\s*float\s+DeltaTime\s*\)"

    Assert-Matches $tickBody "UpdateHitStopPresentation\s*\(\s*DeltaTime\s*\)\s*;" "Tick() must call UpdateHitStopPresentation(DeltaTime) every frame so the mesh-only freeze/blend actually runs while the Capsule/CMC keep ticking normally."
}

function test_update_hit_stop_presentation_forces_mesh_only_never_the_capsule {
    param([string]$Cpp)

    $body = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::UpdateHitStopPresentation\s*\("

    Assert-Matches $body "SetWorldTransform\s*\(" "UpdateHitStopPresentation() must force the mesh's world transform during the freeze/blend window."
    Assert-NotMatches $body "GetCharacterMovement\s*\(\s*\)\s*->\s*Velocity\s*=" "UpdateHitStopPresentation() must never write CharacterMovementComponent::Velocity directly — gameplay position must keep advancing untouched."
    Assert-NotMatches $body "SetActorLocation|SetActorTransform" "UpdateHitStopPresentation() must only move the mesh component, never the actor/Capsule itself."
    Assert-NotMatches $body "CustomTimeDilation|SetGlobalTimeDilation" "UpdateHitStopPresentation() must not use any form of Time Dilation."
}

$header = Get-Content -LiteralPath $characterHeader -Raw
$cpp = Get-Content -LiteralPath $characterCpp -Raw

test_no_custom_time_dilation_anywhere_in_character_header_or_cpp $header $cpp
test_trigger_hit_stop_captures_mesh_transform_instead_of_dilating $cpp
test_end_hit_stop_does_not_restore_time_dilation $cpp
test_tick_drives_the_hitstop_presentation_step_every_frame $cpp
test_update_hit_stop_presentation_forces_mesh_only_never_the_capsule $cpp

Write-Host "hitstop no-time-dilation static checks passed (negative: zero CustomTimeDilation/SetGlobalTimeDilation refs; positive: capture-and-blend mechanism verified)"
