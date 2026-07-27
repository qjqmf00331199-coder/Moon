$ErrorActionPreference = "Stop"

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

function test_camera_yaw_flags_snap_to_controller_yaw {
    param([string]$Cpp)

    Assert-Matches $Cpp "bUseControllerRotationYaw\s*=\s*true\s*;" "AMoonCharacterBase must face controller yaw."
    Assert-Matches $Cpp "bUseControllerRotationPitch\s*=\s*false\s*;" "AMoonCharacterBase must not inherit controller pitch."
    Assert-Matches $Cpp "bUseControllerRotationRoll\s*=\s*false\s*;" "AMoonCharacterBase must not inherit controller roll."
    Assert-Matches $Cpp "bOrientRotationToMovement\s*=\s*false\s*;" "CharacterMovement must not orient rotation to movement."
}

function test_move_uses_yaw_only_camera_relative_axes {
    param([string]$Cpp)

    $moveBody = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::Move\s*\("
    Assert-Matches $moveBody "GetControlRotation\s*\(" "Move must derive movement basis from controller rotation."
    Assert-Matches $moveBody "FRotator\s+YawRotation\s*\(\s*0\s*,\s*Rotation\.Yaw\s*,\s*0\s*\)" "Move must discard pitch and roll when deriving movement axes."
    Assert-Matches $moveBody "GetUnitAxis\s*\(\s*EAxis::X\s*\)" "Move must derive a forward axis from yaw."
    Assert-Matches $moveBody "GetUnitAxis\s*\(\s*EAxis::Y\s*\)" "Move must derive a right axis from yaw."
    Assert-Matches $moveBody "AddMovementInput\s*\(\s*ForwardDirection\s*,\s*MovementVector\.Y\s*\)" "Forward input must use the yaw-only forward axis."
    Assert-Matches $moveBody "AddMovementInput\s*\(\s*RightDirection\s*,\s*MovementVector\.X\s*\)" "Right input must use the yaw-only right axis."
}

function test_move_path_has_no_spell_casting_references {
    param([string]$Cpp)

    $moveBody = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::Move\s*\("
    Assert-NotMatches $moveBody "Spell|CastAbility|TryActivateAbilityByTag|GameplayAbility" "Move must not reference spell state or spell activation."
}

function test_locomotion_code_does_not_enable_root_motion {
    param([string]$Cpp)

    Assert-NotMatches $Cpp "bEnableRootMotion\s*=\s*true|RootMotionMode\s*=" "Movement story code must not enable root motion for locomotion."
}

$cpp = Get-Content -LiteralPath $characterCpp -Raw
test_camera_yaw_flags_snap_to_controller_yaw $cpp
test_move_uses_yaw_only_camera_relative_axes $cpp
test_move_path_has_no_spell_casting_references $cpp
test_locomotion_code_does_not_enable_root_motion $cpp

Write-Host "camera yaw facing unit checks passed"
