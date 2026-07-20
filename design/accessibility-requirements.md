# Accessibility Requirements — Moon Fragment Hunt

> **Status**: Draft (Sections 1–4 authored for Technical Setup → Pre-Production gate)
> **Author**: Antigravity ux-designer pass (solo mode, 2026-07-20)
> **Last Updated**: 2026-07-20
> **Source Docs**: design/gdd/game-concept.md, design/gdd/combat-hud.md, design/art/art-bible.md
> **Scope**: MVP combat systems only (Player Movement, Spell Casting, Dash/Evasion, Combo/Tension, Luna Overdrive, Combat HUD). Narrative/audio/menu accessibility deferred.

---

## 1. Accessibility Tier Declaration

**Tier**: **Standard** (WCAG 2.1 AA for UI text; game-specific combat accessibility at "Recommended" level)

This is a fast-paced, precision combat game. Full accessibility (e.g., single-switch play, zero-reaction-time alternatives for just-dodge) would require fundamental redesign of the core loop. The declared tier commits to:
- No accessibility feature that requires redesigning a core mechanical contract
- All critical information available via at least **two independent sensory channels**
- No player-facing text below 16px equivalent at 1080p
- Colorblind modes supported for all gameplay-critical color signals (see art-bible.md Section 4)

---

## 2. Visual Accessibility

### 2.1 HUD Legibility

| Requirement | Spec |
|---|---|
| Minimum HUD text size | 18px at 1080p (scales with resolution) |
| Contrast ratio (text on background) | ≥ 4.5:1 (WCAG AA) |
| HUD element minimum touch/click target | N/A (controller/KBM only per current scope) |
| HUD scaling | HUD scale slider: 80%–150% of default |

### 2.2 Motion Sensitivity

| Requirement | Spec |
|---|---|
| Camera shake intensity | Slider: Off / Low / Medium (default) / High. **Off must be visually unambiguous** — no gameplay info lost when shake is off. |
| Screen flash (Overdrive onset) | Must be ≤ 3 flashes/second, duration ≤ 0.3s per flash. Photosensitive safety: no sustained strobing during Overdrive |
| Overdrive vignette | Toggle: On (default) / Off. Disabling it must not remove the Overdrive state signal — color tint remains active when vignette is off. |

### 2.3 Colorblind Support

Every gameplay-critical color distinction must have a **non-color backup** (shape, icon, or animation):

| Critical Signal | Primary Color | Backup Cue | Colorblind Mode |
|---|---|---|---|
| Overdrive Active | Global crimson tint | Full-screen VFX + UI restructure (∞ symbols) + audio sting | Optional: high-contrast mode replaces red with magenta+pattern |
| Executable Prompt | Amber F-key icon | Distinctive pulse animation (0.5s cycle) + proximity distance indicator | Icon adds a border stroke |
| Tension at 90%+ | Gauge fringe turns red | Gauge pulses (animation) + audio feedback | Icon adds hatching pattern |
| Enemy Archetype (Ranged) | No color-only distinction | Silhouette + visible projectile device (art-bible Section 3) | No change needed — shape-based |
| i-frame active | White flash | Brief player invincibility visual (art-bible) | White flash is colorblind-safe |

**Colorblind mode options to implement** (minimum):
- [ ] Protanopia/Deuteranopia mode: replaces red signal colors with magenta + pattern overlay
- [ ] Tritanopia mode: replaces blue accent with green-yellow
- [ ] High-contrast mode: all semantic colors replaced with black/white + pattern

> Implementation note: Colorblind mode is an option in accessibility settings, not an auto-detect. Auto-detection is unreliable; player should choose explicitly.

---

## 3. Motor Accessibility

### 3.1 Input Remapping

| Requirement | Spec |
|---|---|
| Full keybinding remapping | All keyboard/mouse bindings remappable (Enhanced Input system — UE5.8 supports per-action binding override) |
| Controller button remapping | All controller face buttons + shoulder buttons remappable |
| Stick dead-zone | Adjustable dead-zone per stick: 0–30% |
| Toggle vs. Hold | Any sustained-hold action (e.g., hold dash = sprint) must offer a Toggle alternative |

### 3.2 Timing Sensitivity

The core loop relies on skill-gated timing (just-dodge window = 0.2s per `dash-evasion.md`). No "easy mode" that removes timing is in scope — but:

| Feature | Spec |
|---|---|
| Just-dodge window (default 0.2s) | NOT adjustable — core mechanic. Instead: clear telegraph from Enemy AI (≥0.4s `min_telegraph_window` per `enemy-ai-base.md` — already mandated) |
| Spell cast gating | No hold-to-cast; all spells instant-cast (GDD mandate). No motor accessibility conflict. |
| Execution prompt timing | `executable_duration = 3.0s` (registry). Ample — no adjustment needed. |

---

## 4. Cognitive Accessibility

### 4.1 Concurrent Information Load

Current HUD: HP + Dash charges + Tension gauge + Mana + 3 cooldowns + Executable prompt (when active) + Overdrive timer (when active). Peak = 8 concurrent trackers.

Mitigations already designed-in (per combat-hud.md):
- Overdrive collapses Mana + cooldowns to ∞ — removes 4 of 8 trackers at peak spectacle
- HUD designed for 0.2s peripheral glance (per combat-hud.md overview)
- Executable prompt appears on the enemy (diegetic location), not in a fixed HUD zone

Additional mitigation required:
- [ ] **Simplified HUD mode**: removes or minimizes numeric displays; keeps only gauge shapes. Aim: ≤4 concurrent visual elements. Deferred to Polish phase.

### 4.2 Information Priority Hierarchy

When everything is happening at once, players need a visual hierarchy that makes the most important information survive:

1. **Safety-critical**: Player HP (bottom-left, large) — always visible
2. **Opportunity**: Executable prompt (on-enemy) — contextual, diegetic
3. **Resource state**: Mana + cooldowns (bottom-center) — collapses during Overdrive
4. **Momentum**: Tension gauge (bottom-right) — always visible, animation-driven
5. **Power state**: Overdrive timer (replaces tension) — unmissable by design

This hierarchy is intentional and must be preserved in any HUD scale or layout variant.

---

## 5. Deferred Items (Post-MVP)

| Item | Milestone |
|---|---|
| Audio accessibility (subtitle/captions for VO) | Narrative GDD milestone |
| Menu/UI screen reader support | Beta milestone |
| Simplified HUD mode | Polish phase |
| Auto-detection colorblind | Will not implement (unreliable; player choice instead) |
| Single-switch / switch access | Will not implement (incompatible with core just-dodge mechanic design) |

---

## Acceptance Criteria

1. At 1080p, all HUD text elements render at ≥ 18px and ≥ 4.5:1 contrast ratio against their background.
2. No gameplay-critical color distinction relies on color as the sole differentiator — each has a documented non-color backup cue.
3. Screen flash during Overdrive onset: ≤ 3 events/second, ≤ 0.3s per flash.
4. Camera shake Off setting: verified that no gameplay-critical information is conveyed only through shake (AC: play the full tutorial with shake=Off and all missions remain completable).
5. All keyboard bindings remappable; all controller face/shoulder buttons remappable.
6. Colorblind mode (protanopia at minimum) available in accessibility settings and verified to not introduce new same-color ambiguities.
