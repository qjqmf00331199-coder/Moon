$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$characterCpp = Join-Path $repoRoot "Moon/Source/Moon/Character/MoonCharacterBase.cpp"
$buildCs = Join-Path $repoRoot "Moon/Source/Moon/Moon.Build.cs"

function Assert-Contains {
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

$cpp = Get-Content -LiteralPath $characterCpp -Raw
$build = Get-Content -LiteralPath $buildCs -Raw

Assert-Contains $cpp "bUseControllerRotationYaw\s*=\s*true\s*;" "AMoonCharacterBase must face controller yaw."
Assert-Contains $cpp "bUseControllerRotationPitch\s*=\s*false\s*;" "AMoonCharacterBase must not inherit controller pitch."
Assert-Contains $cpp "bUseControllerRotationRoll\s*=\s*false\s*;" "AMoonCharacterBase must not inherit controller roll."
Assert-Contains $cpp "bOrientRotationToMovement\s*=\s*false\s*;" "CharacterMovement must not orient rotation to movement."

if ($build -match "SpellCasting") {
    throw "Moon.Build.cs must not depend on a SpellCasting module for movement."
}

$moveBody = Get-FunctionBody $cpp "void\s+AMoonCharacterBase::Move\s*\("
if ($moveBody -match "Spell|CastAbility|TryActivateAbilityByTag") {
    throw "AMoonCharacterBase::Move must not reference spell state or spell activation."
}

Write-Host "movement foundation contract static checks passed"
