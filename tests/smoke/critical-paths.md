# Smoke Test: Critical Paths

**Purpose**: Run these checks in under 15 minutes before any QA hand-off.
**Run via**: `/smoke-check` (which reads this file)
**Update**: Add new entries when new core systems are implemented.

## Core Stability (always run)

1. Editor/PIE launches the combat test map without crash
2. Play session starts and player pawn spawns possessed
3. Inputs (move/look/cast/dash) register without freeze

## Core Mechanic (update per sprint — MVP systems)

<!-- Add each MVP system's primary path as it is implemented -->
4. Player can move, jump, air-control; camera follows (Player Movement / Camera)
5. Spell cast applies damage through the single ApplyDamage entry; enemy HP drops (Spell / Health-Damage)
6. Dash grants i-frames; just-dodge grants State.Executable (Dash/Evasion)
7. Enemy AI perceives (sight/hearing), telegraphs, commits attack (Enemy AI)
8. Tension gauge fills on hits → Overdrive triggers → CostBypass active (Combo/Tension → Luna Overdrive)
9. Combat HUD mirrors HP / mana / cooldowns / gauge / overdrive without gating gameplay (Combat HUD)

## Data Integrity

10. Death triggers instant checkpoint respawn — no loading screen, transform + attributes restored (Checkpoint)

## Performance

11. No visible frame-rate drops on target hardware (60fps / 16.6ms target)
12. No memory growth over 5 minutes of the core loop
