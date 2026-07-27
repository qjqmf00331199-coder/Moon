$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$characterHeader = Join-Path $repoRoot "Moon/Source/Moon/Character/MoonCharacterBase.h"
$characterCpp = Join-Path $repoRoot "Moon/Source/Moon/Character/MoonCharacterBase.cpp"
$sourceRoot = Join-Path $repoRoot "Moon/Source"

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

function Get-PrivateSectionText {
    param([string]$Text)

    $idx = $Text.LastIndexOf("`nprivate:")
    if ($idx -lt 0) {
        throw "Could not find a private: access-specifier section in AMoonCharacterBase."
    }
    return $Text.Substring($idx)
}

# --- AC-4: MovementLocked ownership (TR-mov-006) ---

function test_movement_locked_field_and_setter_are_declared_private {
    param([string]$Header)

    $privateSection = Get-PrivateSectionText $Header
    Assert-Matches $privateSection "bool\s+bMovementLocked\s*=\s*false\s*;" "bMovementLocked must be declared as a private bool defaulting to false."
    Assert-Matches $privateSection "void\s+SetMovementLocked\s*\(\s*bool\s+bLocked\s*\)\s*;" "SetMovementLocked(bool) must be declared in the private section."
}

function test_set_movement_locked_is_not_blueprint_exposed {
    param([string]$Header)

    # The declaration line itself (and the few lines immediately above it, where a UFUNCTION
    # macro would sit) must not carry BlueprintCallable/BlueprintPure — SetMovementLocked is a
    # plain private C++ method, not a Blueprint-exposed one.
    $declMatch = [regex]::Match($Header, "void\s+SetMovementLocked\s*\(\s*bool\s+bLocked\s*\)\s*;")
    if (-not $declMatch.Success) {
        throw "Could not locate the SetMovementLocked(bool) declaration."
    }
    $precedingWindowStart = [Math]::Max(0, $declMatch.Index - 200)
    $precedingWindow = $Header.Substring($precedingWindowStart, $declMatch.Index - $precedingWindowStart)
    if ($precedingWindow -match "UFUNCTION\s*\(") {
        throw "SetMovementLocked must not be a UFUNCTION (not BlueprintCallable, not BlueprintPure)."
    }
}

function test_is_movement_locked_is_a_public_readonly_query {
    param([string]$Header)

    $queryIdx = $Header.IndexOf("bool IsMovementLocked() const")
    if ($queryIdx -lt 0) {
        throw "IsMovementLocked() const read-only query was not found."
    }

    $privateIdx = $Header.LastIndexOf("`nprivate:")
    if ($privateIdx -ge 0 -and $queryIdx -ge $privateIdx) {
        throw "IsMovementLocked() must be declared in a public section, not private."
    }

    # Confirm it is exposed for read-only Blueprint queries (BlueprintPure), consistent with the
    # other read-only state queries on this class (IsOverdriveActive, IsTensionGainLocked, etc).
    $precedingWindowStart = [Math]::Max(0, $queryIdx - 200)
    $precedingWindow = $Header.Substring($precedingWindowStart, $queryIdx - $precedingWindowStart)
    Assert-Matches $precedingWindow "UFUNCTION\s*\(\s*BlueprintPure" "IsMovementLocked() should be exposed as a BlueprintPure read-only query."
}

function test_move_gates_on_movement_locked_before_add_movement_input {
    param([string]$Cpp)

    $moveBody = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::Move\s*\("

    $lockCheckMatch = [regex]::Match($moveBody, "if\s*\(\s*bMovementLocked\s*\)\s*\{\s*return\s*;\s*\}")
    if (-not $lockCheckMatch.Success) {
        throw "Move() must check 'if (bMovementLocked) { return; }' before doing anything else."
    }

    $addMovementInputMatch = [regex]::Match($moveBody, "AddMovementInput\s*\(")
    if (-not $addMovementInputMatch.Success) {
        throw "Move() no longer calls AddMovementInput() — cannot verify gating order."
    }

    if ($lockCheckMatch.Index -ge $addMovementInputMatch.Index) {
        throw "The bMovementLocked check must occur before AddMovementInput() is called."
    }
}

function test_set_movement_locked_has_no_call_sites_anywhere_in_moon_source {
    param(
        [string]$SourceRoot
    )

    # The only two legitimate appearances of "SetMovementLocked(" in the entire module are:
    #   1. its declaration in MoonCharacterBase.h  ("void SetMovementLocked(bool bLocked);")
    #   2. its definition in MoonCharacterBase.cpp ("void AMoonCharacterBase::SetMovementLocked(bool bLocked)")
    # Anything else — a real call site, or a call site hidden behind a differently-formatted
    # signature — must fail this check. Prose mentions in comments (no argument, i.e. no
    # non-whitespace between the parens) are tolerated since they are not call sites.
    $declarationOrDefinitionPattern = "^\s*(//.*)?\s*(void\s+)?(AMoonCharacterBase::)?SetMovementLocked\s*\(\s*(bool\s+bLocked\s*)?\)\s*;?\s*(\{)?\s*$"

    $files = Get-ChildItem -Path $SourceRoot -Recurse -Include *.h, *.cpp -File
    $callSites = @()

    foreach ($file in $files) {
        $lines = Get-Content -LiteralPath $file.FullName
        for ($i = 0; $i -lt $lines.Count; $i++) {
            $line = $lines[$i]
            if ($line -notmatch "SetMovementLocked") {
                continue
            }

            $trimmed = $line.Trim()

            # Tolerate prose comments that merely name the function (e.g. "see SetMovementLocked()
            # below") — these carry no argument between the parentheses at all.
            if ($trimmed -match "^//.*SetMovementLocked\s*\(\s*\)") {
                continue
            }

            # Tolerate the exact declaration and definition lines.
            if ($trimmed -match "^(void\s+)?(AMoonCharacterBase::)?SetMovementLocked\s*\(\s*bool\s+bLocked\s*\)\s*;?\s*(\{)?\s*$") {
                continue
            }

            $callSites += "$($file.FullName):$($i + 1): $trimmed"
        }
    }

    if ($callSites.Count -gt 0) {
        throw "SetMovementLocked must remain private and uncalled until a Status Effect ADR grants a caller. Unexpected reference(s) found:`n$($callSites -join "`n")"
    }
}

$header = Get-Content -LiteralPath $characterHeader -Raw
$cpp = Get-Content -LiteralPath $characterCpp -Raw

test_movement_locked_field_and_setter_are_declared_private $header
test_set_movement_locked_is_not_blueprint_exposed $header
test_is_movement_locked_is_a_public_readonly_query $header
test_move_gates_on_movement_locked_before_add_movement_input $cpp
test_set_movement_locked_has_no_call_sites_anywhere_in_moon_source $sourceRoot

Write-Host "movement lock contract unit checks passed"
