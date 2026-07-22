# Spell Casting (base)

> **Status**: Approved (2026-07-23 re-review: mana-regen pause / temporal-bypass sync verified consistent against luna-overdrive.md; Combat HUD dependency row added)
> **Author**: user + Claude Code (full review mode) + 2026-07-23 Claude Code re-review
> **Last Updated**: 2026-07-23
> **Implements Pillar**: Dopamine Driven Design — foundation for Spell Weaving synergy, Luna Overdrive unlimited-mana window, Core Extraction mana-refund loop

## Overview

Spell Casting (base)는 플레이어가 발동하는 모든 마법 스킬(투사체/광역/유틸리티)의 공용 실행 기반이다. 데이터/인프라 관점에서 GAS(`AbilitySystemComponent` + Mana `AttributeSet`)를 통해 스펠별 `GameplayAbility` 인스턴스를 관리하며, 코스트(마나 소모)·쿨다운·캐스트 판정을 `GameplayEffect`/GAS 태그로 처리하는 단일 캐스팅 파이프라인 역할을 한다. 이 MVP 범위는 개별 스펠의 밸런스나 시너지(초신성, 플라즈마 스톰 등 — Spell Weaving GDD 소유)를 정의하지 않고, "스펠이 어떻게 발동되고, 마나/쿨다운이 어떻게 소모·회복되며, 데미지가 어떻게 Health/Damage Core로 전달되는가"라는 공용 실행 계약만 소유한다.

Player Movement가 이동의 뼈대, Health/Damage Core가 피격/사망의 뼈대였다면, 이 시스템은 "플레이어가 스펠을 쏘는 모든 순간"의 뼈대다 — Spell Weaving 시너지, Combo/Tension Gauge 누적, 루나 오버드라이브의 무한마나/쿨타임 제로, 코어 적출의 마나 100% 회복까지, 이후 모든 도파민 순간이 이 파이프라인 위에서 발동된다.

## Player Fantasy

플레이어는 스펠을 '준비하는 행위'가 아니라 '즉시 터트리는 무기'처럼 다뤄야 한다 — 캐스팅 입력과 동시에(또는 최소 지연으로) 효과가 발동되고, 이동을 막지 않으며(Player Movement Core Rule 7 계약), 콤보 중간에 자유롭게 스펠→스펠, 스펠→이동으로 전환할 수 있어야 한다. 마나가 바닥나기 전까지는 "다음 스펠을 못 쓸까봐 아끼는" 게 아니라 "얼마나 몰아칠 수 있는가"를 계산하며 공격적으로 소모하는 리듬이 목표다. 이 판타지는 두 축으로 분리해 계약한다 — **(a) 실행 지연(latency)**: 입력→발동은 항상 즉각(Rule 2/3/4가 보장, 절대 타협 없음), **(b) 가용성(availability)**: 마나/쿨다운은 "지금 쏠 수 있는가"를 결정하는 리듬 게이트이며 페이싱 도구다. Souls류가 지향 안 하는 건 (a)의 지연이지 (b)의 자원 관리가 아니다 — 마나 고갈은 실패가 아니라 "다음 몰아치기 준비" 박자로 읽혀야 하고, 최소한 블랙홀 전량 투입 직후에도 위빙 1회(Fire/Lightning)는 항상 보장된다(Tuning Knobs의 위빙 보장 교차 제약 참조). 루나 오버드라이브 진입 시 마나 무한·쿨타임 제로가 되는 순간, 그 직전까지 쌓인 "아껴야 했던" 긴장이 완전히 풀리며 화면을 가득 채우는 학살로 폭발해야 한다(game-concept.md 텐션 곡선과 직결).

**Feel Reference**: *Devil May Cry 5*의 즉각적인 스킬 캔슬/콤보 연계(선입력 없이 인풋 즉시 반영) + *Vanquish*의 몰아치는 화력 발산 리듬.
**Anti-reference**: Souls류/Elden Ring 소서리의 캐스팅 중 이동 불가·긴 후딜레이는 지향 안함 — Player Movement Core Rule 7과 정면 충돌, Endless Catharsis 목표와도 상충.

> 2026-07-17 full design-review에서 `creative-director` 상담 완료 — latency/availability 분리 계약 및 Blackhole 코스트 하향은 해당 리뷰 반영분.

## Detailed Design

### Core Rules

1. **MVP는 3속성 고정 슬롯**: 블랙홀(Radial Force)/화염/번개 — 각각 독립 입력 바인딩, 로드아웃 교체 없음. 속성 식별은 `Spell.Element.Blackhole`/`Spell.Element.Fire`/`Spell.Element.Lightning` GameplayTag로 노출(Enemy AI의 `Enemy.Archetype.*` 패턴과 동일) — Spell Weaving GDD(미설계)가 이 태그로 시너지 조합(초신성/플라즈마 스톰)을 감지한다.
2. **즉시발동, 위액/차지 없음**: 입력 프레임에 코스트/쿨다운 검사를 통과하면 그 프레임에 `GameplayAbility`가 즉시 활성화·이펙트 적용된다. 캐스팅 애니메이션은 순수 프레젠테이션이며 (Player Movement Core Rule 8/9와 동일 철학) 판정을 지연시키지 않는다.
3. **이동을 절대 막지 않는다**: 캐스팅은 `MovementLocked`를 사용하지 않으며, `AddMovementInput` 처리는 캐스팅 중에도 100% 정상 틱 — Player Movement Core Rule 7 계약을 이 문서가 마법 쪽에서 대칭적으로 준수(양쪽 문서 모두 계약 소유, 어느 한쪽이 위반 못하도록 고정).
4. **캐스팅은 자유롭게 캔슬/전환 가능**: 진행 중인 스펠 프레젠테이션(애니메이션) 도중에도 새 입력(다른 속성 스펠 또는 이동)이 들어오면 즉시 처리 — 강제 recovery 프레임 없음. 콤보 위빙(스펠→스펠, 스펠→이동)의 전제 조건. **GAS 구현 계약**: Rule 2(즉시발동)에 의해 `GameplayAbility`의 활성화→커밋→이펙트 적용→`EndAbility`가 전부 동일 프레임에 완결된다(Instancing Policy: `InstancedPerActor`). 따라서 "캔슬"은 GAS 레벨에 존재하지 않는다 — `CancelAbility`/`CancelAbilitiesWithTag`/`BlockAbilitiesWithTag` 불사용. 캔슬되는 것은 순수 프레젠테이션(애니메이션 몽타주 블렌드 아웃)뿐이며, 새 캐스트/이동 입력은 이미 종료된 어빌리티와 무관하게 정상 게이트 체크를 거쳐 처리된다.
5. **마나는 단일 공용 자원**: `AttributeSet`의 Mana/MaxMana. 3개 스펠 모두 이 자원을 소모하며, 스펠별 `ManaCost`만 상이(Tuning Knobs 참조).
6. **쿨다운은 스펠별 개별 적용**: 3개 속성 각각 독립 쿨다운 타이머(GAS Cooldown `GameplayEffect` + 태그). 공유/글로벌 쿨다운 없음 — 서로 다른 속성을 자유롭게 섞어 쓸 수 있어야 위빙이 성립.
7. **마나 회복은 패시브 + 코어 적출 보너스**: 시간경과 패시브 회복(Tuning Knob `ManaRegenRate`)이 기본 소스이며, Luna Overdrive Active 동안에만 정지한다. 무료 캐스트 창이 종료 자원을 자동 만충시키지 않게 하기 위한 예외다. Recovery 진입 즉시 패시브 회복을 재개한다. 추가로 Health/Damage Core의 `OnExecuted` 이벤트(코어 적출 성공)를 구독해 Mana를 즉시 MaxMana로 100% 회복시킨다.
8. **모든 스펠 데미지는 단일 진입점을 통과**: Health/Damage Core Rule 1(`ApplyDamage`/GameplayEffect)을 그대로 사용 — 별도 데미지 경로 없음. 기본 3속성 스펠은 `bBypassDefense=false`가 기본값(일반 데미지) — 방어무시가 필요한 시너지 콤보(초신성 등)는 Spell Weaving GDD가 자신의 GameplayEffect에서 `bBypassDefense=true`로 오버라이드한다(이 문서는 그 오버라이드를 막지 않는다는 계약만 소유).
9. **스펠 피격/충돌 시 소음 발생 의무**: 모든 스펠의 임팩트(투사체 명중, 광역 발동 등)는 `MakeNoise`를 호출해야 한다(Enemy AI (base)의 청각 인지 계약, Rule 2 참조). 소음 크기(`NoiseLoudness`, 0.0–1.0)는 스펠별로 이 문서가 정의(Formulas 참조) — 큰 폭발형 스펠(블랙홀)이 작은 스펠보다 더 멀리 어그로를 끈다.
10. **오버드라이브 코스트·쿨다운 우회 훅 노출**: `bManaCostBypassed`/`bCooldownBypassed`는 Luna Overdrive Active 동안만 true다. 판정은 `CostBypass.Active` 태그 **AND** 캐릭터의 `CurrentTime < OverdriveEndTime`을 두 지점(`CanActivateAbility`, `CommitAbility`)에서 동일하게 확인한다. 태그만 존재하는 만료 프레임에는 우회하지 않는다. 표준 `CommitAbility()`는 Cost+Cooldown GE를 원자적으로 적용하므로, 유효한 Active 상태에서만 커밋을 건너뛴다.
11. **캐스트 레이트 바닥 (per-element per-frame cap)**: 동일 속성의 캐스트는 **프레임당 최대 1회**로 제한한다. 이 제한은 코스트/쿨다운 우회 여부와 무관하게 항상 적용된다. 추가로 모든 속성을 합산한 **전역 초당 캐스트 상한**(`MaxCastsPerSecond`, Tuning Knob — 기본값 20)을 두어, 터보/매크로 입력이 프레임 레이트에 비례하는 출력을 생산하지 못하게 한다. **설계 근거**: `CostBypass.Active`(Luna Overdrive)가 코스트·쿨다운을 동시에 제거하면, 입력 속도가 유일한 제약이 되어 frame-instant 캐스트(Rule 2) + recovery 없음(Rule 4) 조합이 터보/매크로에 의한 입력률 종속 출력을 허용한다. 이 캡은 그 익스플로잇을 구조적으로 차단하되, 정상적인 손가락 입력(인간의 실측 연타 약 10–12 CPS)에는 걸리지 않는 느슨한 값을 기본으로 한다(2026-07-17 luna-overdrive.md design-review, creative-director 판정 — 캡 소유권은 이 문서).

### States and Transitions

| State/Tag | Entry Condition | Exit Condition | Behavior |
|-----------|-----------------|-----------------|----------|
| Ready (per-element) | 기본 상태 / 쿨다운 만료 | 해당 속성 캐스트 성공 | 캐스트 입력 시 코스트/쿨다운 검사 통과하면 즉시 활성화 |
| OnCooldown (per-element) | 해당 속성 캐스트 성공 직후 | 쿨다운 타이머 만료 | 해당 속성 재캐스트 차단(다른 속성은 영향 없음 — Rule 6) |
| InsufficientMana (게이트, 상태 아님) | 캐스트 시도 시 Mana < ManaCost | 즉시(같은 프레임 재평가) | 캐스트 실패, 사이드이펙트 없음(코스트 차감/쿨다운 진입 없음) |
| `CostBypass.Active` (오버레이, Luna Overdrive 전용) | Luna Overdrive가 플래그 설정 | Luna Overdrive가 플래그 해제 | Ready/OnCooldown/InsufficientMana 판정 자체를 우회 — 모든 속성 즉시 캐스트 가능 |

### Interactions with Other Systems

- **Player Movement** (상류): Core Rule 7 계약(이동이 캐스팅을 막지 않음)을 대칭적으로 준수 — 이 문서는 이동 상태/타입을 컴파일 타임에 참조하지 않는다.
- **Health/Damage Core** (상류): 모든 스펠 데미지는 Rule 1 단일 진입점 사용(Core Rule 8). `OnExecuted` 이벤트를 구독해 마나 100% 회복 트리거(Core Rule 7, Health/Damage Core Rule 9가 위임한 지점).
- **Enemy AI (base)** (하류 데이터 흐름, 인덱스상 정식 의존 아님): 스펠 임팩트가 `MakeNoise` 호출 — Enemy AI의 청각 인지(`noise_detection_radius` 공식)가 이 소음을 소비.
- **Dash/Evasion** (하류, 미설계): 캐스팅이 절대 이동/대쉬 입력을 막지 않는다는 계약(Core Rule 3)에 의존할 것으로 예상 — 대쉬캔슬 스펠 위빙 전제.
- **Combo/Tension Gauge** (하류): `OnSpellCast(Element)` / `OnSpellHit(Element, Target)` 이벤트를 구독해 콤보 누적에 활용할 것으로 예상 — "무엇이 콤보 가치가 있는가"는 해당 GDD 소유, 이 문서는 이벤트 발행까지만.
- **Combat HUD** (하류, 소프트): Mana/MaxMana 값과 변경 델리게이트, 3속성 쿨다운 상태, `CostBypass.Active` 태그 존재 여부를 read-only 구독(UI Requirements 참조) — 없어도 캐스팅 로직 자체는 정상 동작.
- **Spell Weaving / Synergy** (하류, 미설계): 속성 GameplayTag + 히트 위치/타임스탬프 데이터를 소비해 시너지 판정(초신성/플라즈마 스톰)을 수행할 것으로 예상 — 시너지 판정 로직과 방어무시 오버라이드는 해당 GDD 소유.
- **Luna Overdrive** (하류): Core Rule 10의 코스트/쿨다운 우회 훅을 소비 — 지속시간(10초)과 발동 조건은 해당 GDD 소유.

> 2026-07-17 full design-review에서 `systems-designer`/`ue-gas-specialist` 상담 완료 — Rule 4/10의 GAS 구현 계약은 해당 리뷰 반영분.

## Formulas

### Effective Mana Cost

```
EffectiveManaCost = bCostBypassed ? 0 : ManaCost[Element]
```

| Variable | Symbol | Type | Range | Description |
|----------|--------|------|-------|--------------|
| ManaCost[Element] | — | float | Blackhole=70, Fire=25, Lightning=25 | 속성별 고정 마나비용(Tuning Knob) — Blackhole은 2026-07-17 리뷰에서 100→70 하향(위빙 1회 보장 교차 제약, Tuning Knobs 참조) |
| bCostBypassed | — | bool | `CostBypass.Active` 태그 존재 여부 | Luna Overdrive가 설정(Core Rule 10) |

**Output Range**: 0 또는 ManaCost[Element] 중 하나 — 부분비용 없음.
**Example**: Blackhole 캐스트, bCostBypassed=false → EffectiveManaCost=70(잔여 30 — Fire/Lightning 1회 위빙 가능). Overdrive 중이면 0.

### Cast Gate Check

```
CanCast = bCostBypassed ? true : (Mana >= ManaCost[Element])
        AND (bCooldownBypassed ? true : NOT OnCooldown[Element])
```

| Variable | Symbol | Type | Range | Description |
|----------|--------|------|-------|--------------|
| Mana | — | float | 0–MaxMana(100) | 현재 마나 |
| OnCooldown[Element] | — | bool | 해당 속성 쿨다운 타이머 활성 여부 | Core Rule 6 |
| bCooldownBypassed | — | bool | `CostBypass.Active` 태그 존재 여부 | Luna Overdrive가 설정 |

> **표기 주의**: `bCostBypassed`/`bCooldownBypassed`는 공식상 독립 변수로 표기되지만 MVP에서는 **단일 태그 `CostBypass.Active` 하나가 둘 다 결정**한다(Core Rule 10, Edge Cases의 "부분 우회 발생 안 함" 참조). 독립 표기는 Luna Overdrive GDD가 향후 태그를 분리할 경우를 대비한 논리적 구분일 뿐, MVP 구현에서 두 변수가 다른 값을 갖는 경로는 존재하지 않는다.

**Output Range**: boolean.
**Example**: Mana=60, Blackhole ManaCost=70, OnCooldown=false, bCostBypassed=false → CanCast=false(마나 부족). Overdrive 중이면 마나/쿨다운 무관하게 true.

### New Mana (캐스트 후)

```
NewMana = clamp(CurrentMana - EffectiveManaCost, 0, MaxMana)
```

| Variable | Type | Range | Description |
|----------|------|-------|-------------|
| CurrentMana | float | 0–MaxMana | 캐스트 전 마나 |
| EffectiveManaCost | float | 위 공식 결과 | — |
| MaxMana | float | Tuning Knob, 기본 100 | — |

**Output Range**: 0–100.
**Example**: CurrentMana=100, Blackhole 캐스트 → NewMana=30(6초 쿨다운 시작, Fire/Lightning 1회 즉시 위빙 가능 — 초신성/플라즈마 스톰 진입 경로 보장).

> **선행 게이트 의존성 명시**: 이 공식의 clamp 하한(0)은 수치 안전망일 뿐이다. `EffectiveManaCost > CurrentMana`인 채 이 공식이 호출되면 "정가보다 싸게 캐스트"가 성립해버리므로, 구현은 반드시 Cast Gate Check 통과 후에만 이 공식을 적용해야 한다(GAS에서는 `CanActivateAbility` 통과 → `CommitAbility` 순서가 이를 구조적으로 보장). 게이트 우회 호출은 설계 위반.

### Mana Regen Tick (패시브)

```
NewMana_PerTick = OverdriveActive ? CurrentMana : clamp(CurrentMana + ManaRegenRate * DeltaTime, 0, MaxMana)
```

| Variable | Type | Range | Description |
|----------|------|-------|-------------|
| ManaRegenRate | float (mana/s) | Tuning Knob, 기본 8 | 초당 패시브 회복량 |
| DeltaTime | float (s) | 프레임 델타 | 엔진 틱 |
| OverdriveActive | bool | 런타임 | true면 패시브 회복 정지; Recovery부터 false |

**Output Range**: 0–100. **Example**: Blackhole 캐스트 직후(Mana=30), 재캐스트에 필요한 70까지 40/8=5.0초 — 쿨다운(6.0초)이 마나 회복보다 근소하게 길어 **쿨다운이 Blackhole 재사용의 실질 제약**이 된다(Tuning Knobs의 "쿨다운 무의미화" 우려가 기본값에서 성립하지 않음을 확인하는 관계). 완전 고갈(Mana=0)에서 만충까지는 12.5초.

### Core Extraction 마나 보너스 (`OnExecuted` 구독)

```
Mana_OnExecuted = MaxMana
```

Health/Damage Core Rule 9의 `OnExecuted` 이벤트 수신 시 Mana를 즉시 MaxMana로 스냅(회복 공식 아님, 즉시 설정) — Core Rule 7.

**GAS 구현 계약(적용 순서)**: 스냅은 Instant GameplayEffect의 `Override` ModOp, 패시브 리젠은 Infinite/Periodic GE의 `Add` ModOp으로 적용한다. 같은 프레임에 둘이 경합해도 두 경로 모두 MaxMana에서 클램프되므로 최종값은 순서와 무관하게 MaxMana로 동일(benign race — 순서 보장 로직 불필요). 단, 구현은 이 클램프 불변식(`PostGameplayEffectExecute`에서 Mana를 0–MaxMana로 클램프)을 반드시 유지해야 이 무순서 안전성이 성립한다.

### Spell Impact Noise Loudness

```
NoiseLoudness[Element] = Blackhole:1.0, Fire:0.6, Lightning:0.7
```

Enemy AI (base)의 `noise_detection_radius` 공식(`BaseHearingRadius * NoiseLoudness`)에 그대로 대입되는 입력값 — 이 문서는 값만 정의, 반경 계산은 Enemy AI 소유.
**서수 제약(튜닝 시 유지)**: `Blackhole ≥ Lightning ≥ Fire` — "큰 폭발형 스펠이 더 멀리 어그로"라는 Rule 9의 의도를 값 재조정 후에도 보존하기 위한 순서 불변식.
**Example**: BaseHearingRadius=800 가정 시, Blackhole 임팩트 → 감지반경 800uu. Fire → 480uu.

> 2026-07-17 full design-review에서 `systems-designer` 경계값 검증 완료(교차 제약 3건 도출). `economy-designer` 미상담 — 밸런스 수치는 플레이테스트로 최종 검증(Open Questions).

## Edge Cases

| Scenario | Expected Behavior | Rationale |
|----------|-------------------|-----------|
| Mana == ManaCost[Element] 정확히 일치 (예: Mana=70, Blackhole ManaCost=70) | CanCast=true, NewMana=0 | 경계값 포함 판정(`>=`) — Cast Gate Check 공식 그대로 |
| Mana == 0에서 임의 속성 캐스트 시도 | InsufficientMana 게이트에서 실패 — 사이드이펙트 없음. 0은 특수 상태가 아니라 `Mana >= ManaCost` 판정의 일반 케이스 | 별도 "마나 고갈 상태머신" 없음 — 게이트 공식 하나로 전 구간 처리, 상태 분기 최소화 |
| 3속성 모두 OnCooldown 중에 캐스트 입력 | 각 속성 개별 게이트에서 실패 — 전역 락 상태 아님, 이동/대쉬 등 비스펠 행동은 전혀 영향 없음 | Rule 6(속성별 개별 쿨다운)의 자연 귀결 — "전 스펠 봉인"이라는 별도 상태가 존재하지 않음을 명시 |
| `CostBypass.Active` 해제와 캐스트 입력이 같은 프레임에 겹침 | 프레임 내 처리 순서 고정: **태그 상태 변경(부여/해제)을 먼저 반영한 후** 그 프레임의 캐스트 게이트를 평가한다 — 해제 프레임의 캐스트는 정상 코스트/쿨다운 검사를 받음 | 우회 종료 경계에서 "공짜 마지막 한 발"이 프레임 타이밍 복불복이 되면 안 됨 — 결정론적 순서 하나를 계약으로 고정(Luna Overdrive GDD도 이 순서를 전제해야 함) |
| 마나 충분하지만 해당 속성이 OnCooldown 중 | 캐스트 실패, 사이드이펙트 없음(마나 차감·쿨다운 재시작 없음) | Health/Damage Core의 `TryExecute` 실패 시 무효과 패턴과 동일 — 판정 불확실성 없음 |
| Luna Overdrive 종료(`CostBypass.Active` 해제) 시점에 오버드라이브 도중 캐스트한 스펠들의 쿨다운 상태 | 오버드라이브 중 캐스트는 실제 쿨다운 타이머를 전혀 시작시키지 않음 — 종료 즉시 3속성 모두 Ready 상태 | 코스트/쿨다운 우회는 게이트 체크 자체를 건너뛰므로(Cast Gate Check 공식), 우회 중 발생한 쿨다운 진입 자체가 없음. 종료 후 "숨겨진 쿨다운"이 남아 판타지를 깨는 것을 방지 |
| Luna Overdrive Active 중 패시브 Mana Regen | Regen을 적용하지 않아 진입 시 Mana를 보존. Recovery 진입 즉시 정상 Regen 재개 | 무료 난사와 자동 만충이 겹쳐 하강 구간이 사라지는 문제를 방지 |
| 코스트 우회와 쿨다운 우회가 서로 다른 시점에 개별 활성화되는 경우(부분 우회) | MVP 범위에서 발생하지 않음 — `CostBypass.Active` 단일 태그가 코스트·쿨다운을 동시에 우회(Core Rule 10). 부분 우회가 필요해지면 Luna Overdrive GDD가 태그를 분리하는 리비전 대상 | 이 문서는 단일 태그 계약만 소유 — 분리 우회는 미정의 상태로 남기지 않고 "발생 안 함"을 명시 |
| Health/Damage Core의 `OnExecuted` 이벤트가 이미 Mana=MaxMana인 상태에서 발행됨 | Mana_OnExecuted 공식 재적용(MaxMana로 스냅) — 사실상 no-op, 초과 회복이나 에러 없음 | 멱등적(idempotent) 동작 — 이벤트가 몇 번 오든 결과가 동일해야 예측 가능 |
| 같은 프레임에 서로 다른 두 속성(예: 블랙홀+화염) 캐스트 입력이 동시에 들어옴 | 두 캐스트를 완전히 독립적으로 처리 — 각자의 마나/쿨다운 게이트만 검사, 합산 비용/공유 쿨다운 없음(단, 전역 MaxCastsPerSecond 예산에서 2회분 소비 — Rule 11) | Core Rule 6(속성별 개별 쿨다운) 계약 — 시너지(Spell Weaving)를 노리는 동시입력이 여기서 인위적으로 막히면 안 됨 |
| 착지 히트스탑(Player Movement, 40ms) 또는 처형 연출(Core Extraction Execution) 중 캐스트 입력 | 100% 정상 처리됨 — 이 문서는 어떤 프레젠테이션 레이어의 히트스탑/카메라컷도 캐스팅 판정을 지연시키는 근거로 삼지 않음 | Player Movement Core Rule 9/11과 동일 철학 — 프레젠테이션과 게임플레이 판정 분리 |
| 스펠 임팩트가 유효하지 않은 대상(이미 사망한 적, `OnDeath` 처리된 대상)에 도달 | Health/Damage Core Edge Case("Death 상태 대상은 이후 데미지 완전 무시")를 그대로 상속 — 이 문서가 별도 처리 안 함 | 단일 데미지 진입점(Rule 8)을 그대로 신뢰 — 중복 처리 로직을 이 문서가 재구현하지 않음 |
| 오버드라이브 중 터보/매크로 장치로 프레임마다 동일 속성 캐스트 입력 연사 | 동일 속성은 프레임당 1회로 제한(Rule 11) — 초과 입력은 무시(마나 차감·이펙트 발동 없음). 전역 MaxCastsPerSecond(기본 20) 초과 시에도 해당 프레임의 캐스트가 거부됨 | 코스트/쿨다운 우회(Rule 10)와 즉시발동(Rule 2)·무강제회복(Rule 4) 조합이 입력률 종속 출력을 만드는 익스플로잇 차단 — 정상 인간 연타(~10–12 CPS)에는 걸리지 않는 느슨한 값(2026-07-17 luna-overdrive.md design-review, creative-director 판정) |

> 2026-07-17 full design-review에서 `systems-designer`/`qa-lead` 상담 완료 — Mana=0/전속성 쿨다운/우회 해제 프레임 레이스 3건은 해당 리뷰에서 추가.
> 2026-07-17 luna-overdrive.md design-review에서 터보/매크로 익스플로잇 edge case 추가 — cast-rate floor (Rule 11) BLOCKING cross-dep 해소.

## Dependencies

| System | Direction | Nature of Dependency |
|--------|-----------|----------------------|
| Player Movement | 상류 (이 시스템이 의존) | Core Rule 7 계약(이동이 캐스팅을 막지 않음) — 이동 상태를 컴파일타임에 참조하지 않음 |
| Health/Damage Core | 상류 (이 시스템이 의존) | Rule 1 단일 데미지 진입점 사용, `OnExecuted` 이벤트 구독(마나 회복) |
| Enemy AI (base) | 데이터 흐름(정식 의존 아님, systems-index 일치) | `MakeNoise` 호출 — 소음 소비는 Enemy AI 소유, 이 문서는 발신만 |
| Combo/Tension Gauge | 하류 (이 시스템에 의존) | `OnSpellCast`/`OnSpellHit` 이벤트 구독 예정 |
| Combat HUD | 하류 (이 시스템에 의존, 소프트) | Mana/MaxMana, 쿨다운 상태, `CostBypass.Active` read-only 구독 |
| Spell Weaving / Synergy (미설계) | 하류 (이 시스템에 의존) | 속성 GameplayTag + 히트 데이터 소비, 시너지 판정 시 `bBypassDefense` 오버라이드 |
| Luna Overdrive | 하류 (이 시스템에 의존) | `CostBypass.Active` 태그를 통한 코스트/쿨다운 우회 훅 소비 |

## Tuning Knobs

| Parameter | Current Value | Safe Range | Effect of Increase | Effect of Decrease |
|-----------|---------------|------------|--------------------|--------------------|
| MaxMana | 100 | 80–150 | 몰아치기 여유↑, 위기감 희석 | 스펠 남발 불가, 위빙 빈도↓ |
| ManaCost (Blackhole) | 70 | 50–75 (**하드 클램프 최소 50** — 그 아래는 "시그니처 스펠" 정체성 붕괴, 남발 가능해짐. 2026-07-17 리뷰에서 100→70 하향: 코스트=MaxMana면 캐스트 직후 전 스펠 3.1초 락아웃 — Rule 4 "강제 recovery 없음" 및 초신성 위빙 진입 자체를 봉인해 필라와 정면 충돌) | 더 신중하게 써야 함(하이리스크↑) — 75 초과 시 위빙 보장 제약 위반 | 잔여 마나 여유↑, "대형 투자" 판타지 약화 |
| ManaCost (Fire) | 25 | 15–35 | 위빙 빈도↓ | 남발 가능, 화염 단독 스팸 위험 |
| ManaCost (Lightning) | 25 | 15–35 | 위빙 빈도↓ | 남발 가능, 번개 단독 스팸 위험 |
| CooldownDuration (Blackhole) | 6.0s | 4.0–10.0s (하드 클램프 최소 3.0s — Safe Range는 설계 권장 대역, 하드 클램프는 그보다 낮은 절대 바닥값: 4.0 미만~3.0 구간은 "설계상 비권장이나 시스템이 깨지진 않는" 완충 대역) | 시너지 타이밍 압박↑ | 기본값 기준 재캐스트 마나 회복(5.0s)보다 짧아지면 마나가 다시 실질 제약이 됨 — 아래 교차 제약 (2) 참조 |
| CooldownDuration (Fire/Lightning) | 2.0s / 2.5s | 1.0–4.0s | 위빙 리듬 느려짐 | 너무 짧으면 두 속성 번갈아 스팸이 사실상 무한 캐스트가 됨 — 교차 제약 (1) 참조 |
| ManaRegenRate | 8 mana/s | 5–15 (**하드 클램프 최소 1** — 0 시 코어 적출 외 마나 회복 수단 소멸, 전투 흐름 완전 정지 위험) | Blackhole 재사용 텀 단축, 위기감↓ | 너무 낮으면 코어 적출 실패 시 사실상 마나 고갈로 전투 정체 |
| NoiseLoudness (Blackhole/Fire/Lightning) | 1.0 / 0.6 / 0.7 | 0.3–1.0 (Enemy AI의 `NoiseLoudness` 0.0–1.0 계약 내 서브레인지) + 서수 제약 `Blackhole ≥ Lightning ≥ Fire` 유지 | 어그로 반경↑ — 은근슬쩍 캐스팅 불가능해짐 | 너무 낮으면 큰 폭발형 스펠도 조용해져 Enemy AI 청각 인지 계약이 사실상 무의미해짐 |
| MaxCastsPerSecond | 20 | 10–30 (**하드 클램프 최소 10** — 그 아래는 정상 인간 연타 10–12 CPS에 걸려 반응성 훼손. 30 초과는 사실상 무제한과 동일 — 200fps 기준 6.67 casts/frame, 체감 효과 없음) | 터보/매크로 내성↑, 정상 플레이 시 체감 안 됨 | 너무 낮으면 빠른 위빙 리듬에서 입력 누락 — Rule 2(즉시발동) 판타지 위반. 2026-07-17 luna-overdrive.md design-review 반영 |

**교차 제약 (개별 Safe Range 안이어도 조합이 위반하면 안 되는 불변식):**

1. **무한 스팸 방지**: 모든 속성에 대해 `ManaCost[E] > ManaRegenRate × CooldownDuration[E]` — 위반 시 쿨다운만 기다리면 마나가 항상 차 있어 steady-state 무한 캐스트 성립(예: Fire Cost=15, CD=1.0s, Regen=15 → 15=15 정확히 breakeven으로 위반. 기본값 25 > 8×2.0=16은 만족).
2. **쿨다운이 Blackhole의 실질 제약**: `CooldownDuration[Blackhole] > (ManaCost[Blackhole] − (MaxMana − ManaCost[Blackhole])) / ManaRegenRate` — 기본값: 6.0s > (70−30)/8 = 5.0s 만족. 위반 시 마나가 쿨다운을 무의미하게 만듦.
3. **위빙 1회 보장**: `MaxMana − ManaCost[Blackhole] ≥ max(ManaCost[Fire], ManaCost[Lightning])` — 기본값: 100−70=30 ≥ 25 만족. 블랙홀 직후 최소 한 번의 속성 위빙(초신성/플라즈마 스톰 진입 경로)이 항상 열려 있어야 함. **이 제약이 게임 필라(Dopamine Driven Design) 직결 — 최우선.**

> 각 속성 ManaCost/Cooldown은 서로 연동됨 — Blackhole 하나만 올리거나 내리면 "센터피스 vs 잡스킬" 밸런스가 즉시 깨짐(High-Risk 시스템 표의 "Combo/Tension Gauge → Luna Overdrive" 리스크와 동일 계열). 이 셋을 함께 재조정하되 위 교차 제약 3건을 항상 만족시킬 것.

## Visual/Audio Requirements

| Event | Visual Feedback | Audio Feedback | Priority |
|-------|-----------------|-----------------|----------|
| 블랙홀(Radial Force) 캐스트 | 중심부 흡입 파티클 + 화면 왜곡(radial distortion) | 저음역 흡입 루프 SFX | High |
| 화염 캐스트 | 투사체/폭발 파티클(오렌지-레드 팔레트) | 화염 발사/폭발 SFX | High |
| 번개 캐스트 | 다단히트 전격 이펙트(화이트-블루 팔레트) | 전격 크랙 SFX(히트마다) | High |
| 캐스트 실패(마나부족/쿨다운) | 해당 스펠 아이콘 짧은 흔들림/붉은 틴트 피드백 | 낮은 "실패" SFX(짧고 거슬리지 않게) | Medium |
| 코어 적출 마나 100% 회복(`OnExecuted`) | 마나바 전체 급속 충전 이펙트(플래시) | 마나 풀차지 SFX | Medium |

> Solo mode — `art-director` 미상담. Production 전 수동 확인 필요.
>
> **📌 Asset Spec**: art bible 승인 후 `/asset-spec system:spell-casting-base` 실행해 에셋별 스펙/생성 프롬프트 도출할 것.

## UI Requirements

이 문서는 UI 레이아웃/스타일을 소유하지 않음(Combat HUD GDD 소유) — 다음 인터페이스 데이터만 노출한다:

- `Mana` / `MaxMana` 어트리뷰트 값 및 변경 델리게이트(Combat HUD 마나바 바인딩용)
- 3속성 각각의 `OnCooldown` 상태 + 남은 쿨다운 시간(스펠 아이콘 쿨다운 오버레이용)
- `CostBypass.Active` 태그 존재 여부(오버드라이브 중 "무한마나" UI 표시 트리거용)

> **📌 UX Flag**: Combat HUD가 이 데이터를 소비. Production 진입 전 `/ux-design`으로 `design/ux/combat-hud.md` 작성 시 위 3개 인터페이스 참조할 것(Health/Damage Core UI Requirements와 병합될 예정).

## Acceptance Criteria

1. **GIVEN** Mana=100, Blackhole Ready, **WHEN** Blackhole 캐스트 입력, **THEN** 즉시 발동, NewMana=30, Blackhole OnCooldown 진입(6초), Fire/Lightning은 영향 없음.
2. **GIVEN** Mana=60, Blackhole ManaCost=70, **WHEN** Blackhole 캐스트 입력, **THEN** 캐스트 실패, Mana 변화 없음, 쿨다운 진입 없음.
3. **GIVEN** Fire가 OnCooldown 중, **WHEN** Lightning 캐스트 입력, **THEN** Lightning 정상 발동(Fire 쿨다운과 무관) — 스펠별 개별 쿨다운 확인.
4. **GIVEN** 이동 중(`AddMovementInput` 활성), **WHEN** 임의 속성 캐스트, **THEN** 이동 입력 처리가 동일 프레임에도 100% 정상 진행 — `MovementLocked` 미사용 확인.
5. **GIVEN** 테스트 하네스가 `CostBypass.Active` 태그를 대상에 직접 부여(Luna Overdrive 실제 구현 불요 — 태그 계약은 이 문서 Rule 10 소유이므로 유닛 테스트 스텁으로 검증), **WHEN** Blackhole 연속 3회 캐스트, **THEN** 3회 모두 즉시 발동, Mana 변화 없음, 태그 제거 후 3속성 모두 Ready(잔존 쿨다운 없음).
6. **GIVEN** 코어 적출(`TryExecute`) 성공으로 `OnExecuted` 발행, **WHEN** 이벤트 수신, **THEN** Mana 즉시 MaxMana(100)로 스냅.
7. **GIVEN** Blackhole 임팩트 지점에서 800uu 이내에 적 A, 800uu 밖에 적 B 배치(BaseHearingRadius=800 가정), **WHEN** 임팩트 발생 프레임, **THEN** 적 A만 청각 인지로 Alert 전환, 적 B는 미전환 — `MakeNoise(loudness=1.0)` 발신이 관측 가능한 행동 결과로 검증됨(Integration 테스트, Enemy AI 스텁 필요).
8. **GIVEN** Mana=100, **WHEN** Blackhole 캐스트 직후 같은 초 내 Fire 캐스트, **THEN** 두 캐스트 모두 성공(Blackhole 후 잔여 30 ≥ Fire 25), 최종 Mana=5 — 위빙 1회 보장 교차 제약의 실행 검증.
9. **GIVEN** 같은 프레임에 블랙홀+화염 입력 동시 발생, **WHEN** 처리, **THEN** 두 캐스트 모두 독립적으로 성공/실패 판정(합산 비용 없음).
10. **GIVEN** `CostBypass.Active` 태그 부여(오버드라이브 스텁), **WHEN** 터보 장치로 동일 속성(Blackhole)을 3프레임 연속 프레임마다 캐스트 입력, **THEN** 3프레임 모두 프레임당 1회씩 정상 발동(동일 속성 per-frame cap 미초과 — Rule 11). 추가로 동일 프레임 내 Blackhole 2회째 입력은 무시됨을 확인. *(Integration — 실제 캐스트 게이트 + rate limiter)*
11. **GIVEN** `CostBypass.Active` 태그 부여(오버드라이브 스텁), 1초간 합산 21회(3속성 혼합) 캐스트 입력 연사, **WHEN** 처리, **THEN** 20회만 발동, 21번째는 MaxCastsPerSecond(20) 초과로 거부됨(마나·이펙트 무변화) — Rule 11 전역 상한 검증. *(Logic/unit — rate limiter 단독)*

> (구) AC8 "초신성 `bBypassDefense=true` 실드 관통"은 미설계 시스템(Spell Weaving)의 동작이므로 이 문서의 AC에서 제거 — Spell Weaving GDD 작성 시 그쪽 Acceptance Criteria로 이관(Open Questions 참조). 이 문서가 소유하는 계약은 "오버라이드를 막지 않는다"(Core Rule 8)까지다.

> 2026-07-17 full design-review에서 `qa-lead` 상담 완료 — AC5 스텁화, AC7 관측가능화, (구)AC8 이관, AC8(위빙 보장) 신설은 해당 리뷰 반영분.

## Open Questions

- **캐스트 조준 방식 미정** — 카메라 forward 오토에임/논타겟 즉시발동인지, 커서·스킬샷 방식인지 이 문서에서 결정하지 않음. Player Fantasy의 "즉시성"을 해치지 않는 방향으로 구현 시 확정 필요(gameplay-programmer/ux-designer).
- **게임패드 입력 매핑 미정** — technical-preferences.md의 "게임패드 추후 Full 지원 목표"와 별개로, 3속성+이동+대쉬가 버튼에 어떻게 배분될지 미정 — Dash/Evasion 설계 시 함께 조율.
- **UE5.8 GAS 어트리뷰트 초기화 패턴 재검증 필요** — Health/Damage Core와 동일한 gap 상속(`deprecated-apis.md`의 "Legacy GAS attribute set initialization" 항목 — 스코프는 초기화 함수에 한정, Rule 4/10의 커밋 분기·캔슬 계약은 이 gap과 별개이며 본문에 이미 확정됨). 구현 시 `ue-gas-specialist` 실제 헤더 대조 필수.
- **밸런스 수치 플레이테스트 미검증** — ManaCost/Cooldown/ManaRegenRate/NoiseLoudness는 2026-07-17 리뷰에서 교차 제약 3건(Tuning Knobs)으로 수학적 정합은 확보했으나, 실제 체감(특히 Blackhole 70 코스트의 "대형 투자" 무게감, 위빙 리듬)은 프로토타입 플레이테스트로 재검증 필요.
- **InsufficientMana 실패 피드백 톤 재검토** — 실패 SFX/붉은 틴트(Visual/Audio Requirements)는 DMC5류 레퍼런스에 없는 "거부 UX"라는 지적(game-designer 리뷰) — 억제적 피드백이 몰아치기 리듬을 꺾지 않는지 프로토타입에서 확인, 필요시 "무음+아이콘 딤" 수준으로 완화.
- **(구 AC8) 초신성 실드 관통 검증 이관** — `bBypassDefense=true` 시너지 스펠의 실드 관통 AC는 Spell Weaving GDD 작성 시 해당 문서 Acceptance Criteria로 포함할 것.
