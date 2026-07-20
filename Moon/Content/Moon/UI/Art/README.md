# Moon Combat HUD Art — UE 5.8 Import Guide

## Delivered source art

`Generated/` contains the alpha PNG source files. `ToImport/` contains the 12 individual files intended for import into the Unreal Content Browser. `Deprecated/` holds a superseded vertical mana-frame draft; do not import it.

| Asset | UMG use |
|---|---|
| `F_UI_HUD_HealthCrystalFrame.png` | Health `Overlay` frame |
| `F_UI_HUD_ManaFrame_Horizontal.png` | Mana `Overlay` frame |
| `F_UI_HUD_SpellSlotFrame.png` | Reusable spell-slot frame |
| `F_UI_HUD_DashPipFrame.png` | Reusable dash-charge frame |
| `F_UI_HUD_TensionArcFrame.png` | Tension / Overdrive visual frame |
| `I_UI_Spell_*.png` | Blackhole, Fire, Lightning slot icons |
| `I_UI_HUD_*.png` | Contextual HUD state icons |

## Texture settings

After importing, set every texture to **LOD Group: UI**, **Mip Gen Settings: NoMipmaps**, and **Compression Settings: TC Editor Icon**. This keeps the small HUD linework crisp. Do not use the generated source atlases in UMG; use the individual files.

## Widget mapping

- `WBP_CombatHUD/HealthCluster`: `Overlay` containing a blue `ProgressBar` under `F_UI_HUD_HealthCrystalFrame`.
- `WBP_CombatHUD/ManaCluster`: a blue `ProgressBar` under `F_UI_HUD_ManaFrame_Horizontal`; place three reusable `WBP_SpellCooldownSlot` widgets beneath it.
- `WBP_CombatHUD/RightCluster`: two `WBP_DashPip` widgets plus the `F_UI_HUD_TensionArcFrame` overlay.
- The existing Health/Mana event bindings remain gameplay's source of truth. The art layer must never own combat values.

## Animation contract

- Health loss: 0.12 s scale pulse, then 0.18 s return; trigger only on `OnHealthChanged`.
- Mana cast: 0.10 s opacity pulse on the mana fill; trigger only on `OnManaChanged`.
- Cooldown: use a radial material wipe per slot; do not animate with a per-frame Blueprint tick.
- Tension charged: 0.60 s looping opacity pulse while real Tension is at least 90%.
- Overdrive: replace the tension cluster with `I_UI_HUD_OverdriveInfinity`; use the existing real duration source for the radial countdown.
- Low health: show the existing vignette only below the real health threshold; no color-only warning.

## Compact PC layout

At 1920x1080, keep all permanent HUD in the lower 16% of the screen with a 4% safe margin. Default art scale is 1.0, with the project HUD scale setting permitted to range from 0.8 to 1.5.
