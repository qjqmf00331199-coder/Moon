$ErrorActionPreference = "Stop"

# Story 005: Movement Traceability and Static Regression Checks (TR-mov-009, AC-1/AC-2).
#
# IMPORTANT — what this script IS and IS NOT:
# This is a SOURCE-LEVEL static verification that the two MovementInputTrace Insights scopes exist
# with the exact names ADR-0009 Decision 6 specifies, and are positioned at the ADR-mandated points
# (Move() callback start, before the MovementLocked gate; Tick() immediately after Super::Tick()).
# It does NOT run a live Unreal Insights capture — that requires a running Editor/PIE session,
# which is not available in this automation environment. A human/future session with Editor access
# must still perform the live capture described in the Story 005 evidence doc before AC-1/AC-2 are
# considered fully closed against real Insights trace data, not just source instrumentation.

$repoRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
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

# --- AC-1: MovementInputTrace.InputTriggered fires unconditionally at the top of Move() ---

function test_move_contains_input_triggered_trace_scope_with_exact_name {
    param([string]$Cpp)

    $moveBody = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::Move\s*\("
    Assert-Matches $moveBody "TRACE_CPUPROFILER_EVENT_SCOPE\s*\(\s*MovementInputTrace\.InputTriggered\s*\)\s*;" "Move() must contain TRACE_CPUPROFILER_EVENT_SCOPE(MovementInputTrace.InputTriggered); with this exact name (ADR-0009 Decision 6)."
}

function test_input_triggered_trace_scope_precedes_the_movement_locked_gate {
    param([string]$Cpp)

    $moveBody = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::Move\s*\("

    $traceMatch = [regex]::Match($moveBody, "TRACE_CPUPROFILER_EVENT_SCOPE\s*\(\s*MovementInputTrace\.InputTriggered\s*\)\s*;")
    $lockCheckMatch = [regex]::Match($moveBody, "if\s*\(\s*bMovementLocked\s*\)")

    if (-not $traceMatch.Success) {
        throw "Could not locate the MovementInputTrace.InputTriggered scope in Move() to verify ordering."
    }
    if (-not $lockCheckMatch.Success) {
        throw "Could not locate the bMovementLocked gate in Move() to verify ordering."
    }

    if ($traceMatch.Index -ge $lockCheckMatch.Index) {
        throw "MovementInputTrace.InputTriggered must fire BEFORE the bMovementLocked gate — the trace measures Enhanced Input's Triggered delegate dispatch itself and must capture the callback firing regardless of whether the input is subsequently gated (ADR-0009 Decision 6 / Story 005)."
    }
}

# --- AC-2: MovementInputTrace.VelocityUpdated fires in Tick(), after Super::Tick() ---

function test_tick_contains_velocity_updated_trace_scope_with_exact_name {
    param([string]$Cpp)

    $tickBody = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::Tick\s*\(\s*float\s+DeltaTime\s*\)"
    Assert-Matches $tickBody "TRACE_CPUPROFILER_EVENT_SCOPE\s*\(\s*MovementInputTrace\.VelocityUpdated\s*\)\s*;" "Tick() must contain TRACE_CPUPROFILER_EVENT_SCOPE(MovementInputTrace.VelocityUpdated); with this exact name (ADR-0009 Decision 6)."
}

function test_velocity_updated_trace_scope_follows_super_tick {
    param([string]$Cpp)

    $tickBody = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::Tick\s*\(\s*float\s+DeltaTime\s*\)"

    $superTickMatch = [regex]::Match($tickBody, "Super::Tick\s*\(\s*DeltaTime\s*\)\s*;")
    $traceMatch = [regex]::Match($tickBody, "TRACE_CPUPROFILER_EVENT_SCOPE\s*\(\s*MovementInputTrace\.VelocityUpdated\s*\)\s*;")

    if (-not $superTickMatch.Success) {
        throw "Tick() must call Super::Tick(DeltaTime) — could not locate it to verify ordering."
    }
    if (-not $traceMatch.Success) {
        throw "Could not locate the MovementInputTrace.VelocityUpdated scope in Tick() to verify ordering."
    }

    if ($traceMatch.Index -le $superTickMatch.Index) {
        throw "MovementInputTrace.VelocityUpdated must be placed AFTER Super::Tick(DeltaTime) — this is the first point this frame where CharacterMovementComponent has updated Velocity toward its new target (ADR-0009 Decision 6 / Story 005, same stale-value ordering risk as the TR-mov-003 airborne substate derivation)."
    }
}

# --- Both scopes use the bare-token TRACE_CPUPROFILER_EVENT_SCOPE macro, not the _STR string variant ---

function test_neither_trace_scope_uses_the_string_literal_macro_variant {
    param([string]$Cpp)

    # Verified against this project's actual UE5.8 install
    # (Engine/Source/Runtime/Core/Public/ProfilingDebugging/CpuProfilerTrace.h): the bare
    # TRACE_CPUPROFILER_EVENT_SCOPE(Name) macro stringifies its raw token argument internally
    # (#Name), so a dotted bare token like MovementInputTrace.InputTriggered is valid and produces
    # the exact intended name — using a quoted string literal with this macro would instead emit
    # literal escaped quotes around the name (the header's own documented warning), and the
    # string-literal variant (TRACE_CPUPROFILER_EVENT_SCOPE_STR) is unnecessary here. Epic's own
    # engine source uses this same dotted-bare-token pattern
    # (Editor/UnrealEd/.../DumpMaterialShaderTypes.cpp: TRACE_CPUPROFILER_EVENT_SCOPE(UDumpMaterialShaderTypesCommandlet.AssetRegistryScan)).
    if ($Cpp -match "TRACE_CPUPROFILER_EVENT_SCOPE_STR\s*\(\s*[`"']MovementInputTrace") {
        throw "MovementInputTrace scopes should use the bare-token TRACE_CPUPROFILER_EVENT_SCOPE(Name) macro, not TRACE_CPUPROFILER_EVENT_SCOPE_STR(`"Name`") — the bare macro's internal stringification already produces the exact dotted name correctly."
    }
}

$cpp = Get-Content -LiteralPath $characterCpp -Raw

test_move_contains_input_triggered_trace_scope_with_exact_name $cpp
test_input_triggered_trace_scope_precedes_the_movement_locked_gate $cpp
test_tick_contains_velocity_updated_trace_scope_with_exact_name $cpp
test_velocity_updated_trace_scope_follows_super_tick $cpp
test_neither_trace_scope_uses_the_string_literal_macro_variant $cpp

Write-Host "movement traceability source-level static checks passed (AC-1/AC-2: both MovementInputTrace scopes exist with exact names, at ADR-0009-mandated positions — NOT a live Insights capture, see evidence doc)"
