# Systems Index: Moon Fragment Hunt — 전투 시스템 확장

> **Status**: Draft
> **Created**: 2026-07-16
> **Last Updated**: 2026-07-17
> **Source Concept**: design/gdd/game-concept.md

---

## Overview

이 문서는 게임 전체가 아니라 `game-concept.md`에 정의된 **전투 시스템 확장(Dopamine Driven Design)** 범위를 다룬다.
프로젝트에 기존 베이스(이동/카메라/기본 마법시스템)가 전혀 없으므로, Foundation 레이어부터 새로 설계한다.
핵심 목표는 콤보 축적 → 루나 오버드라이브(각성) → 스펠 위빙 시너지 → 환경 연쇄파괴 → 코어 적출 처형 →
보스전 아레나 몰핑으로 이어지는 텐션 곡선 하나를 완성하는 것. Progression/Economy/Persistence/Narrative/Audio
카테고리는 이 확장 범위 밖이므로 이 인덱스에서 제외.

---

## Systems Enumeration

| # | System Name | Category | Priority | Status | Design Doc | Depends On |
|---|-------------|----------|----------|--------|------------|------------|
| 1 | Player Movement (inferred) | Core | MVP | Approved | design/gdd/player-movement.md | — |
| 2 | Camera System (base) (inferred) | Core | MVP | Approved | design/gdd/camera-system-base.md | Player Movement (mutual — see player-movement.md Dependencies) |
| 3 | Health/Damage Core (inferred) | Core | MVP | Approved | design/gdd/health-damage-core.md | — |
| 4 | Enemy AI (base) (inferred) | Gameplay | MVP | Approved | design/gdd/enemy-ai-base.md | Health/Damage Core |
| 5 | Destructible Geometry (inferred) | Core | Vertical Slice | Not Started | — | — |
| 6 | Spell Casting (base) (inferred) | Gameplay | MVP | Approved | design/gdd/spell-casting-base.md | Player Movement, Health/Damage Core |
| 7 | Dash/Evasion | Gameplay | MVP | Approved | design/gdd/dash-evasion.md | Player Movement, Camera System (base), Health/Damage Core, Enemy AI (base) |
| 8 | Status Effect (inferred) | Gameplay | Vertical Slice | Not Started | — | Health/Damage Core |
| 9 | Combo/Tension Gauge | Gameplay | MVP | Needs Revision | design/gdd/combo-tension-gauge.md | Spell Casting (base), Health/Damage Core, Dash/Evasion |
| 10 | Spell Weaving / Synergy | Gameplay | Vertical Slice | Not Started | — | Spell Casting (base), Status Effect |
| 11 | Luna Overdrive (Blood Moon) | Gameplay | MVP | Needs Revision | design/gdd/luna-overdrive.md | Combo/Tension Gauge, Spell Casting (base), Health/Damage Core (soft — added 2026-07-17 during luna-overdrive.md authoring: player Death subscription for forced overdrive end) |
| 12 | Core Extraction Execution | Gameplay | Vertical Slice | Not Started | — | Dash/Evasion, Enemy AI (base), Health/Damage Core |
| 13 | Enemy Elite Shield (inferred) | Gameplay | Alpha | Not Started | — | Enemy AI (base), Health/Damage Core |
| 14 | Super Armor / CC Interrupt (inferred) | Gameplay | Alpha | Not Started | — | Status Effect, Enemy AI (base), Health/Damage Core |
| 15 | Environmental Chain-Destruction | Gameplay | Vertical Slice | Not Started | — | Destructible Geometry, Status Effect, Health/Damage Core |
| 16 | Boss Phase (inferred) | Gameplay | Alpha | Not Started | — | Enemy AI (base), Health/Damage Core |
| 17 | Arena Morphing | Gameplay | Alpha | Not Started | — | Boss Phase, Destructible Geometry, Dash/Evasion, Camera System (base) |
| 18 | Overdrive Visual State (inferred) | UI | Vertical Slice | Not Started | — | Luna Overdrive |
| 19 | Execution Camera/Cutscene (inferred) | UI | Vertical Slice | Not Started | — | Core Extraction Execution, Camera System (base) |
| 20 | Combat HUD (inferred) | UI | MVP | Needs Revision | design/gdd/combat-hud.md | Combo/Tension Gauge, Luna Overdrive, Core Extraction Execution (미설계 — 가정 인터페이스), Health/Damage Core, Spell Casting (base), Dash/Evasion (3건은 2026-07-17 combat-hud.md 설계 시 명기 — 상류 UI Requirements 위임분) |

---

## Categories

| Category | Description | Typical Systems |
|----------|-------------|-----------------|
| **Core** | Foundation systems everything depends on | Player Movement, Camera System, Health/Damage Core, Destructible Geometry |
| **Gameplay** | The systems that make the game fun | Spell Casting, Combat Combo/Overdrive, Spell Weaving, Execution, Enemy AI, Boss Phase, Arena Morphing |
| **UI** | Player-facing information displays | Combat HUD, Overdrive Visual State, Execution Camera |

---

## Priority Tiers

| Tier | Definition | Target Milestone | Design Urgency |
|------|------------|------------------|----------------|
| **MVP** | Core combat loop 성립 + 시그니처 오버드라이브 각성 체험 가능 | First playable prototype | Design FIRST |
| **Vertical Slice** | 파괴/처형/시너지까지 포함한 "Endless Catharsis" 루프 완성 | Vertical slice / demo | Design SECOND |
| **Alpha** | 보스전 전용 콘텐츠 (실드, 슈퍼아머, 페이즈, 아레나 몰핑) | Alpha milestone | Design THIRD |
| **Full Vision** | 해당 없음 (이 확장 범위 내 정의된 것 없음) | — | — |

---

## Dependency Map

### Foundation Layer (no dependencies)

1. Player Movement — 모든 이동 기반 행동(대쉬, 체공전투)의 전제
2. Health/Damage Core — 모든 데미지/사망 처리의 공용 기반
3. Destructible Geometry — 포자/다리/투기장 붕괴 전부가 공유하는 파괴 지오메트리 기반 (GC 스파이크 완료)

### Core Layer (depends on foundation)

1. Camera System (base) — depends on: Player Movement (mutual reference, not a true cycle — see player-movement.md Dependencies)
2. Enemy AI (base) — depends on: Health/Damage Core
3. Spell Casting (base) — depends on: Player Movement, Health/Damage Core
4. Dash/Evasion — depends on: Player Movement, Camera System (base), **Health/Damage Core** (added 2026-07-16 during health-damage-core.md authoring — Dash/Evasion consumes `State.Invulnerable`/`State.Executable` tags owned by Health/Damage Core, gap missing from original index)
5. Status Effect — depends on: Health/Damage Core

### Feature Layer (depends on core)

1. Combo/Tension Gauge — depends on: Spell Casting (base), Health/Damage Core, Dash/Evasion
2. Spell Weaving / Synergy — depends on: Spell Casting (base), Status Effect
3. Luna Overdrive (Blood Moon) — depends on: Combo/Tension Gauge, Spell Casting (base), Health/Damage Core (soft — player Death subscription, added 2026-07-17)
4. Core Extraction Execution — depends on: Dash/Evasion, Enemy AI (base), Health/Damage Core
5. Enemy Elite Shield — depends on: Enemy AI (base), Health/Damage Core
6. Super Armor / CC Interrupt — depends on: Status Effect, Enemy AI (base), Health/Damage Core
7. Environmental Chain-Destruction — depends on: Destructible Geometry, Status Effect, Health/Damage Core
8. Boss Phase — depends on: Enemy AI (base), Health/Damage Core

### Presentation Layer (depends on features)

1. Arena Morphing — depends on: Boss Phase, Destructible Geometry, Dash/Evasion, Camera System (base)
2. Overdrive Visual State — depends on: Luna Overdrive
3. Execution Camera/Cutscene — depends on: Core Extraction Execution, Camera System (base)
4. Combat HUD — depends on: Combo/Tension Gauge, Luna Overdrive, Core Extraction Execution (미설계 — 가정 인터페이스), Health/Damage Core, Spell Casting (base), Dash/Evasion (뒤 3건은 상류 UI Requirements가 위임한 표시 책임 — 2026-07-17 combat-hud.md 설계 시 추가)

### Polish Layer

- (해당 없음 — 이 확장 범위에서 Polish 레이어 시스템 없음)

---

## Recommended Design Order

| Order | System | Priority | Layer | Agent(s) | Est. Effort |
|-------|--------|----------|-------|----------|-------------|
| 1 | Player Movement | MVP | Foundation | game-designer | S |
| 2 | Health/Damage Core | MVP | Foundation | systems-designer | M |
| 3 | Camera System (base) | MVP | Core | game-designer | S |
| 4 | Enemy AI (base) | MVP | Core | ai-programmer / game-designer | M |
| 5 | Spell Casting (base) | MVP | Core | systems-designer | M |
| 6 | Dash/Evasion | MVP | Core | game-designer | S |
| 7 | Combo/Tension Gauge | MVP | Feature | systems-designer | S |
| 8 | Luna Overdrive (Blood Moon) | MVP | Feature | game-designer | M |
| 9 | Combat HUD | MVP | Presentation | ui-programmer / game-designer | S |
| 10 | Status Effect | Vertical Slice | Core | systems-designer | M |
| 11 | Destructible Geometry | Vertical Slice | Foundation | technical-artist / systems-designer | M |
| 12 | Spell Weaving / Synergy | Vertical Slice | Feature | systems-designer | M |
| 13 | Core Extraction Execution | Vertical Slice | Feature | game-designer | M |
| 14 | Environmental Chain-Destruction | Vertical Slice | Feature | level-designer / systems-designer | L |
| 15 | Overdrive Visual State | Vertical Slice | Presentation | art-director | S |
| 16 | Execution Camera/Cutscene | Vertical Slice | Presentation | game-designer | S |
| 17 | Enemy Elite Shield | Alpha | Feature | ai-programmer / systems-designer | M |
| 18 | Super Armor / CC Interrupt | Alpha | Feature | systems-designer | M |
| 19 | Boss Phase | Alpha | Feature | game-designer / ai-programmer | L |
| 20 | Arena Morphing | Alpha | Presentation | level-designer / systems-designer | L |

---

## Circular Dependencies

- None found.

---

## High-Risk Systems

| System | Risk Type | Risk Description | Mitigation |
|--------|-----------|-------------------|------------|
| Arena Morphing | Technical | 40+ Nanite 프래그먼트 + 5개 상승 플랫폼 + 플레이어 에어대쉬 동시 60fps 예산 미검증 (`prototypes/arena-morphing-spike-2026-07-16` 참고) | Production 진입 전 perf 측정 재실행, 필요시 조각 수/플랫폼 수 스케일 다운 |
| Destructible Geometry | Technical | Nanite per-fragment 지원 확인됨(스파이크 완료)이나 headless bake 트리거 신뢰성 이슈 있었음 | 에디터 인터랙티브 사용 시엔 정상 — 툴체인 자동화 시 asset editor 탭 close 트리거 필요 여부 재확인 |
| Enemy Elite Shield / Core Extraction Execution | Design | 처형 발동조건(저스트회피 or 실드파괴)이 두 시스템에 동시 걸림 — 설계 시점 순서 조율 안하면 조건 충돌 가능 | #12, #13, #17 설계 시 트리거 조건 명세를 Core Extraction Execution GDD에서 단일 소유 |
| Combo/Tension Gauge → Luna Overdrive | Design | 오버드라이브 트리거 임계값이 너무 쉽거나 어려우면 텐션 곡선 전체가 무너짐 (콘셉트의 핵심 목표) | 초기 프로토타입 단계에서 게이지 충전 곡선 튜닝, 플레이테스트로 검증 |

---

## Progress Tracker

| Metric | Count |
|--------|-------|
| Total systems identified | 20 |
| Design docs started | 9 |
| Design docs reviewed | 6 |
| Design docs approved | 6 |
| MVP systems designed | 9/9 (6 approved, 3 Needs Revision — see gdd-cross-review-2026-07-18.md) |
| Vertical Slice systems designed | 0/7 |

---

## Next Steps

- [ ] Review and approve this systems enumeration
- [ ] Design MVP-tier systems first (use `/design-system [system-name]`) — start with Player Movement
- [ ] Run `/design-review` on each completed GDD
- [ ] Run `/gate-check pre-production` when MVP systems are designed
- [ ] Validate the highest-risk systems with `/vertical-slice` before committing to Production
