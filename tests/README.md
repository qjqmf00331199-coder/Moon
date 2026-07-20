# Test Infrastructure

**Engine**: Unreal Engine 5.8
**Test Framework**: UE Automation (Automation Spec + Functional Tests)
**CI**: `.github/workflows/tests.yml`
**Setup date**: 2026-07-18

## Directory Layout

```
tests/
  unit/           # Isolated unit tests (formulas, state machines, logic)
  integration/    # Cross-system and checkpoint/respawn round-trip tests
  smoke/          # Critical path test list for /smoke-check gate
  evidence/       # Screenshot logs and manual test sign-off records
```

C++ automation test classes live in the module at `Moon/Source/Tests/`
(see `Moon/Source/Tests/README.md`). This root `tests/` tree holds the
engine-agnostic test organisation, smoke list, and manual evidence that the
studio skills (`/qa-plan`, `/smoke-check`, `/story-done`) read.

## Running Tests

Headless:
```
UnrealEditor-Cmd.exe "D:/moon-fragment-hunt/Moon/Moon.uproject" \
  -nullrhi -nosound -unattended \
  -ExecCmds="Automation RunTests Moon.; Quit" -log
```

Interactive: Session Frontend → Automation → select `Moon.` tests.

## Test Naming

- **Files**: `[system]_[feature]_test.[ext]`
- **Functions / spec descriptions**: `test_[scenario]_[expected]`
- **Automation category**: `Moon.[System].[Feature]`
- **Example**: `Moon.HealthDamage.ApplyDamage` → `test_bBypassDefense_does_not_bypass_State_Invulnerable`

## Story Type → Test Evidence

| Story Type | Required Evidence | Location | Gate |
|---|---|---|---|
| Logic | Automated unit test — must pass | `tests/unit/[system]/` | BLOCKING |
| Integration | Integration test OR playtest doc | `tests/integration/[system]/` | BLOCKING |
| Visual/Feel | Screenshot + lead sign-off | `tests/evidence/` | ADVISORY |
| UI | Manual walkthrough OR interaction test | `tests/evidence/` | ADVISORY |
| Config/Data | Smoke check pass | `production/qa/smoke-*.md` | ADVISORY |

## CI

Tests run automatically on every push to `main` and every pull request.
A failed suite blocks merging. UE CI requires a **self-hosted runner** with
Unreal Editor 5.8 installed and `UE_EDITOR_PATH` set — see the workflow file.
