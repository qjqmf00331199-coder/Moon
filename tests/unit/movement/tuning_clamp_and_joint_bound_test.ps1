$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$characterHeader = Join-Path $repoRoot "Moon/Source/Moon/Character/MoonCharacterBase.h"
$characterCpp = Join-Path $repoRoot "Moon/Source/Moon/Character/MoonCharacterBase.cpp"
$dashCpp = Join-Path $repoRoot "Moon/Source/Moon/GAS/MoonGameplayAbility_Dash.cpp"

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

function Get-BracedBlockAt {
    param(
        [string]$Text,
        [int]$AnchorIndex
    )

    $start = $Text.IndexOf("{", $AnchorIndex)
    if ($start -lt 0) {
        throw "No opening brace found after anchor index $AnchorIndex"
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

    throw "No matching closing brace found after index $start"
}

# Strips // line comments and /* */ block comments so content-based regex assertions (AC-2 formula
# structure, AC-3 no-hardcoded-literal grep) can't be tripped up by prose in doc comments that happens
# to mention "GravityScale=0.1" or "JumpZVelocity=100" etc. Declaration-existence checks are run
# against the RAW (unstripped) text instead, since the declarations themselves are code, not comments.
function Remove-CppComments {
    param([string]$Text)

    $noBlockComments = [regex]::Replace($Text, "/\*.*?\*/", "", [System.Text.RegularExpressions.RegexOptions]::Singleline)
    $noLineComments = [regex]::Replace($noBlockComments, "//[^\r\n]*", "")
    return $noLineComments
}

# --- Formula mirror (GDD player-movement.md Formulas section, Jump Air Time) ---
# AirTime = (2 * JumpZVelocity) / (GravityScale * abs(WorldGravityZ)). WorldGravityZ magnitude is
# 980 (UE default, unscaled). GravityScale must appear exactly once (not squared) — the GDD explicitly
# warns against reusing UCharacterMovementComponent::GetGravityZ(), which already bakes GravityScale
# in and would silently square it if multiplied again by this formula's own GravityScale term.
$Script:WorldGravityZMagnitude = 980.0
$Script:AirTimeJointBoundMin = 0.5
$Script:AirTimeJointBoundMax = 3.0

function Get-AirTimeSeconds {
    param([double]$JumpZVelocity, [double]$GravityScale)
    return (2.0 * $JumpZVelocity) / ($GravityScale * $Script:WorldGravityZMagnitude)
}

function Test-AirTimeWithinJointBound {
    param([double]$AirTimeSeconds)
    return ($AirTimeSeconds -ge $Script:AirTimeJointBoundMin) -and ($AirTimeSeconds -le $Script:AirTimeJointBoundMax)
}

# --- AC-1: six hard clamp minimums declared with the GDD's exact values ---
function test_six_clamp_minimums_declared_with_gdd_exact_values {
    param([string]$Header)

    Assert-Matches $Header "static\s+constexpr\s+float\s+MinMaxWalkSpeed\s*=\s*100\.0f\s*;" "MinMaxWalkSpeed must be exactly 100.0f (GDD Tuning Knobs)."
    Assert-Matches $Header "static\s+constexpr\s+float\s+MinJumpZVelocity\s*=\s*100\.0f\s*;" "MinJumpZVelocity must be exactly 100.0f (GDD Tuning Knobs)."
    Assert-Matches $Header "static\s+constexpr\s+float\s+MinGravityScale\s*=\s*0\.1f\s*;" "MinGravityScale must be exactly 0.1f (GDD Tuning Knobs)."
    Assert-Matches $Header "static\s+constexpr\s+float\s+MinMaxAcceleration\s*=\s*1000\.0f\s*;" "MinMaxAcceleration must be exactly 1000.0f (GDD Tuning Knobs)."
    Assert-Matches $Header "static\s+constexpr\s+float\s+MinBrakingDecelerationWalking\s*=\s*1000\.0f\s*;" "MinBrakingDecelerationWalking must be exactly 1000.0f (GDD Tuning Knobs)."
    Assert-Matches $Header "static\s+constexpr\s+float\s+MinGroundFriction\s*=\s*1\.0f\s*;" "MinGroundFriction must be exactly 1.0f (GDD Tuning Knobs)."
}

function test_validate_function_applies_each_clamp_individually_before_joint_bound {
    param([string]$Cpp)

    $body = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::ValidateAndClampMovementTuning\s*\(\s*\)"
    $strippedBody = Remove-CppComments $body

    Assert-Matches $strippedBody "MaxWalkSpeed\s*<\s*MinMaxWalkSpeed" "MaxWalkSpeed must be individually clamped against MinMaxWalkSpeed."
    Assert-Matches $strippedBody "MaxWalkSpeed\s*=\s*MinMaxWalkSpeed" "MaxWalkSpeed must be written back to MinMaxWalkSpeed when out of range."
    Assert-Matches $strippedBody "JumpZVelocity\s*<\s*MinJumpZVelocity" "JumpZVelocity must be individually clamped against MinJumpZVelocity."
    Assert-Matches $strippedBody "JumpZVelocity\s*=\s*MinJumpZVelocity" "JumpZVelocity must be written back to MinJumpZVelocity when out of range."
    Assert-Matches $strippedBody "GravityScale\s*<\s*MinGravityScale" "GravityScale must be individually clamped against MinGravityScale."
    Assert-Matches $strippedBody "GravityScale\s*=\s*MinGravityScale" "GravityScale must be written back to MinGravityScale when out of range."
    Assert-Matches $strippedBody "MaxAcceleration\s*<\s*MinMaxAcceleration" "MaxAcceleration must be individually clamped against MinMaxAcceleration."
    Assert-Matches $strippedBody "MaxAcceleration\s*=\s*MinMaxAcceleration" "MaxAcceleration must be written back to MinMaxAcceleration when out of range."
    Assert-Matches $strippedBody "BrakingDecelerationWalking\s*<\s*MinBrakingDecelerationWalking" "BrakingDecelerationWalking must be individually clamped against MinBrakingDecelerationWalking."
    Assert-Matches $strippedBody "BrakingDecelerationWalking\s*=\s*MinBrakingDecelerationWalking" "BrakingDecelerationWalking must be written back to MinBrakingDecelerationWalking when out of range."
    Assert-Matches $strippedBody "GroundFriction\s*<\s*MinGroundFriction" "GroundFriction must be individually clamped against MinGroundFriction."
    Assert-Matches $strippedBody "GroundFriction\s*=\s*MinGroundFriction" "GroundFriction must be written back to MinGroundFriction when out of range."

    # Order: the joint-bound computation (ComputeAirTime call) must textually follow all six
    # individual clamps, per the GDD's mandated validation order ("검증 순서").
    $jointCallMatch = [regex]::Match($strippedBody, "ComputeAirTime\s*\(")
    $lastClampMatches = [regex]::Matches($strippedBody, "GroundFriction\s*=\s*MinGroundFriction")
    if (-not $jointCallMatch.Success) {
        throw "ValidateAndClampMovementTuning must call ComputeAirTime() for the joint bound check."
    }
    if ($lastClampMatches.Count -eq 0) {
        throw "Could not locate the GroundFriction clamp-assignment to verify validation ordering."
    }
    $lastClampMatch = $lastClampMatches[$lastClampMatches.Count - 1]
    if ($jointCallMatch.Index -le $lastClampMatch.Index) {
        throw "Joint bound check (ComputeAirTime) must run AFTER all six individual clamps, per the GDD's mandated validation order."
    }
}

# --- AC-2: AirTime formula matches the GDD exactly (direct value injection, no PIE required) ---
function test_airtime_formula_matches_gdd_boundary_and_regression_examples {
    $airTimeExtremeLow = Get-AirTimeSeconds -JumpZVelocity 800 -GravityScale 0.1     # ~16.33s, reject
    $airTimeExtremeHigh = Get-AirTimeSeconds -JumpZVelocity 100 -GravityScale 1.3    # ~0.157s, reject
    $airTimeDefault = Get-AirTimeSeconds -JumpZVelocity 600 -GravityScale 1.0        # ~1.2245s, pass
    $airTimeNonDefault = Get-AirTimeSeconds -JumpZVelocity 400 -GravityScale 1.3     # ~0.6279s, pass (non-default GravityScale regression)

    if ([Math]::Abs($airTimeExtremeLow - 16.3265) -gt 0.01) {
        throw "AirTime(JumpZVelocity=800, GravityScale=0.1) expected ~16.33s, got $airTimeExtremeLow"
    }
    if ([Math]::Abs($airTimeExtremeHigh - 0.15702) -gt 0.001) {
        throw "AirTime(JumpZVelocity=100, GravityScale=1.3) expected ~0.157s, got $airTimeExtremeHigh"
    }
    if ([Math]::Abs($airTimeDefault - 1.22449) -gt 0.001) {
        throw "AirTime(JumpZVelocity=600, GravityScale=1.0) expected ~1.2245s, got $airTimeDefault"
    }
    if ([Math]::Abs($airTimeNonDefault - 0.62789) -gt 0.001) {
        throw "AirTime(JumpZVelocity=400, GravityScale=1.3) expected ~0.6279s, got $airTimeNonDefault"
    }

    if (Test-AirTimeWithinJointBound $airTimeExtremeLow) {
        throw "AirTime=$airTimeExtremeLow (JumpZVelocity=800, GravityScale=0.1) must be REJECTED — outside the [0.5s, 3.0s] joint bound."
    }
    if (Test-AirTimeWithinJointBound $airTimeExtremeHigh) {
        throw "AirTime=$airTimeExtremeHigh (JumpZVelocity=100, GravityScale=1.3) must be REJECTED — outside the [0.5s, 3.0s] joint bound."
    }
    if (-not (Test-AirTimeWithinJointBound $airTimeDefault)) {
        throw "AirTime=$airTimeDefault (JumpZVelocity=600, GravityScale=1.0) must PASS — within the [0.5s, 3.0s] joint bound."
    }
    if (-not (Test-AirTimeWithinJointBound $airTimeNonDefault)) {
        throw "AirTime=$airTimeNonDefault (JumpZVelocity=400, GravityScale=1.3) must PASS — within the [0.5s, 3.0s] joint bound."
    }
}

function test_cpp_formula_does_not_square_gravityscale {
    param([string]$Cpp)

    $body = Get-FunctionBody $Cpp "float\s+AMoonCharacterBase::ComputeAirTime\s*\(\s*float\s+JumpZVelocity\s*,\s*float\s+GravityScale\s*\)\s*const"
    $strippedBody = Remove-CppComments $body

    Assert-Matches $strippedBody "GetWorld\s*\(\s*\)\s*->\s*GetGravityZ\s*\(\s*\)" "ComputeAirTime must use UWorld::GetGravityZ() (unscaled world gravity), not the CMC's GravityScale-baked GetGravityZ()."
    if ($strippedBody -match "GetCharacterMovement\s*\(\s*\)\s*->\s*GetGravityZ") {
        throw "ComputeAirTime must NOT use UCharacterMovementComponent::GetGravityZ() — it already bakes GravityScale in, which would square GravityScale when multiplied again by the formula's own GravityScale term."
    }
    Assert-Matches $strippedBody "2\.0f\s*\*\s*JumpZVelocity" "AirTime formula numerator must be 2 * JumpZVelocity."
    Assert-Matches $strippedBody "GravityScale\s*\*\s*FMath::Abs\s*\(\s*WorldGravityZ\s*\)" "AirTime formula denominator must be GravityScale * abs(WorldGravityZ)."

    # Defensive regression check: GravityScale must appear exactly once in the (comment-stripped)
    # function body — a second occurrence would indicate an accidental squaring.
    $gravityScaleOccurrences = ([regex]::Matches($strippedBody, "\bGravityScale\b")).Count
    if ($gravityScaleOccurrences -ne 1) {
        throw "GravityScale token appears $gravityScaleOccurrences time(s) in ComputeAirTime's body (comments stripped) — expected exactly 1 (its single use in the formula). A second occurrence would indicate accidental squaring."
    }
}

# --- AC-3: no hardcoded tuning literals (the six GDD clamp minimums), outside their declarations ---
function test_no_bare_numeric_literal_hardcoding_of_clamp_minimums {
    param([string]$Header, [string]$Cpp)

    $combinedSource = (Remove-CppComments $Header) + "`n" + (Remove-CppComments $Cpp)

    $checks = @(
        @{ Property = "MaxWalkSpeed"; Number = "100(\.0+f?)?" },
        @{ Property = "JumpZVelocity"; Number = "100(\.0+f?)?" },
        @{ Property = "GravityScale"; Number = "0\.1f?" },
        @{ Property = "MaxAcceleration"; Number = "1000(\.0+f?)?" },
        @{ Property = "BrakingDecelerationWalking"; Number = "1000(\.0+f?)?" },
        @{ Property = "GroundFriction"; Number = "1(\.0+f?)?" }
    )

    foreach ($check in $checks) {
        # \b before the property name deliberately excludes the designated MinXxx / DefaultXxx /
        # LastValidXxx clamp-constant declarations and references — "Min" + "PropertyName" is a
        # single contiguous identifier with no word-boundary between them, so \bPropertyName\b does
        # not match inside it. Only a BARE, standalone occurrence of the property name directly
        # compared/assigned against the literal number is flagged.
        $pattern = "\b$($check.Property)\b\s*(<=|>=|==|!=|<|>|=)\s*$($check.Number)\b"
        $foundMatches = [regex]::Matches($combinedSource, $pattern)
        if ($foundMatches.Count -gt 0) {
            throw "Found a bare numeric-literal comparison/assignment for '$($check.Property)' (`"$($foundMatches[0].Value)`") outside its designated Min$($check.Property) clamp-constant declaration — AC-3 requires the GDD clamp minimum to be referenced only via the named constant."
        }
    }
}

# --- Revert-to-last-valid-pair policy (not reject-and-fallback-to-engine-defaults) ---
function test_last_valid_pair_state_exists {
    param([string]$Header)

    Assert-Matches $Header "float\s+LastValidJumpZVelocity\s*=" "AMoonCharacterBase must store LastValidJumpZVelocity as private state for the revert policy."
    Assert-Matches $Header "float\s+LastValidGravityScale\s*=" "AMoonCharacterBase must store LastValidGravityScale as private state for the revert policy."
    Assert-Matches $Header "bool\s+bHasValidatedMovementTuning\s*=\s*false\s*;" "AMoonCharacterBase must track whether a valid tuning pair has ever been established (bootstrap-vs-revert distinction)."
}

function test_bootstrap_case_falls_back_to_gdd_defaults_only_when_no_previous_valid_pair_exists {
    param([string]$Cpp)

    $body = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::ValidateAndClampMovementTuning\s*\(\s*\)"
    $strippedBody = Remove-CppComments $body

    $bootstrapAnchor = [regex]::Match($strippedBody, "if\s*\(\s*!\s*bHasValidatedMovementTuning\s*\)")
    if (-not $bootstrapAnchor.Success) {
        throw "ValidateAndClampMovementTuning must branch on '!bHasValidatedMovementTuning' for the bootstrap case."
    }
    $bootstrapBlock = Get-BracedBlockAt -Text $strippedBody -AnchorIndex $bootstrapAnchor.Index

    Assert-Matches $bootstrapBlock "if\s*\(\s*!\s*bCandidateValid\s*\)" "Bootstrap branch must itself check bCandidateValid to distinguish 'first load already valid' from 'first load invalid, no previous pair to revert to'."
    Assert-Matches $bootstrapBlock "JumpZVelocity\s*=\s*DefaultJumpZVelocity\s*;" "Bootstrap-and-invalid case must fall back to DefaultJumpZVelocity (GDD documented default), since no previous valid pair exists yet."
    Assert-Matches $bootstrapBlock "GravityScale\s*=\s*DefaultGravityScale\s*;" "Bootstrap-and-invalid case must fall back to DefaultGravityScale (GDD documented default), since no previous valid pair exists yet."
    Assert-Matches $bootstrapBlock "bHasValidatedMovementTuning\s*=\s*true\s*;" "Bootstrap branch must set bHasValidatedMovementTuning=true after establishing the first valid pair (either as-configured or the GDD default fallback)."
}

function test_rejected_combination_reverts_to_last_valid_pair_without_overwriting_it {
    param([string]$Cpp)

    $body = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::ValidateAndClampMovementTuning\s*\(\s*\)"
    $strippedBody = Remove-CppComments $body

    $revertAnchor = [regex]::Match($strippedBody, "else\s+if\s*\(\s*!\s*bCandidateValid\s*\)")
    if (-not $revertAnchor.Success) {
        throw "ValidateAndClampMovementTuning must have an 'else if (!bCandidateValid)' revert branch, separate from the bootstrap branch."
    }
    $revertBlock = Get-BracedBlockAt -Text $strippedBody -AnchorIndex $revertAnchor.Index

    Assert-Matches $revertBlock "JumpZVelocity\s*=\s*LastValidJumpZVelocity\s*;" "Revert branch must set JumpZVelocity back to LastValidJumpZVelocity (revert-to-last-valid-pair policy, not fallback-to-engine-defaults)."
    Assert-Matches $revertBlock "GravityScale\s*=\s*LastValidGravityScale\s*;" "Revert branch must set GravityScale back to LastValidGravityScale (revert-to-last-valid-pair policy, not fallback-to-engine-defaults)."

    if ($revertBlock -match "LastValidJumpZVelocity\s*=\s*[A-Za-z]" -and $revertBlock -notmatch "LastValidJumpZVelocity\s*=\s*LastValidJumpZVelocity") {
        throw "Revert branch must NOT reassign LastValidJumpZVelocity — the rejected out-of-bound combination must not become the new baseline for future reverts."
    }
    if ($revertBlock -match "LastValidGravityScale\s*=\s*[A-Za-z]" -and $revertBlock -notmatch "LastValidGravityScale\s*=\s*LastValidGravityScale") {
        throw "Revert branch must NOT reassign LastValidGravityScale — the rejected out-of-bound combination must not become the new baseline for future reverts."
    }
}

function test_accepted_combination_updates_last_valid_pair {
    param([string]$Cpp)

    $body = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::ValidateAndClampMovementTuning\s*\(\s*\)"
    $strippedBody = Remove-CppComments $body

    Assert-Matches $strippedBody "LastValidJumpZVelocity\s*=\s*ClampedJumpZVelocity\s*;" "A joint-bound-valid combination must update LastValidJumpZVelocity to the newly-accepted value."
    Assert-Matches $strippedBody "LastValidGravityScale\s*=\s*ClampedGravityScale\s*;" "A joint-bound-valid combination must update LastValidGravityScale to the newly-accepted value."
}

function test_validate_and_clamp_called_once_from_beginplay {
    param([string]$Cpp)

    $beginPlayBody = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::BeginPlay\s*\(\s*\)"
    $strippedBody = Remove-CppComments $beginPlayBody
    Assert-Matches $strippedBody "ValidateAndClampMovementTuning\s*\(\s*\)\s*;" "BeginPlay() must call ValidateAndClampMovementTuning() once at load time (TR-mov-004)."
}

# --- TR-mov-005: Z-impulse injection API ---
function test_inject_z_impulse_api_declared_public_and_blueprintcallable {
    param([string]$Header)

    $declMatch = [regex]::Match($Header, "void\s+InjectZImpulse\s*\(\s*float\s+ZVelocity\s*\)\s*;")
    if (-not $declMatch.Success) {
        throw "AMoonCharacterBase must declare a public InjectZImpulse(float ZVelocity) hook (TR-mov-005)."
    }

    $precedingText = $Header.Substring(0, $declMatch.Index)
    $lastPublic = $precedingText.LastIndexOf("public:")
    $lastPrivate = $precedingText.LastIndexOf("private:")
    $lastProtected = $precedingText.LastIndexOf("protected:")
    if ($lastPublic -lt $lastPrivate -or $lastPublic -lt $lastProtected) {
        throw "InjectZImpulse must be declared in a public: section — external systems (Dash/Evasion, Arena Morphing) must be able to call it directly."
    }

    Assert-Matches $Header "UFUNCTION\s*\(\s*BlueprintCallable[^)]*\)\s*\r?\n\s*void\s+InjectZImpulse" "InjectZImpulse must be UFUNCTION(BlueprintCallable, ...) so Blueprint-side external systems can call it too."
}

function test_inject_z_impulse_uses_established_launchcharacter_z_only_pattern {
    param([string]$Cpp)

    $body = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::InjectZImpulse\s*\(\s*float\s+ZVelocity\s*\)"
    $strippedBody = Remove-CppComments $body

    Assert-Matches $strippedBody "LaunchCharacter\s*\(\s*FVector\s*\(\s*0\.0f\s*,\s*0\.0f\s*,\s*ZVelocity\s*\)\s*,\s*false\s*,\s*true\s*\)" "InjectZImpulse must use LaunchCharacter(FVector(0,0,ZVelocity), false, true) — the same bXYOverride=false/bZOverride=true pattern Input_Jump() already uses for its coyote-time launch."
}

# --- Hard boundary regression: Dash's existing MaxWalkSpeed distance-calc read must be untouched ---
function test_dash_maxwalkspeed_distance_read_left_untouched {
    param([string]$DashCppText)

    Assert-Matches $DashCppText "MoveComp->MaxWalkSpeed\s*\*\s*DashSpeedMultiplier\s*\*\s*DashDuration" "Story 002 must not modify Dash's existing MaxWalkSpeed distance-calc read (explicitly out of scope)."
}

# --- Regression: the asymmetric jump-feel effect (Tick() -> UpdateJumpFeelGravity(), extracted
# from Tick() during the 2026-08-12 /code-review pass — same logic, same call-per-frame contract)
# must MULTIPLY the validated BaseGravityScale, not overwrite it. The original code (predating this
# story) did `MoveComp->GravityScale = bDescending ? FallingGravityScaleMultiplier : 1.0f;` — a flat
# assignment that silently discarded whatever GravityScale ValidateAndClampMovementTuning() had
# just clamped/joint-bound-validated, meaning the joint-bound guarantee only held for the instant
# between BeginPlay() and the first Tick(). Found and fixed during Story 002 implementation review.
function test_tick_multiplies_base_gravity_scale_instead_of_overwriting_it {
    param([string]$Cpp)

    $gravityBody = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::UpdateJumpFeelGravity\s*\(\s*\)"

    Assert-Matches $gravityBody "MoveComp->GravityScale\s*=\s*BaseGravityScale\s*\*\s*\(\s*bDescending\s*\?\s*FallingGravityScaleMultiplier\s*:\s*1\.0f\s*\)\s*;" "UpdateJumpFeelGravity() must compute GravityScale = BaseGravityScale * (bDescending ? FallingGravityScaleMultiplier : 1.0f) — a flat overwrite (GravityScale = FallingGravityScaleMultiplier : 1.0f, without the BaseGravityScale factor) would silently discard the TR-mov-004-validated base value every frame."

    if ($gravityBody -match "MoveComp->GravityScale\s*=\s*bDescending\s*\?\s*FallingGravityScaleMultiplier\s*:\s*1\.0f\s*;") {
        throw "UpdateJumpFeelGravity() must NOT use the old flat-overwrite form (GravityScale = bDescending ? FallingGravityScaleMultiplier : 1.0f) — this discards BaseGravityScale and defeats the AirTime joint-bound guarantee for the entire falling phase."
    }

    $tickBody = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::Tick\s*\(\s*float\s+DeltaTime\s*\)"
    Assert-Matches $tickBody "UpdateJumpFeelGravity\s*\(\s*\)\s*;" "Tick() must call UpdateJumpFeelGravity() once per frame."
}

# --- Regression: FallingGravityScaleMultiplier must be clamped too — an unclamped multiplier
# (e.g. 0 or negative) would silently zero-out or invert descent gravity even with BaseGravityScale
# itself correctly clamped/validated.
function test_falling_gravity_scale_multiplier_is_clamped {
    param([string]$Cpp)

    $validateBody = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::ValidateAndClampMovementTuning\s*\(\s*\)"
    Assert-Matches $validateBody "FallingGravityScaleMultiplier\s*<\s*MinGravityScale" "FallingGravityScaleMultiplier must be individually clamped against MinGravityScale, same as BaseGravityScale — an unclamped multiplier can still break descent gravity even when the base value is safe."
    Assert-Matches $validateBody "FallingGravityScaleMultiplier\s*=\s*MinGravityScale\s*;" "FallingGravityScaleMultiplier must be written back to MinGravityScale when out of range."
}

# --- Regression: BaseGravityScale (not the live, Tick()-derived GetCharacterMovement()->GravityScale)
# must be the value ValidateAndClampMovementTuning() ultimately stores as the source of truth.
function test_base_gravity_scale_is_the_validated_source_of_truth {
    param([string]$Header, [string]$Cpp)

    Assert-Matches $Header "float\s+BaseGravityScale\s*=\s*DefaultGravityScale\s*;" "AMoonCharacterBase must declare a BaseGravityScale member, distinct from the live CMC GravityScale property, defaulting to DefaultGravityScale."

    $validateBody = Get-FunctionBody $Cpp "void\s+AMoonCharacterBase::ValidateAndClampMovementTuning\s*\(\s*\)"
    Assert-Matches $validateBody "BaseGravityScale\s*=\s*MoveComp->GravityScale\s*;" "ValidateAndClampMovementTuning() must write its final validated/reverted GravityScale value into BaseGravityScale, since GetCharacterMovement()->GravityScale becomes a derived per-tick value once Tick() runs."
}

$header = Get-Content -LiteralPath $characterHeader -Raw
$cpp = Get-Content -LiteralPath $characterCpp -Raw
$dashCppText = Get-Content -LiteralPath $dashCpp -Raw

test_six_clamp_minimums_declared_with_gdd_exact_values $header
test_validate_function_applies_each_clamp_individually_before_joint_bound $cpp
test_airtime_formula_matches_gdd_boundary_and_regression_examples
test_cpp_formula_does_not_square_gravityscale $cpp
test_no_bare_numeric_literal_hardcoding_of_clamp_minimums $header $cpp
test_last_valid_pair_state_exists $header
test_bootstrap_case_falls_back_to_gdd_defaults_only_when_no_previous_valid_pair_exists $cpp
test_rejected_combination_reverts_to_last_valid_pair_without_overwriting_it $cpp
test_accepted_combination_updates_last_valid_pair $cpp
test_validate_and_clamp_called_once_from_beginplay $cpp
test_inject_z_impulse_api_declared_public_and_blueprintcallable $header
test_inject_z_impulse_uses_established_launchcharacter_z_only_pattern $cpp
test_dash_maxwalkspeed_distance_read_left_untouched $dashCppText
test_tick_multiplies_base_gravity_scale_instead_of_overwriting_it $cpp
test_falling_gravity_scale_multiplier_is_clamped $cpp
test_base_gravity_scale_is_the_validated_source_of_truth $header $cpp

Write-Host "movement tuning clamp / AirTime joint bound / Z-impulse hook unit checks passed"
