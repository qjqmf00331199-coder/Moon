# Dash/Evasion

> **Status**: Approved
> **Author**: user + game-designer (solo design mode)
> **Last Updated**: 2026-07-17
> **Implements Pillar**: Dopamine Driven Design — 핵심 카타르시스 루프(저스트회피 → 처형)의 시작점

## Overview

Dash/Evasion은 플레이어의 주요 생존기이자 공격적 리포지셔닝 수단이며, 하이리스크-하이리턴 전투의 방아쇠다. 이동(Player Movement) 위에 임펄스를 더해 빠른 속도로 위치를 이동시키고, 체공/지상 상관없이 발동 가능하다. 가장 중요한 기능은 적의 공격 텔레그래프를 읽고 정확한 타이밍에 회피했을 때(Just-Dodge) 대상에게 `State.Executable`(처형 가능) 태그를 부여하여 F키 코어 적출 처형으로 이어지는 판타지를 완성하는 것이다.

## Player Fantasy

대쉬는 단순한 이동기가 아니라 무기다. 적의 거대한 공격이 내려찍히기 직전, 아슬아슬하게 몸을 빼면 적의 자세가 무너지며 처형의 기회가 열린다. 베요네타(Bayonetta)나 니어 오토마타(NieR:Automata)처럼 저스트회피가 성립하는 순간의 쾌감과 보상이 확실해야 하며, 조작감은 무겁지 않고 즉각적이어야 한다.

**Feel Reference**: *NieR:Automata* (빠르고 즉각적인 회피, 회피 후 딜레이 없음), *Bayonetta* (저스트회피 시 확실한 리턴)
**Anti-reference**: *Dark Souls* (무거운 구르기, 긴 후딜레이, 단순 생존 목적) — 본 게임에서는 회피가 콤보와 처형의 시작점이어야 함.

## Detailed Design

### Core Rules

1. **대쉬는 쿨다운 기반의 차지(Charge) 시스템을 갖는다** — 스태미나 없이 고정된 최대 스택(예: 2회)을 가지며, 사용 시 쿨다운(예: 2.0초) 후 1스택씩 충전된다. 
2. **이동 방향 입력에 따른 즉시 위치 이동** — 입력 방향(WASD, 카메라 기준)으로 정해진 대쉬 거리만큼 한 번에 이동하며, 입력이 없으면 캐릭터가 바라보는 방향(카메라 정면)으로 전진한다. 이동 경로는 충돌 검사를 수행하고, 대쉬 지속시간에는 일반 이동 입력을 받지 않는다.
3. **지상/공중 사용 가능** — Grounded 상태는 물론 Airborne 상태에서도 사용 가능(Air-Dash). 공중 대쉬는 Arena Morphing 체공전투의 핵심 기동 수단으로 활용되며, 수평 임펄스 외에 Z축 체공 시간을 연장하는 미세한 상승(또는 낙하 방지) 임펄스를 포함한다.
4. **임펄스 합성 방식: Override(덮어쓰기)** — 대쉬 발동 시 기존 velocity(수평+수직 momentum 전부)를 대쉬 벡터로 완전히 덮어쓴다(additive 아님). 매 대쉬가 momentum 무관하게 항상 동일한 거리/속도로 발동되어 "즉각 방향전환" 판타지를 보장하고 예측 가능한 포지셔닝을 만든다. (player-movement.md Open Question 해결 — design-review 2026-07-17)
5. **회피 프레임 (I-frames)** — 대쉬 시작 시 즉시 Health/Damage Core의 `State.Invulnerable` 태그를 플레이어에게 부여하며, 대쉬 모션(Tuning Knob: `dash_invuln_duration`)이 끝나는 시점에 태그를 제거한다.
6. **저스트회피 (Just-Dodge) 타이밍** — Enemy AI (base)가 발행하는 `OnAttackTelegraphed`와 `OnAttackCommitted` 이벤트를 구독한다. 적의 공격 텔레그래프 윈도우 내의 특정 타이밍(커밋 직전, `JustDodgeWindow`)에 플레이어가 대쉬를 발동하고, 그 적의 공격 범위(히트박스/사거리) 내에 있었다면 저스트회피로 판정한다.
7. **저스트회피 보상** — 저스트회피 성공 시 2가지 효과가 발생한다:
   - 판정 시점에 조건을 만족하는 **모든** 적 개체에게 각각 Health/Damage Core의 `State.Executable` 태그를 부여한다. (지속시간: `executable_duration`)
   - 대상이 몇 명이든 **대쉬 차지는 1스택만** 즉시 반환(또는 충전 시간 초기화)한다 — 대쉬 이벤트 1회당 반환 상한 1스택 (다수 동시 텔레그래프 상황에서 차지 무한 스택 방지, design-review 2026-07-17 결정).
8. **캐스팅 및 애니메이션 캔슬** — 대쉬는 다른 액션을 방해받지 않고 즉시 발동되어야 하며, Spell Casting 중에도 캔슬 없이 발동 가능하다(Player Movement Core Rule 7). 
9. **MovementLocked 무시 불가** — 미래의 Status Effect 시스템이 `MovementLocked`를 걸었다면, 대쉬 발동은 불가능하다.

### States and Transitions

| State/Tag | Entry Condition | Exit Condition | Behavior |
|-----------|-----------------|-----------------|----------|
| Dashing | 입력 + 차지 1 이상 + `MovementLocked` 아님 | 대쉬 지속시간 만료 | 수평(필요시 수직) 임펄스 적용, `State.Invulnerable` 획득 |
| `State.Invulnerable` (플레이어) | Dashing 상태 진입 | Dashing 상태 종료 | 데미지 무시 (Health/Damage Core가 차단) |
| `State.Executable` (적 대상) | 적 공격의 Just-Dodge 윈도우 내 플레이어 회피 성공 | 지속시간 만료 또는 Core Extraction Execution 소비 | F키 처형 프롬프트 활성화 |

### Interactions with Other Systems

- **Player Movement** (상류): `AddMovementInput` 또는 Velocity API를 오버라이드하여 대쉬 속도를 적용. 공중 대쉬 시 Z축 velocity 오버라이드/Launch API 호출.
- **Health/Damage Core** (상류): `State.Invulnerable` 태그 획득/제거(플레이어). `State.Executable` 태그 부여(적).
- **Enemy AI (base)** (상류): `OnAttackTelegraphed`, `OnAttackCommitted` 델리게이트를 수신하여 저스트회피 가능 상태를 판단.
- **Camera System (base)** (상류): 카메라 기준 방향 입력 파악.
- **Combo/Tension Gauge** (하류): 저스트회피 성공 시 콤보 게이지 상승 이벤트 발송 예정.
- **Core Extraction Execution** (하류, 미설계): 이 시스템이 적에게 붙인 `State.Executable` 태그를 소비.

## Formulas

### Just-Dodge Check

```
IsJustDodge = (CurrentTime >= AttackCommittedTime - JustDodgeWindow) AND (CurrentTime <= AttackCommittedTime) AND IsInAttackRange
```

| Variable | Type | Range | Source | Description |
|----------|------|-------|--------|-------------|
| AttackCommittedTime | float | 런타임 | Enemy AI | 적의 `OnAttackCommitted` 발생 예정 시각 |
| JustDodgeWindow | float | 0.15–0.3 | Tuning Knob | 공격 커밋 직전 회피 판정이 인정되는 시간 (단위: 초) |
| IsInAttackRange | bool | true/false | 런타임 공간 판정 | 대상 공격의 유효 반경/히트박스 내에 플레이어가 존재하는가 |

**Notes**: 저스트회피는 "아슬아슬하게 피했다"는 느낌을 위해 공격 커밋 시점 직전(`JustDodgeWindow`)에 대쉬가 입력되어야 함. 

**Example**: `AttackCommittedTime=10.0s`, `JustDodgeWindow=0.2s` → 유효 구간은 `[9.8s, 10.0s]`. 플레이어가 `CurrentTime=9.85s`에 대쉬 발동 + `IsInAttackRange=true` → `IsJustDodge=true`. `CurrentTime=9.5s`에 발동했다면(구간 밖) → `IsJustDodge=false` (일반 대쉬로만 처리, AC 5 참조).

### Dash Cooldown & Charges

```
CurrentCharges = clamp(CurrentCharges + (DeltaTime / RechargeRate), 0, MaxCharges)
```

| Variable | Type | Range | Source | Description |
|----------|------|-------|--------|--------------|
| CurrentCharges | float | 0–MaxCharges | 런타임 상태 | 매 틱 갱신되는 현재 보유 차지 (소비 시 -1, 정수 UI 표시는 floor) |
| DeltaTime | float | 프레임 델타(초) | 엔진 틱 | 매 틱 호출 시 프레임 간격 |
| RechargeRate | float | 1.5–4.0 | Tuning Knob | 차지 1스택 완전 충전에 걸리는 시간(초) |
| MaxCharges | int | 1–3 | Tuning Knob | 최대 보유 가능 차지 수 |

**Example**: `RechargeRate=2.0s`, `MaxCharges=2`, 대쉬 소비 직후 `CurrentCharges=1.0`. `DeltaTime=0.1s`마다 `+0.05` 누적 → 2.0초 후 `CurrentCharges=2.0`(clamp 상한 도달, 완전 충전).

## Edge Cases

| Scenario | Expected Behavior | Rationale |
|----------|-------------------|-----------|
| 다수의 적 공격이 겹치는 상황에서 저스트회피 | 윈도우 조건을 만족하는 **모든** 적에게 `State.Executable` 태그 부여 | 군중 제어와 대규모 처형의 카타르시스 극대화 |
| 체공 중 지상을 향해 대쉬 (또는 반대) | 수평 임펄스만 주입되며 Z축 하강 속도는 CMC 중력/낙하 가속을 따름. 단 에어 대쉬 발동 시 약간의 체공 보정(Z축 임펄스 0 세팅)을 통해 일시적 체공 | 이동 시스템의 관성을 존중하되 체공 전투의 조작성 확보 |
| 대쉬 중 지형 모서리 밖으로 이탈 | 수평 임펄스를 유지하며 Airborne 상태로 자연스럽게 전환 | Player Movement의 기존 이탈 로직 계승 |
| `State.Executable` 태그가 부여된 적이 이미 다른 디버프로 해당 태그를 갖고 있음 | 태그 참조 카운트 추가 (오버레이 중첩 유지) | Health/Damage Core의 참조 카운트 기반 태그 설계 준수 |

## Dependencies

| System | Direction | Nature of Dependency |
|--------|-----------|----------------------|
| Player Movement | 이 시스템이 의존 | 수평/수직 이동 임펄스 API 오버라이드 |
| Health/Damage Core | 이 시스템이 의존 | `State.Invulnerable` 획득, `State.Executable` 부여 |
| Enemy AI (base) | 이 시스템이 의존 | `OnAttackTelegraphed`/`OnAttackCommitted` 델리게이트 참조 |
| Camera System (base) | 이 시스템이 의존 | 이동 입력 축 기준 |
| Combat HUD | 이 시스템에 의존 | 대쉬 차지 스택(남은 횟수 및 쿨다운 게이지) 렌더링을 위한 데이터 전달 |

## Tuning Knobs

| Parameter | Current Value | Safe Range | Effect of Increase | Effect of Decrease |
|-----------|---------------|------------|--------------------|--------------------|
| MaxCharges | 2 | 1–3 | 이동/생존 너무 관대해짐 | 스태미나 부족 느낌, 답답함 유발 |
| RechargeRate | 2.0초 | 1.5–4.0초 | 쿨다운 길어져 빈틈 발생 | 무한 대쉬 스팸 가능 |
| DashDuration | 0.2초 | 0.15–0.3초 | 구르기처럼 길어져 템포 하락 | 이동 거리 짧아져 리포지셔닝 실패 |
| DashSpeedMultiplier | 2.5 | 2.0–4.0 | 너무 멀리 나가 조준 어려움 | 이동 효과 미미함 |
| JustDodgeWindow | 0.2초 | 0.15–0.3초 | 판정이 관대해져 긴장감 저하 | 저스트회피 실패 확률 급증 (난이도 폭증) |
| ExecutableDuration | 3.0초 | 2.0–5.0초 | 처형 기회가 너무 길게 유지됨 | 기회를 잡기 전에 태그 소멸 |
| DashInvulnDuration | 0.2초 | 0.15–0.4초 | 생존 쉬워짐 (무적기 스팸) | 회피 판정 빡빡해짐 |

> `DashInvulnDuration`은 `DashDuration`과 거의 동일하게 맞춰 대쉬 중에는 안전함을 보장.

## Visual/Audio Requirements

| Event | Visual Feedback | Audio Feedback | Priority |
|-------|-----------------|-----------------|----------|
| 대쉬 발동 | 짧은 잔상(Motion Blur / Afterimage 이펙트), 발밑 흙먼지 파티클 | 빠르고 날카로운 바람 가르는 SFX | High |
| 저스트회피 성공 | 화면 미세한 슬로우모션 힌트(옵션, 히트스탑 대체 가능), 대상 적의 붉은색 강렬한 점멸 | 카타르시스를 주는 경쾌한 '칭' SFX | High |
| `State.Executable` 지속 | 대상 머리 위 혹은 몸체에 명확한 처형 가능 인디케이터 (글리치 링 등) | 짧은 루프형 공명음 (처형 대기) | High |
| 저스트회피 성공 (카메라) | 화면 횡방향 카메라 쉐이크 재생 (Camera System 소유 API 호출, Dash/Evasion이 트리거) — camera-system-base.md 계약 | — | High |

## UI Requirements

- 전투 HUD 쪽에 대쉬 차지 스택(남은 횟수 및 쿨다운 게이지) 노출 필요. (Combat HUD 시스템에 데이터 전달)
- 저스트회피 성공 시 화면 외곽 미세 이펙트 (선택사항, Combat HUD와 협의)

## Acceptance Criteria

1. **GIVEN** 대쉬 차지가 1 이상이고 입력 방향이 주어짐, **WHEN** 대쉬 버튼 입력, **THEN** 차지가 1 소모되며 입력 방향으로 `MaxWalkSpeed × DashSpeedMultiplier × DashDuration` 거리만큼 즉시 이동한다. 이동은 충돌 검사를 수행하며, 대쉬 지속시간 동안에는 일반 이동 입력으로 추가 전진하지 않는다.
2. **GIVEN** 플레이어가 대쉬 중임, **WHEN** 데미지 이펙트가 들어옴, **THEN** `State.Invulnerable`로 인해 데미지가 무시된다.
3. **GIVEN** 적 공격의 `OnAttackTelegraphed` 이후 `JustDodgeWindow` (0.2초) 내에 플레이어가 대쉬 발동, **WHEN** 플레이어가 공격 반경에 있음, **THEN** 해당 적에게 `State.Executable` 태그가 부여되고 처형 프롬프트가 활성화된다.
4. **GIVEN** 저스트회피 조건이 달성됨, **WHEN** 판정 완료 시, **THEN** 플레이어의 대쉬 차지가 즉시 1 회복된다.
5. **GIVEN** 적이 공격을 시작한 지 얼마 안 되어 `JustDodgeWindow` 진입 전임, **WHEN** 플레이어가 대쉬 발동, **THEN** 대쉬는 정상 발동되나 적에게 `State.Executable` 태그는 부여되지 않는다.
6. **GIVEN** 체공 중 (Airborne) 대쉬 입력, **WHEN** 차지 존재 시, **THEN** Z축 추락을 잠시 멈추거나 늦추며 수평 방향으로 대쉬한다.
7. **GIVEN** Status Effect 시스템이 플레이어에게 `MovementLocked` 태그를 건 상태, **WHEN** 대쉬 버튼 입력, **THEN** 대쉬는 발동하지 않으며 차지도 소모되지 않는다.
8. **GIVEN** 대쉬 발동 시 동시에 2개 이상의 적이 Just-Dodge 조건을 만족함, **WHEN** 판정 완료, **THEN** 조건 만족한 모든 적에게 `State.Executable`이 부여되나 플레이어의 대쉬 차지는 정확히 1스택만 반환된다(다중 반환 없음).

## Open Questions

- 저스트회피 성공 시 대상 주변에 광역 타격을 주어 여러 적을 동시에 비틀거리게 할 것인가? (군중 제어 강화) -> MVP 이후 테스트 예정.
- Z축 공중 대쉬 시 정확한 엔진 틱 기반 물리 조작(Launch Character API 오버라이드) 검증 필요.

## Cross-Reference Resolutions (design-review 2026-07-17)

- **enemy-ai-base.md:155 교차검증 완료**: 대쉬 거리 = `MaxWalkSpeed(600uu/s, player-movement.md) × DashSpeedMultiplier(2.5) × DashDuration(0.2s) = 300uu`. `MinEngageRange`(400uu, enemy-ai-base.md)보다 짧으므로 "필드 밖에서 한 방에 근접권 진입"은 불가하나, 전투는 통상 이미 Attack band(400–1200uu) 진입 후 벌어지므로 밴드 내에서는 대쉬 1회로 충분히 근접권 도달 가능. 수치 상충 아님 — 확인 완료로 open question 해소.
- **enemy-ai-base.md:109 교차검증 완료**: `MinTelegraphWindow`(0.4s, enemy-ai-base.md 기본값) > `JustDodgeWindow`(0.2s, 본 문서) — 저스트회피 판정 구간이 텔레그래프 구간에 완전히 포함되므로 회피가 항상 성립 가능함을 확인.
