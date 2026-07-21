# UE 5.8 Combat HUD — Visual Implementation Handoff

> **Status**: Ready for UMG assembly
> **Source UX**: `design/ux/combat-hud.md`
> **Art package**: `Moon/Content/Moon/UI/Art/`
> **Scope**: Compact PC normal-combat HUD; the existing gameplay-event bindings remain unchanged.

## Implemented result — Lunar Fracture v2 (2026-07-21)

- `WBP_CombatHUD` now uses the v2 Health, Mana, Spell Slot, Blackhole, and combined Tension/Dash crescent assets from `/Game/Moon/UI/Art`.
- The previous absolute 1920px Tension/Dash placement was replaced with a relative bottom-right anchor. Health remains bottom-left; Mana and spell slots remain bottom-centre.
- Permanent ornament and idle glow were reduced. The frames use a common dark lunar-glass body, thin moonlight edge, and one asymmetric fracture notch.
- The Tension and Dash cluster is one crescent instrument. Runtime Tension is still represented by the existing horizontal fill inside it; a radial material is intentionally deferred.

### Aspect-ratio evidence

| Requested PIE client | Purpose | Result | Evidence |
|---|---|---|---|
| 1920×1080, 16:9 | Baseline composition | PASS — left/centre/right clusters remain separated | `production/qa/hud-1920x1080.png` |
| 2560×1080, 21:9 | Ultrawide anchoring | PASS — side clusters move outward while centre stays fixed | `production/qa/hud-2560x1080.png` |

The OS title bar reduced the captured client area's physical height below 1080 in the screenshots. The PIE request sizes and relative-anchor behavior were still verified; a later fullscreen capture should be used for final pixel-perfect certification.

## Required UMG hierarchy

```text
WBP_CombatHUD
└─ CanvasPanel_Root (non-focusable)
   ├─ HealthCluster (bottom-left)
   │  ├─ ProgressBar_Health (existing binding)
   │  └─ Image_HealthCrystalFrame
   ├─ ManaCluster (bottom-center)
   │  ├─ ProgressBar_Mana (existing binding)
   │  ├─ Image_ManaFrame
   │  └─ HorizontalBox_SpellSlots
   │     ├─ WBP_SpellCooldownSlot_Blackhole
   │     ├─ WBP_SpellCooldownSlot_Fire
   │     └─ WBP_SpellCooldownSlot_Lightning
   ├─ RightCluster (bottom-right)
   │  ├─ HorizontalBox_DashPips
   │  └─ Overlay_TensionOrOverdrive
   └─ Border_LowHealthVignette (existing binding)
```

## Safe implementation rules

1. Use relative anchors and a `ScaleBox`; do not use fixed full-HD positions.
2. Use the individual transparent PNGs, not the mockup screenshot or the source atlases.
3. Keep `WBP_CombatHUD` read-only: GAS/gameplay owns every value and every transition decision.
4. Place the art on top of fills using `Overlay`; frames do not need a material.
5. Cooldown and radial tension fills use a UI-domain material with one scalar parameter. Create one dynamic material instance per active widget, not per update.
6. The HUD is non-focusable and uses `Set Input Mode Game Only` in gameplay.

## Verification checklist

- Health and mana still respond to the existing events.
- At 1920x1080, permanent HUD remains within the lower 16% and leaves the screen centre unobstructed.
- At 80% and 150% HUD scale, left, centre, and right clusters do not overlap.
- Low-health, charged-tension, and Overdrive states retain their non-colour cue (crack, pulse, structural replacement).
- Texture import settings are UI / NoMipmaps / TC Editor Icon.
