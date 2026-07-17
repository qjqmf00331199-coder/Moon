# Combo/Tension Gauge

> **Status**: In Design
> **Author**: user + game-designer (solo design mode)
> **Last Updated**: 2026-07-17
> **Implements Pillar**: Dopamine Driven Design — 텐션 곡선의 상승 구간(콤보 누적 → 오버드라이브 폭발점 트리거)

## Overview

Combo/Tension Gauge는 전투 중 플레이어의 공격적 행동(스펠 명중, 저스트회피 처형 태그 부여 등)을 실시간으로 누적하는 단일 수치 게이지이며, 이 값이 임계치에 도달하면 루나 오버드라이브(각성)를 발동시키는 트리거 역할을 한다. 데이터 레이어로는 GAS Attribute(0~Max) 하나와 감쇠(decay) 타이머로 구성되고, 플레이어가 체감하는 층위에서는 화면 게이지가 차오르는 시각적 피드백과 "다음 타격이 얼마나 폭발에 가까운가"라는 실시간 리스크-리턴 판단을 제공한다 — Spell Weaving으로 빠르게 채우고 오버드라이브 직전에 큰 스펠을 아낄지 즉시 쏠지 결정하게 만드는, Dopamine Driven Design 텐션 곡선의 실질적 엔진이다.

## Player Fantasy

게이지가 차오르는 순간, 플레이어는 "지금 큰 거 한 방 쓸까, 조금만 더 채울까"를 매 타격마다 저울질한다. 화면 한쪽에서 빛나는 게이지가 오버드라이브 임계치에 가까워질수록 긴장감이 고조되고, 도달하는 순간 크림슨 레드로 반전되는 폭발적 해방감으로 이어진다. 이 시스템 자체는 능동적으로 "조작"하는 대상이 아니라, 플레이어의 모든 공격적 판단(콤보 유지, 저스트회피, 스펠 위빙)이 쌓여 만드는 리스크-리턴 압력계다 — 게이지를 직접 만지는 게 아니라 게이지를 위해 플레이한다.

**Feel Reference**: 베요네타 마녀시간 게이지, DMC 스타일 랭크 — "차오름을 지켜보는 긴장감" 자체가 재미의 일부.

> **Note**: `creative-director` 미컨설트 — Solo 모드. Production 전 수동 리뷰 필요.

## Detailed Design

### Core Rules

1. **단일 GAS Attribute** — `TensionGauge`, 범위 0 ~ `TensionGaugeMax`(Tuning Knob). 플레이어 직접 조작 불가, 순수 이벤트 반응형.
2. **획득 소스 (2종)**:
   - Spell Casting의 `OnSpellHit(Element, Target)` 구독 — 속성별 획득량은 `TensionGainPerElement` (Formulas 소유).
   - Health/Damage Core의 `State.Executable` 태그가 적에게 부여되는 순간을 GAS `OnTagAdded(State.Executable)` 콜백으로 구독 — Dash/Evasion의 저스트회피 성공을 간접 감지 (dash-evasion.md는 수정하지 않음). 고정 보너스 `JustDodgeTensionBonus`.
3. **감쇠(Decay)** — 마지막 획득 이벤트로부터 `TensionDecayGracePeriod`초 경과 시, 이후 `TensionDecayRatePerSec`으로 지속 감소. 대쉬 무적(`State.Invulnerable`) 중에도 감쇠 타이머는 예외 없이 계속 돈다.
4. **피격 페널티** — Health/Damage Core가 플레이어에게 데미지를 적용하는 이벤트(`OnDamageApplied`, 상류 문서가 "추후" 노출 예정으로 명시한 인터페이스)를 구독, 유효 데미지 1 이상 발생 시 게이지를 즉시 `DamagePenaltyPercent`만큼 비례 감소(가산 아님) — 감쇠와 별도로 즉시 적용.
5. **오버드라이브 트리거** — 게이지가 `TensionGaugeMax`에 도달하는 프레임에 `OnOverdriveTriggered` 이벤트를 1회 발행(Luna Overdrive 소비)하고, 같은 프레임에 게이지를 0으로 즉시 리셋한다.
6. **재진입 방지** — 게이지 0인 상태에서 리셋 직후 프레임에 대량 동시 획득 이벤트(예: 다수의 State.Executable 동시 부여)가 들어와도, 리셋 프레임 내 재차 Max 도달·재발동은 허용(연속 오버드라이브 자체는 막지 않음 — 다만 동일 프레임 중복 발행은 방지, Health/Damage Core의 `OnDeath` 1회 발행 패턴과 동일 원칙 적용).

### States and Transitions

| State | Entry Condition | Exit Condition | Behavior |
|---|---|---|---|
| Building | 획득 이벤트 발생, gauge < Max | 마지막 획득 후 `TensionDecayGracePeriod` 경과 | 게이지 증가, 감쇠 타이머 리셋 |
| Decaying | Building 상태에서 grace period 경과 | 새 획득 이벤트 발생(→Building) 또는 gauge=0 | `TensionDecayRatePerSec`로 지속 감소 |
| Charged→Reset | gauge == Max 도달 | 즉시(같은 프레임) | `OnOverdriveTriggered` 발행 후 gauge=0, Building/Idle로 복귀 |

### Interactions with Other Systems

- **Spell Casting (base)** (상류) — `OnSpellHit(Element, Target)` 구독. 속성별 값은 이 문서가 소유(Formulas).
- **Health/Damage Core** (상류) — `OnTagAdded(State.Executable)` 콜백 구독(Just-Dodge 감지 우회 경로), `OnDamageApplied`(플레이어 대상) 구독(피격 페널티).
- **Dash/Evasion** (간접 상류) — 직접 이벤트 구독 없음. `State.Executable` 태그 부여가 유일한 접점이며 Health/Damage Core의 태그 시스템을 경유해서만 감지.
- **Luna Overdrive** (하류, 미설계) — `OnOverdriveTriggered` 이벤트 구독 예상. "무엇을 각성으로 할지"는 그쪽 GDD 소유, 이 문서는 트리거 발행까지만.
- **Combat HUD** (하류, 미설계) — 게이지 실시간 값(0~Max) 및 Building/Decaying 상태 구독 예상 (시각 피드백).

## Formulas

### Tension Gain (Spell Hit)

`TensionGain = ManaCost[Element] × TensionGainCoefficient`

**Variables:**
| Variable | Symbol | Type | Range | Description |
|----------|--------|------|-------|-------------|
| ManaCost[Element] | — | float | Blackhole=70, Fire=25, Lightning=25 (spell-casting-base.md 소유값 그대로 재사용) | 명중한 스펠의 속성별 마나비용 |
| TensionGainCoefficient | — | float | 0.5–1.5 (기본 1.0) | 마나비용 대비 텐션 환산 계수 |

**Output Range**: Blackhole 명중 시 70, Fire/Lightning 명중 시 25 (계수 1.0 기준). Max 게이지(100) 대비 Blackhole 1회로 70% 충전.
**Example**: Blackhole 명중 → TensionGain=70×1.0=70. 이어서 Fire 위빙 명중 → +25 → 95, 오버드라이브 근접.

### New Tension (명중 시)

`NewTension = clamp(CurrentTension + TensionGain, 0, TensionGaugeMax)`

**Variables:**
| Variable | Symbol | Type | Range | Description |
|----------|--------|------|-------|-------------|
| TensionGaugeMax | — | float | 80–120 (기본 100) | 오버드라이브 발동 임계값 |

**Output Range**: 0–100. 100 도달 시 같은 프레임에 Core Rule 5(오버드라이브 트리거)가 개입해 0으로 리셋.
**Example**: CurrentTension=95, Lightning 명중(+25) → clamp(120,0,100)=100 → 즉시 오버드라이브 발동.

### Just-Dodge Bonus

`TensionGain(JustDodge) = JustDodgeTensionBonus`

**Variables:**
| Variable | Symbol | Type | Range | Description |
|----------|--------|------|-------|-------------|
| JustDodgeTensionBonus | — | float | 15–30 (기본 20) | 저스트회피(State.Executable 부여) 1회당 고정 텐션 획득. 대상 수와 무관하게 대쉬 이벤트 1회당 1회만 적용(dash-evasion.md 차지반환 규칙과 동일 원칙) |

**Output Range**: 고정 20 (대상 다수여도 중복 없음).
**Example**: CurrentTension=60, 저스트회피 성공(3명 동시 Executable 부여) → +20 (1회분만) → 80.

### Tension Decay

`TensionAfterDecay(t) = max(0, CurrentTension - TensionDecayRatePerSec × Δt)` (단, `Δt`는 `TensionDecayGracePeriod` 경과 후 프레임 간격만 누적)

**Variables:**
| Variable | Symbol | Type | Range | Description |
|----------|--------|------|-------|-------------|
| TensionDecayGracePeriod | — | float | 2.0–4.0초 (기본 3.0) | 마지막 획득 이벤트 후 감쇠 시작까지 유예 시간 |
| TensionDecayRatePerSec | — | float | 5–15/초 (기본 10) | 유예 종료 후 초당 감소량 |

**Output Range**: 0 ~ CurrentTension. 완전 방치 시 약 10초 내 100→0.
**Example**: CurrentTension=100, 8초간 무획득(grace 3초 + decay 5초) → 100 - 10×5 = 50.

### Damage Penalty

`TensionAfterDamage = CurrentTension × (1 - DamagePenaltyPercent)`

**Variables:**
| Variable | Symbol | Type | Range | Description |
|----------|--------|------|-------|-------------|
| DamagePenaltyPercent | — | float | 0.10–0.30 (기본 0.20) | 유효 데미지 1회 수신 시 게이지 비례 감소율 |

**Output Range**: CurrentTension의 70~90% (기본 80%)로 즉시 감소.
**Example**: CurrentTension=80, 피격(유효 데미지>0) → 80×0.8=64.

## Edge Cases

- **If Blackhole 명중 + 같은 프레임 Fire 위빙 명중이 겹쳐 Tension이 정확히 Max를 초과**: clamp로 100에서 상한, 초과분은 버림(이월 없음) — New Tension 공식의 clamp가 그대로 처리.
- **If 오버드라이브 지속 중(Luna Overdrive 미설계, 마나 무한/쿨다운 제로) 추가 스펠 명중 발생**: Tension은 이미 0(리셋됨)이므로 정상적으로 재누적 시작 — 오버드라이브 중 재차 Max 도달 시 재발동 허용(Core Rule 6).
- **If 저스트회피 성공과 동시에 플레이어가 피격 판정(다른 적에게)**: Just-Dodge 보너스(+20)와 피격 페널티(×0.8)가 같은 프레임에 겹치면, 획득(가산) 먼저 적용 후 피격(비례감소) 나중 적용 — 순서: Gain → Penalty → Decay 평가. (가산 후 비례감소가 플레이어에게 항상 불리하지 않도록 페널티를 마지막에 적용해 "회피 성공의 보상이 무의미해지지 않게" 보장.)
- **If TensionDecayGracePeriod 도중 새 획득 이벤트 발생**: 감쇠 타이머 완전 리셋(잔여 유예시간 소멸, 처음부터 재시작) — 잦은 소량 획득만으로도 감쇠를 영구 회피 가능(의도된 이지어세스 방지책은 Tuning Knobs에서 다룸).
- **If 플레이어 사망(Health/Damage Core Death 상태)**: Tension은 즉시 0으로 강제 리셋 — 재시작 후 이전 게이지 이월 없음(Health/Damage Core Rule 6, "즉시 재시작"과 일관).
- **If 게이지 0에서 피격 페널티 이벤트만 단독 발생**: `0 × 0.8 = 0` — 별도 분기 불필요, 공식이 자연히 처리(음수 방지 clamp 불필요).

## Dependencies

| System | Direction | Interface |
|---|---|---|
| Spell Casting (base) | 상류 (하드) | `OnSpellHit(Element, Target)` 구독 — ManaCost[Element] 값 재사용 |
| Health/Damage Core | 상류 (하드) | `OnTagAdded(State.Executable)` 구독(Just-Dodge 간접 감지), `OnDamageApplied`(플레이어向) 구독(피격 페널티), 플레이어 Death 상태 구독(즉시 리셋) |
| Dash/Evasion | 상류 (소프트, 간접) | 직접 이벤트 없음 — Health/Damage Core의 `State.Executable` 태그 경유로만 연결 |
| Luna Overdrive (미설계) | 하류 (하드) | `OnOverdriveTriggered` 이벤트 발행 — 소비측이 각성 로직 소유 |
| Combat HUD (미설계) | 하류 (소프트) | 게이지 값(0~Max) + Building/Decaying 상태 read-only 구독 — 없어도 게이지 로직 자체는 정상 동작 |

## Tuning Knobs

| Knob | Default | Safe Range | Too High | Too Low |
|---|---|---|---|---|
| TensionGaugeMax | 100 | 80–120 | 오버드라이브 너무 늦게 발동 → 텐션곡선 늘어짐 | 스펠 1~2방으로 즉시 발동 → 각성이 희소성 상실 |
| TensionGainCoefficient | 1.0 | 0.5–1.5 | 즉시발동 남발(위 항목과 동일 위험) | 위빙해도 안 채워짐 → 무기력 |
| JustDodgeTensionBonus | 20 | 15–30 | 회피만으로 게이지 채우기 최적 전략화 → 스펠 위빙 무의미해짐 | 저스트회피 리스크 대비 보상 부족 → 안 씀 |
| TensionDecayGracePeriod | 3.0초 | 2.0–4.0초 | 사실상 감쇠 없음(항상 유예 중) → 방치 플레이 허용 | 공격 텀만 살짝 벌어져도 즉시 감쇠 시작 → 답답함 |
| TensionDecayRatePerSec | 10 | 5–15 | 잠깐 쉬면 게이지 증발 → 좌절감 | 감쇠 사실상 무의미 → 방치해도 유지됨 |
| DamagePenaltyPercent | 0.20 | 0.10–0.30 | 피격 한 번에 게이지 거의 증발 → 과응징 | 피격해도 손해 없음 → 리스크-리턴 설계 의미 상실 |

**교차 제약**: `TensionGaugeMax`를 올리면 `TensionGainCoefficient`도 같이 올리지 않는 한 오버드라이브 도달 시간이 늘어짐 — 이 둘은 항상 같이 재조정할 것 (High-Risk 표의 "Combo/Tension Gauge → Luna Overdrive" 리스크와 동일 계열, 플레이테스트로 검증 필요).

## Visual/Audio Requirements

> **Note**: `art-director` 미컨설트 — Solo 모드. Production 전 수동 리뷰 필요.

- 게이지 값(0~100%)에 실시간으로 채워지는 화면 요소(Combat HUD 소유 위젯 — 이 문서는 "값과 상태를 노출한다"까지만).
- Charged(임계값 근접, 예: 90%+) 구간 진입 시 시각적 강조(펄스/글로우 강도 증가) — 정확한 임계값 표시 로직은 Combat HUD GDD 소유.
- 오버드라이브 트리거(`OnOverdriveTriggered`) 순간 짧은 화면 전역 임팩트(플래시) — 이후 크림슨 레드 전환 자체는 Luna Overdrive 소유, 이 시스템은 트리거 발행 순간의 즉각 피드백만 담당.
- 오디오: 게이지 Building 중 낮은 피치의 지속음이 점점 상승, Max 도달 순간 짧은 스팅어 — 구체 SFX 스펙은 `sound-designer` 영역.

> 📌 **Asset Spec** — 위 항목은 Art Bible 승인 후 `/asset-spec system:combo-tension-gauge`로 구체 스펙화 필요.

## UI Requirements

- 게이지 바(0~100%) — Combat HUD 상시 노출 요소 중 하나로 배치될 것으로 예상(레이아웃/배치 결정은 Combat HUD·ux-designer 영역).
- Building/Decaying 상태 구분 시각 표시(예: 감쇠 중엔 바 색이 살짝 어두워짐) — 플레이어가 "지금 게이지가 새고 있다"를 인지해야 리스크 판단이 성립.
- 패드 접근성: 별도 입력 불필요(순수 읽기 전용 표시), 패드 내비게이션 영향 없음.

> **📌 UX Flag — Combo/Tension Gauge**: 이 시스템은 UI 요구사항 있음. Pre-Production 단계에서 `/ux-design`으로 Combat HUD 화면 스펙 작성 필요 — 스토리는 이 GDD가 아니라 `design/ux/combat-hud.md`를 인용해야 함.

## Acceptance Criteria

1. **GIVEN** TensionGauge=0, **WHEN** Blackhole 스펠이 적에게 명중(`OnSpellHit`), **THEN** TensionGauge=70(ManaCost 70 × 계수 1.0).
2. **GIVEN** TensionGauge=70, **WHEN** 같은 교전 중 Fire 스펠이 이어서 명중, **THEN** TensionGauge=95(70+25), 오버드라이브 미발동.
3. **GIVEN** TensionGauge=95, **WHEN** Lightning 스펠 명중(+25), **THEN** clamp에 의해 TensionGauge=100 도달 즉시 `OnOverdriveTriggered` 1회 발행, 같은 프레임에 TensionGauge=0으로 리셋.
4. **GIVEN** TensionGauge=60, **WHEN** 저스트회피 성공(대상 3명 동시 `State.Executable` 부여), **THEN** TensionGauge=80 (+20, 중복 없이 1회만 적용).
5. **GIVEN** TensionGauge=100, 마지막 획득 이벤트 후 8초 경과(획득 없음), **WHEN** 8초 후 값 확인, **THEN** TensionGauge=50 (grace 3초 제외 5초×10/초 감쇠).
6. **GIVEN** TensionGauge=80, **WHEN** 플레이어가 유효 데미지(무적 아님) 수신, **THEN** TensionGauge=64 (80×0.8).
7. **GIVEN** TensionGauge=0, **WHEN** 플레이어가 유효 데미지 수신, **THEN** TensionGauge는 0 유지(음수 없음).
8. **GIVEN** 플레이어 Death 상태 진입, **WHEN** 재시작, **THEN** TensionGauge=0으로 강제 리셋, 이전 값 이월 없음.
9. **GIVEN** 저스트회피 성공과 동일 프레임에 다른 적으로부터 피격, **WHEN** 두 이벤트 처리, **THEN** Gain(+20) 먼저 적용 후 Penalty(×0.8) 적용 순서 준수(Edge Cases 순서 계약과 일치).

## Open Questions

- ~~**Owner: game-designer** — Luna Overdrive GDD 설계 시 `OnOverdriveTriggered` 페이로드에 무엇이 필요한지 확정 필요(트리거 위치/시각 등 추가 데이터 필요 여부). Target: Luna Overdrive 설계 착수 시.~~
  **RESOLVED (2026-07-17, Luna Overdrive 설계 세션)**: 페이로드 **없음(무인자 이벤트)**으로 확정 — 각성 효과가 전부 플레이어 중심(태그 부여 + 타이머)이고 위치/시점 정보는 수신 시점에 플레이어 참조로 조회 가능. luna-overdrive.md Core Rule 1 참조. (본 문서 본문 변경 없음 — Core Rule 5의 "이벤트 1회 발행" 계약이 무인자 발행과 그대로 호환.)
- **Owner: qa-lead** — 위 Acceptance Criteria는 solo 모드로 작성, qa-lead 미검증. Target: Production 진입 전 리뷰.
- **Owner: game-designer** — TensionGaugeMax=100/계수=1.0 조합의 실제 체감(스펠 위빙 1~2회로 각성 도달하는 속도감)은 플레이테스트 전까지 가설 단계. Target: 프로토타입 완료 후.
