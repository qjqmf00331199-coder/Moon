# Art Bible — Moon Fragment Hunt

> **Status**: Draft — Sections 1–4 complete (gate minimum for Technical Setup → Pre-Production)
> **Author**: user + Antigravity art-director pass (solo mode, 2026-07-20)
> **Last Updated**: 2026-07-20
> **Source Docs**: design/gdd/game-concept.md, design/gdd/systems-index.md, design/gdd/combat-hud.md
> **Art Director Sign-Off (AD-ART-BIBLE)**: Skipped — Solo mode (production/review-mode.txt)
> **Scope Note**: Sections 1–4 authored for Technical Setup gate. Sections 5–9 to be completed before Vertical Slice asset production begins. Every visual decision in these sections flows from game-concept.md's "Dopamine Driven Design" pillar and the Blood Moon visual identity.

---

## Section 1: Visual Identity Statement

### One-Line Visual Rule

> **"Controlled darkness that detonates."**  
> Every visual decision should feel restrained and precise — until the moment it doesn't. The game's normal state is a tightly-lit, high-contrast deep-space moonscape (cool, controlled, lethal). Luna Overdrive is the detonation: the same world inverted in crimson, the same effects scaled beyond reason, the same player character doing the same moves — only louder, faster, redder. The contrast *between* states is the payoff.

### Supporting Principles

1. **Cool restraint → Crimson eruption** *(serves Dopamine Driven Design pillar)*  
   Visual test: "Does this element feel appropriately restrained in the normal state, and would it read as amplified/inverted in Overdrive?" If normal-state art already reads as "Overdrive-level," it has nowhere to go — scale it back.

2. **Readable destruction** *(serves Environmental Chain-Destruction pillar)*  
   Visual test: "Can the player track what just broke, what is breaking, and what can still break — in peripheral vision, during motion?" Destruction effects must communicate intent (fragmentation direction, chain targets) before they communicate spectacle.

3. **Player-character as focal anchor** *(serves moment-to-moment gameplay readability)*  
   Visual test: "When the screen is at peak chaos (Overdrive + chain-break + camera shake), can the player still see their character and one enemy target?" Background must never compete with foreground character/enemy contrast. Silhouette > surface detail.

---

## Section 2: Mood & Atmosphere

### Normal Combat State (Pre-Overdrive)

| Property | Specification |
|---|---|
| **Primary emotion** | Focused aggression — predatory, controlled, building |
| **Lighting character** | Cool-white moonlight (color temp ~6500K), high contrast, hard shadows. Rim-lit player character in pale blue. Background: deep blue-black void with faint stellar debris. |
| **Atmospheric descriptors** | Sharp · Lunar · Lethal · Deliberate · Pressurized |
| **Energy level** | Measured → accelerating. The world matches the tension gauge: sparse, clean at gauge=0; more particle debris, more lens flare at gauge=70%+. |
| **Key visual cues** | Tension gauge glow pulse begins at 90%+ (matches `tension_charged_highlight_threshold = 0.90`). Spell elements: Blackhole = deep violet; Fire = pale amber; Lightning = electric white-blue. NOT warm colors in normal state. |

### Luna Overdrive State (Blood Moon Active)

| Property | Specification |
|---|---|
| **Primary emotion** | Triumphant catharsis — unstoppable, reckless, ecstatic |
| **Lighting character** | Crimson-red global tint (color temp ~2200K), high saturation, bloom cranked. Same moonlight source now blood-red. Hard shadows preserved for readability. |
| **Atmospheric descriptors** | Violent · Burning · Unstoppable · Sensory overload · Righteous |
| **Energy level** | Maximum. Particles fill the screen. Every spell impact doubles in scale. Mana/cooldown UI collapses to ∞ symbols (combat-hud.md Rule 7). |
| **Key visual cues** | Overdrive active = global lighting shift + player character outline glows crimson + tension gauge replaced by overdrive timer. Must be visually unmistakable from 3m screen distance. |

### Death / Defeat State

| Property | Specification |
|---|---|
| **Primary emotion** | Punished, momentary — not hopeless |
| **Lighting character** | Desaturation vignette (not full black/white). World goes cold and dark briefly. Checkpoint restore is fast — the darkness should feel like a blink, not a funeral. |
| **Energy level** | Near-zero for ~0.5 seconds, then instantly back to normal. |

### Victory / Execution State (Core Extraction)

| Property | Specification |
|---|---|
| **Primary emotion** | Visceral satisfaction — the moment the system pays off |
| **Lighting character** | Brief camera push-in, increased vignette around the target, spotlight on the extraction moment. Color temperature stays in the active state (if Overdrive, stays crimson). |
| **Energy level** | Spike then release. Particle burst on core extraction, then clean. |

---

## Section 3: Shape Language

### Core Shape Philosophy: Angular Aggression + Circular Tension

The world uses angular, crystalline geometry (moon fragments, spires, arena structures) as its structural grammar — harsh, breakable, inorganic. Player-facing gameplay elements (spell effects, gauge UI, overdrive aura) use circular/radial forms — representing energy that wants to expand outward. The visual tension between the angular world trying to contain circular energy is the game's shape metaphor.

### Character Silhouettes

| Character Type | Silhouette Rule |
|---|---|
| **Player character** | Medium-mass upright humanoid with a deliberately oversized cape/mantle that billows during movement — creates a recognizable, flowing silhouette even in particle-heavy environments. Readable at 1/20th of screen height. |
| **Grunt enemy** | Compact, low center of gravity, squared shoulders. Opposite proportion to player. Crowd-readable in swarms. |
| **Ranged enemy** | Tall, thin, with a visible projectile-emitting limb/device. Must read as "ranged threat" from across the arena without color cues. |
| **Elite/Boss** | 2–3× player height, with a clear geometric "weak point" marking (shield glyph, glowing core). The core target of the execution system must be identifiable at-a-glance. |

### Environment Geometry

- **Dominant form**: Angular, fragmented — moon rock in various stages of breaking. No smooth organic curves in structural elements.
- **Breakability cue**: Destructible objects use a visible crack/fracture pattern and a faint pulsing emission at fracture points. Non-destructible objects have no cracks.
- **Depth layering**: Foreground (playable surface) high detail, midground (arena walls) medium detail with atmospheric scattering, background (void / moon surface) low detail with volumetric fog — creates depth without competing with gameplay.

### UI Shape Grammar

- The HUD is screen-space (not diegetic) but echoes the world's angular vocabulary: tension gauge is a segmented arc (not a smooth bar), health is a crystalline shard column.
- UI shapes are thinner/lighter than world geometry — they must feel like they're *overlaid* on the world, not part of it.
- Circular motif for Overdrive timer (radiating arc countdown) — the one UI element that uses the circular-energy grammar.

---

## Section 4: Color System

### Primary Palette

| Color Name | Role | Hex Guidance | Meaning in World |
|---|---|---|---|
| **Void Black** | Background base | `#050810` (near-black, blue-tinted) | The absence between moon fragments — infinite space |
| **Lunar White** | Player character rim, UI accent | `#D8E8FF` (cool white, slightly blue) | Moonlight — the player's anchor in the dark |
| **Deep Blue** | Ambient fill, mid-ground tones | `#1A2A4A` | Moon surface in shadow — the "resting" world |
| **Crimson Blood** | Overdrive global tint, Overdrive UI | `#CC1A1A` (saturated red) | Blood Moon — power at maximum |
| **Overdrive Accent** | Player outline during Overdrive, core extraction flash | `#FF4444` (brighter crimson) | The payoff — brighter than the threat |

### Spell Element Colors (Semantic)

| Spell | Color | Rationale |
|---|---|---|
| **Blackhole** | Deep violet-black `#2A0A4A` with event-horizon electric blue `#4488FF` edge | Singularity — darkness that attracts |
| **Fire** | Pale amber-white `#FFD070` (NOT orange) | Lunar fire — cold origin, hot effect; must not clash with Crimson Overdrive state |
| **Lightning** | Electric white-blue `#88CCFF` with purple flash | Static discharge in a void — sharp, instantaneous |

### State-Based Semantic Color Contracts

| State | Color Meaning | Player Communication |
|---|---|---|
| Normal | Blue/cool tones = safe/charging, violet = Blackhole | Building toward payoff |
| Tension ≥ 90% | Tension gauge pulses to Lunar White + adds Crimson fringe | "You're almost there" |
| Overdrive Active | Global shift to Crimson — ALL non-critical UI collapses | Maximum state — unmistakable |
| Executable Prompt | Amber/gold `#FFD070` F-key prompt pulses on target | Opportunity — act now |
| Invulnerable (i-frame) | Brief white flash on player | Feedback that i-frame was active |
| Death/Low HP | Vignette desaturation (no specific color addition) | Danger without panic color |

### Colorblind Safety Contracts

All semantic state distinctions must be supported by **at least two of three**: shape/icon, color, animation/timing.

| Risk Pair | Backup Cue |
|---|---|
| Crimson Overdrive vs. Amber Executable Prompt | Shape: Overdrive is full-screen; Executable is a pulsing icon on a specific target |
| Fire spell (amber) vs. Executable prompt (amber) | Animation: Executable prompt has a distinctive pulse rhythm (0.5s on/off); fire spell is constant glow |
| Blackhole violet vs. Void background black | Brightness: Blackhole has a high-brightness edge effect even in dark backgrounds; no gameplay state relies on distinguishing violet from black for safety-critical information |

---

## Sections 5–9: Pending

> ⏳ **Sections 5–9** (Character Design Direction, Environment Design Language, UI/HUD Visual Direction, Asset Standards, Style Prohibitions) are deferred to the Vertical Slice design phase.  
> **Gate note**: Sections 1–4 are sufficient for the Technical Setup → Pre-Production gate. Asset production begins in Vertical Slice milestone; sections 5–9 must be complete before any outsourced asset creation starts.

Planned section owners:
- Section 5 (Character Design): art-director agent + `/asset-spec` skill output
- Section 6 (Environment Design): art-director agent + level design input
- Section 7 (UI/HUD Visual): art-director + ux-designer + combat-hud.md alignment
- Section 8 (Asset Standards): technical-artist + UE5.8 budget constraints from `docs/.claude/docs/technical-preferences.md`
- Section 9 (Style Prohibitions): art-director synthesis pass after 5–8 are complete
