# Unreal Automation Tests — Moon

C++ automation tests for the `Moon` module using the UE Automation Testing Framework.

## Running

Interactive: Session Frontend → Automation → select `Moon.` tests.

Headless:
```
UnrealEditor-Cmd.exe "D:/moon-fragment-hunt/Moon/Moon.uproject" \
  -nullrhi -nosound -unattended \
  -ExecCmds="Automation RunTests Moon.; Quit" -log
```

## Conventions

- Test class naming: `F[SystemName]Test` (e.g. `FHealthDamageApplyDamageTest`)
- Automation category: `Moon.[System].[Feature]` (e.g. `Moon.SpellCasting.CastRateFloor`)
- Prefer `DEFINE_SPEC` / Automation Spec for BDD-style gameplay logic tests.
- Test source files build into a test target; do not ship in the game module.
  Gate them behind `WITH_AUTOMATION_TESTS` or place in an editor/test target.

## Determinism rules (per coding-standards.md)

- No random seeds, no wall-clock/time-dependent assertions.
- Each test sets up and tears down its own state; no execution-order dependency.
- No external API / disk / DB calls — use dependency injection.
- Boundary-value tests may use exact magic numbers (the number IS the point).
