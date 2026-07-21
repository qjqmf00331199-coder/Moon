# Signature Combat Chain — Engine Spike Note

> **Date**: 2026-07-21
> **Status**: PASS — engine feasibility proven
> **Classification**: SPIKE, not production implementation or a formal Vertical Slice

## Question

Can one UE5.8 combat room prove the intended causal grammar — `Blackhole gather → Fire detonation → environment fracture → executable target → core extraction` — while the revised Overdrive rules prevent refresh abuse and provide a clear post-peak boundary?

## Implemented slice

- Blackhole and Fire acquire GAS targets through a Pawn object-type multi-sweep, so world geometry cannot consume the target hit.
- Blackhole changes the target from `Idle` to `Gathered`.
- Fire only changes `Gathered` to `Executable` when a nearby `MoonFracturePillar` fractures.
- `F` core extraction is legal only for an executable target within 350uu. It hides/disables the target and grants the spike Mana/Tension reward through GAS.
- The linked Overdrive state is a fixed 10-second window. It cannot be refreshed, ignores Tension gain while active, pauses Mana regeneration, and applies a 1.5-second post-window Tension lock.

## PIE evidence

Test map: `/Game/Moon/Maps/L_CombatTest`
Input sequence: `1 → 2 → F`

Observed log order:

1. `[MoonSignatureChain] Blackhole setup applied to TargetDummy_0`
2. `[MoonSignatureChain] Environment fractured: MoonFracturePillar_0`
3. `[MoonSignatureChain] Fire detonation armed execution on TargetDummy_0`
4. `[MoonSignatureChain] Core extracted from TargetDummy_0`

Final runtime inspection:

- `TargetDummy_0.SignatureChainState = Extracted`
- `TargetDummy_0.Health = 50` after two 25-point placeholder hits
- `MoonFracturePillar_0.bFractured = true`
- Player Mana returned to 100 and Tension to 0 after the extraction-triggered peak/recovery flow

Overdrive automation regression:

- `Moon.Combat.Overdrive.FixedWindow` — PASS
- `Moon.Combat.Overdrive.RecoveryBoundary` — PASS

## Decision

The causal combat hook is technically viable and is worth carrying into a production-quality Vertical Slice after its owning GDDs and acceptance criteria are approved. This spike does not validate fun, readability under enemy pressure, balance, final destruction fidelity, or player comprehension.

## Next validation

Run a small playtest with no verbal instruction. Pass only if players complete the chain within 60–90 seconds and can explain both why the target became executable and why normal resource pressure returned after Overdrive.

## Explicitly disposable

The cylinder target, cube pillar, hard-coded placeholder damage/rewards, temporary lights, direct actor search, and single-room setup are spike scaffolding. Do not promote them unchanged into production.
