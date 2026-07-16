# Enemy AI (base)

> **Status**: Designed (pending review)
> **Author**: user + game-designer / ai-programmer (solo review mode)
> **Last Updated**: 2026-07-16
> **Implements Pillar**: Dopamine Driven Design — foundation supplying the enemy population that combo/overdrive/execution/chain-destruction all act upon

## Overview

Enemy AI (base)는 이 확장 범위의 모든 적 캐릭터가 공유하는 인식·의사결정·이동·사망정리의 공용 기반이다. 데이터/인프라 관점에서는 UE5.8 `AIController` + Behavior Tree(또는 State Tree) + `AIPerception` 컴포넌트를 통해 적이 플레이어를 감지하고, 순찰→추적→공격→사망의 상태를 오가며, GAS `AttributeSet`의 MaxHealth 슬롯(Health/Damage Core가 슬롯만 제공, 실제 값은 이 문서 소유)에 아키타입별 수치를 채워 넣는 역할을 한다. 하지만 이 시스템은 절대 단순히 '움직이는 표적'으로 느껴지면 안 된다 — 콤보/오버드라이브가 축적되려면 끊임없이 몰려드는 위협적인 적이 필요하고, 코어 적출(F키)과 저스트회피(대쉬)가 하이리스크-하이리턴으로 느껴지려면 적의 공격이 읽을 수 있는 텔레그래프와 함께 실제 위협감을 줘야 하며, 환경 연쇄파괴(포자/다리 붕괴)의 카타르시스는 결국 '얼마나 많은 적이, 얼마나 위협적으로 몰려있었는가'에 달려 있다. Player Movement가 손맛의 뼈대, Health/Damage Core가 타격감의 뼈대였다면, Enemy AI (base)는 '싸울 가치가 있는 상대'의 뼈대다.

## Player Fantasy

플레이어는 적을 두 가지 층위로 느껴야 한다: (1) 무리로 몰려올 땐 '쓸어버리는' 손맛의 연료 — 콤보 게이지를 채우고 오버드라이브로 가는 디딤돌, (2) 개별 공격 순간엔 '읽고 응수하는' 위협 — 텔레그래프가 명확해서 저스트회피(대쉬)와 F키 처형의 타이밍이 플레이어 자신의 실력으로 느껴져야 한다. 인프라 관점에선 Perception/BT/NavMesh가 조용히 동작하지만, 플레이어가 실제로 체감하는 건 "이 자식이 지금 내리칠 거다"라는 확신과 그걸 피했을 때의 쾌감이다.

**Feel Reference**: *DOOM Eternal*의 잡몹(fodder)/강적(heavies) 구분 — 잡몹은 물량으로 밀어붙여 학살감을 주고, 강적은 명확한 텔레그래프로 글로리킬 타이밍을 유도. *Bayonetta*/*DMC*류의 "위치·타이밍 인지 가능한 위협" 원칙(적 공격 판정이 시각적으로 반드시 예고됨).
**Anti-reference**: 판정을 숨기거나 예고 없이 때리는 언페어 AI — Health/Damage Core의 "확실함" 철학과 정면 충돌하고, 저스트회피 타이밍 자체가 성립 불가능해짐.

> Solo mode — `creative-director` 미상담. Production 전 수동 확인 필요.

## Detailed Design

### Core Rules

1. **아키타입 2종 MVP**: Grunt(근접)와 Ranged(원거리) — 동일한 상태머신·인지 로직을 공유하고 무기/애니메이션/거리 파라미터만 아키타입별로 상이. 아키타입 확장은 향후 리비전 대상.
2. **인지는 시야+청각**: `AIPerception`의 Sight(원뿔/거리)와 Hearing(소음 이벤트) 두 감각을 사용. 플레이어 착지음/스펠 피격음/파괴 지오메트리 붕괴음 등 큰 소음은 Hearing을 트리거해 시야 밖에서도 Alert 상태로 전환시킨다(정확한 소음 이벤트 소스 목록은 이 문서가 정의하지 않음 — 발신 시스템이 `MakeNoise` 호출 계약만 따르면 됨).
3. **공격 텔레그래프는 애니메이션 주도**: 공격 애니메이션의 윈드업 구간 시작 지점에 `AnimNotify`로 `OnAttackTelegraphed` 이벤트를 발행하고, 실제 판정(데미지 GameplayEffect 적용)은 스트라이크 프레임의 별도 `AnimNotify`(`OnAttackCommitted`)에서만 발생한다 — 두 이벤트 사이 구간이 곧 플레이어가 읽고 반응할 수 있는 텔레그래프 윈도우다.
4. **텔레그래프 이벤트는 공개 델리게이트로 노출**: `OnAttackTelegraphed`/`OnAttackCommitted`는 이 시스템이 소유하는 공개 인터페이스다 — Dash/Evasion(미설계)이 이를 구독해 저스트회피 판정 타이밍을 계산한다. 저스트회피 성공 여부 판정과 `State.Executable` 태그 부여는 Dash/Evasion 소유이며, 이 문서는 이벤트 발행까지만 책임진다.
5. **Ranged 아키타입은 거리 밴드를 유지**: 최소/최대 사거리(Tuning Knob) 사이를 유지하려 하며, 플레이어가 최소 사거리 안으로 들어오면 후퇴 이동을 시도한다. 공격 판정 자체는 Rule 3과 동일한 텔레그래프 계약을 따른다(발사체 스폰 = `OnAttackCommitted`).
6. **MaxHealth 실값은 이 문서 소유**: Health/Damage Core가 제공하는 어트리뷰트 슬롯에, 아키타입별 실제 MaxHealth 값을 이 문서가 채운다(Formulas 참조).
7. **사망 처리는 Health/Damage Core의 `OnDeath` 구독으로만 진입**: 자체적으로 Health를 판정하지 않는다 — Rule 1(단일 데미지 진입점)을 그대로 신뢰하고, `OnDeath` 수신 시에만 Dead 상태로 전환한다(충돌/인지 비활성화 → 래그돌 재생 → 시체 유지 타이머 → 디스폰).
8. **Stagger(경직) 상태는 슬롯만 소유**: 이 시스템은 Stagger라는 상태 슬롯과 "이동/공격 중단" 훅을 제공하지만, 정확히 언제 경직이 트리거되는지(피격 임계치, CC 종류)는 Super Armor / CC Interrupt(미설계) 소유. Enemy AI는 외부에서 `TriggerStagger()`/`ClearStagger()` 호출을 받아 상태를 전환할 뿐이다.
9. **아키타입 식별은 GameplayTag로 노출**: 각 적 인스턴스는 `Enemy.Archetype.Grunt` / `Enemy.Archetype.Ranged` 태그를 가진다 — Enemy Elite Shield(미설계) 등 하류 시스템이 아키타입별로 다른 처리를 걸 때 이 태그를 참조점으로 사용할 것으로 예상. 태그의 존재/이름만 이 문서 소유, 태그를 보고 무엇을 할지는 소비 시스템 소유.
10. **`State.Executable`/`State.Invulnerable` 태그는 상태머신을 막지 않는다**: 두 태그는 오버레이이므로, 태그가 붙어 있어도 Chase/Attack 등 기존 상태 전환 로직은 그대로 진행된다(단, Dead 전환은 Rule 7 그대로 최우선).

### States and Transitions

| State | Entry Condition | Exit Condition | Behavior |
|-------|-----------------|-----------------|----------|
| Idle | 스폰 시 기본 상태 | Sight 또는 Hearing으로 플레이어 감지 | 정지 또는 패트롤 웨이포인트 순회 |
| Alert (Investigate) | Hearing 이벤트 발생(Sight 미확보) | N초 내 Sight 미확보 → Idle 복귀 / Sight 확보 → Chase | 소음 발생 지점으로 이동, 주변 스캔 |
| Chase | Sight로 플레이어 확인 | 공격 조건(사거리+쿨다운) 충족 → Attack / Sight 상실 후 일정시간 경과 → Alert 경유 후 Idle | NavMesh 경로로 추적(`MoveToActor`), Ranged는 거리 밴드 유지 이동 |
| Attack | Chase 중 공격 조건 충족 | `OnAttackCommitted` 처리 완료 또는 Stagger로 캔슬 | Rule 3/4의 애니메이션 주도 텔레그래프 → 커밋 |
| Stagger | 외부 시스템의 `TriggerStagger()` 호출(Super Armor/CC Interrupt 소유) | 경직 애니메이션 종료 또는 외부 `ClearStagger()` | 이동/공격 일시 중단, 진행 중이던 Attack은 캔슬 |
| Dead | Health/Damage Core `OnDeath` 이벤트 수신 | 시체 유지 타이머 만료 | 충돌/인지 비활성화 → 래그돌 → N초 후 디스폰 |

### Interactions with Other Systems

- **Health/Damage Core** (상류): `OnDeath` 구독 → Dead 전환(이 시스템은 자체 사망 판정 없음). MaxHealth 어트리뷰트 슬롯에 아키타입별 실값을 채움(Rule 6). `State.Invulnerable`/`State.Executable`은 읽기만 하며 부여하지 않음.
- **Dash/Evasion** (하류, 미설계): `OnAttackTelegraphed`/`OnAttackCommitted` 델리게이트를 구독해 저스트회피 타이밍 판정. `State.Executable` 부여는 전적으로 그쪽 소유.
- **Enemy Elite Shield** (하류, 미설계): `Enemy.Archetype.*` 태그를 참조해 실드 부착 대상을 결정할 것으로 예상. 실드 HP/파괴는 그쪽 소유.
- **Super Armor / CC Interrupt** (하류, 미설계): Stagger 상태의 트리거 조건을 소유. 이 문서는 `TriggerStagger()`/`ClearStagger()` 훅만 제공.
- **Core Extraction Execution** (하류, 미설계): `State.Executable` 태그 존재 여부와 무관하게 이 시스템의 상태머신은 정상 진행(태그는 오버레이, Rule 10).
- **Environmental Chain-Destruction / Destructible Geometry** (하류, 미설계): 낙사·붕괴로 인한 사망도 Health/Damage Core의 동일 진입점을 통과하므로 별도 처리 불필요 — Rule 7 그대로 재사용.

## Formulas

### Attack Telegraph Window

`TelegraphWindow = AttackCommitFrame_Time - AttackTelegraphFrame_Time`

**Variables:**
| Variable | Symbol | Type | Range | Description |
|----------|--------|------|-------|-------------|
| AttackTelegraphFrame_Time | — | float(sec) | 애니메이션별 정의 | `OnAttackTelegraphed` AnimNotify 재생 시각 |
| AttackCommitFrame_Time | — | float(sec) | 애니메이션별 정의, > AttackTelegraphFrame_Time | `OnAttackCommitted` AnimNotify 재생 시각 |

**Output Range**: `TelegraphWindow` ≥ `MinTelegraphWindow`(Tuning Knob, 기본 0.4초, Safe Range 0.3–0.6초) — 이보다 짧으면 저스트회피가 성립 불가능한 것으로 간주해 애니메이션 리비전 대상.
**Example**: 윈드업 시작 0.2초, 커밋 0.65초 → TelegraphWindow=0.45초 (기준 통과).

### Noise Detection Radius

`NoiseDetectionRadius = BaseHearingRadius * NoiseLoudness`

**Variables:**
| Variable | Symbol | Type | Range | Description |
|----------|--------|------|-------|-------------|
| BaseHearingRadius | — | float(uu) | 아키타입별, 기본 Grunt 800 / Ranged 600, Safe Range 400–1200 | 아키타입 기본 청각 반경 |
| NoiseLoudness | — | float | 0.0–1.0, 소음 발신 시스템이 `MakeNoise` 호출 시 지정 | 소음 세기 배율(착지=0.6, 스펠 피격=0.8, 구조물 붕괴=1.0 예시) |

**Output Range**: 0–1200uu. **Example**: Grunt(800uu) 기준 구조물 붕괴 소음(1.0) → 800uu 반경 내 즉시 Alert 전환.

### Ranged Engage Distance Check

```
ShouldRetreat = PlayerDistance < MinEngageRange
ShouldAttack  = MinEngageRange <= PlayerDistance <= MaxEngageRange
```

**Variables:**
| Variable | Symbol | Type | Range | Description |
|----------|--------|------|-------|-------------|
| PlayerDistance | — | float(uu) | 0 이상, 런타임 측정값 | Ranged 개체-플레이어 간 거리 |
| MinEngageRange | — | float(uu) | 기본 400, Safe Range 300–600 | 이 아래로 좁혀지면 후퇴 시도 |
| MaxEngageRange | — | float(uu) | 기본 1200, Safe Range 900–1500 | 이 밖이면 Chase(접근)로 재전환 |

**Output Range**: 두 불리언 중 하나만 참(400–1200uu 밴드 내 = 공격, 미만 = 후퇴, 초과 = 접근). **Example**: PlayerDistance=350 → ShouldRetreat=true.

### Enemy MaxHealth by Archetype

| Variable | Type | Range | Source | Description |
|----------|------|-------|--------|-------------|
| MaxHealth (Grunt) | float | 기본 **30**, Safe Range 20–50 | 이 문서 소유 | 근접 잡몹 — RawDamage 25 기준 2타 처치, 물량감 유지 |
| MaxHealth (Ranged) | float | 기본 **20**, Safe Range 15–35 | 이 문서 소유 | 원거리 위협 — 빠르게 우선 처치되어야 압박감 완화 |

**Output Range**: 20–50 (플레이어 MaxHealth 100 대비 상대적으로 낮게 설정 — Health/Damage Core의 "확실한 처치" 철학 지원).
**Example**: RawDamage=25 스펠 히트 → Grunt(30) 1타 생존(5 남음)+2타 사망, Ranged(20) 1타 즉사.

> Open Question으로 등록: `MinTelegraphWindow`(0.4초)는 Dash/Evasion의 실제 저스트회피 입력 버퍼 창과 아직 대조 검증되지 않은 가정값 — Dash/Evasion GDD 작성 시 재검증 필요.

## Edge Cases

| Scenario | Expected Behavior | Rationale |
|----------|-------------------|-----------|
| `OnAttackTelegraphed` 이후 커밋 전에 Stagger 트리거 | 공격 캔슬, `OnAttackCommitted` 미발행, 데미지 미적용 | CC가 정직하게 공격을 끊어야 한다는 계약 보장 |
| `OnAttackTelegraphed` 이후 커밋 전에 `OnDeath` 수신 | 즉시 Dead 전환, `OnAttackCommitted` 미발행 | 죽은 채로 공격이 뒤늦게 맞는 모순 방지 |
| 텔레그래프 시작 후 플레이어가 Sight 밖으로 이탈 | 이미 시작된 공격은 그대로 커밋까지 진행(캔슬 안 함) — Stagger/Death만이 유일한 캔슬 조건 | 텔레그래프 시작이 "확약"이어야 저스트회피 판정이 의미를 가짐 |
| 동일 프레임에 다중 소음 이벤트 발생(붕괴+스펠 히트 동시) | 가장 큰 `NoiseDetectionRadius` 이벤트 하나만 채택, Alert 전환 정확히 1회 | 중복 Alert 트리거로 인한 상태 스팸 방지 |
| Ranged가 최소 사거리 밑으로 붙잡히고 후퇴 경로가 NavMesh상 막힘 | 경로탐색 실패 2초(Tuning Knob) 지속 시 후퇴 포기, 거리 밴드 무시하고 그 자리에서 공격 허용 | 영원히 후퇴만 시도하며 무력해지는 AI 방지 |
| Dead 상태 진입 후 래그돌이 추가 물리충돌(붕괴 등)에 휘말림 | 추가 데미지/충돌 판정 완전 무시, 디스폰 타이머 그대로 진행 | 이미 Dead — Health/Damage Core Edge Case와 동일 철학 |
| 애니메이션의 `TelegraphWindow`가 `MinTelegraphWindow`(0.4초) 미만으로 세팅됨 | 런타임 자동 보정 없음, 빌드/에디터 단계 경고 로그만(콘텐츠 버그로 QA 픽업) | 런타임 강제 보정은 애니메이션-판정 싱크를 깨뜨림 |
| 여러 적이 동시에 같은 플레이어를 Chase/Attack | MVP 범위엔 그룹 협응(포위/어그로 분산) 없음 — 각 개체 완전 독립 실행 | 그룹 AI는 이 확장 범위 밖, 후속 리비전 대상 |

## Dependencies

| System | Direction | Nature of Dependency |
|--------|-----------|----------------------|
| Health/Damage Core | 이 시스템이 의존 | `OnDeath` 이벤트 구독(Dead 전환), MaxHealth 어트리뷰트 슬롯 사용, `State.Invulnerable`/`State.Executable` 태그 읽기 전용 참조 |
| Dash/Evasion (미설계) | Dash/Evasion이 이 시스템에 의존 | `OnAttackTelegraphed`/`OnAttackCommitted` 델리게이트 구독 |
| Enemy Elite Shield (미설계) | Enemy Elite Shield가 이 시스템에 의존 | `Enemy.Archetype.*` GameplayTag 참조 |
| Super Armor / CC Interrupt (미설계) | Super Armor / CC Interrupt가 이 시스템에 의존 | Stagger 상태 슬롯, `TriggerStagger()`/`ClearStagger()` 훅 사용 |
| Core Extraction Execution (미설계) | Core Extraction Execution이 이 시스템에 의존 | `State.Executable` 태그가 붙은 대상의 상태머신이 정상 진행됨을 전제(Rule 10) |
| Boss Phase (미설계) | Boss Phase가 이 시스템에 의존(간접) | 동일 상태머신/텔레그래프 패턴을 보스 개체에도 재사용할 것으로 예상 |
| Environmental Chain-Destruction / Destructible Geometry (미설계) | 상호 참조 없음(직접 의존 아님) | 낙사/붕괴 사망은 Health/Damage Core 진입점을 통과 — 이 시스템은 그 결과(`OnDeath`)만 수신 |

> Health/Damage Core GDD의 Dependencies 표에도 "Enemy AI (base)가 이 시스템에 의존"이 이미 기록되어 있음 — 양방향 일치 확인됨.

## Tuning Knobs

| Parameter | Current Value | Safe Range | Effect of Increase | Effect of Decrease |
|-----------|---------------|------------|--------------------|--------------------|
| SightRange (공통) | 1200uu | 800–1800uu | 더 멀리서 감지, 매복/은신 전략 무력화 | 우회 전략이 유효해지나 너무 낮으면 뒤치기 당한 느낌 유발 |
| SightAngle (공통) | 90° | 60–120° | 시야 사각 좁아져 측면 우회 쉬워짐 | 전방위에 가까워져 우회가 무의미해짐 |
| BaseHearingRadius (Grunt) | 800uu | 400–1200uu | 소음만으로도 쉽게 각성, 은밀 플레이 무력화 | 큰 소음도 무시하는 둔한 AI로 느껴짐 |
| BaseHearingRadius (Ranged) | 600uu | 400–1200uu | 〃 | 〃 |
| MinEngageRange | 400uu | 300–600uu | 더 가까이 와야 후퇴 — 근접전 유도 강해짐 | 너무 자주 후퇴해 산만해 보임 |
| MaxEngageRange | 1200uu | 900–1500uu | 더 먼 거리서도 사격 — 압박감 강함 | 접근을 쉽게 허용, 근접전으로 몰림 |
| MinTelegraphWindow | 0.4초 | 0.3–0.6초 | 회피가 관대해짐(난이도 하락) | 저스트회피 자체가 성립 불가능해질 위험(Formulas 참조) |
| AlertToIdleTimeout | 5초 | 3–8초 | 끈질기게 경계 유지 | 너무 빨리 포기해 허술해 보임 |
| RetreatPathfindFailTimeout | 2초 | 1–4초 | 후퇴 시도를 오래 지속(멍청해 보일 위험) | 너무 빨리 포기하고 제자리 사격 — 회피 난이도 상승 |
| CorpseHoldDuration | 4초 | 2–8초 | 시체가 오래 남아 대규모 학살 시 퍼포먼스 부담 ↑ | 너무 빨리 사라지면 처치 확인의 시각적 만족감 감소 |
| MaxHealth (Grunt) | 30 | 20–50 | 처치 지연, 물량감(swarm) 약화 위험 | 원샷킬 남발, 위협감 상실 |
| MaxHealth (Ranged) | 20 | 15–35 | 처치 지연, 압박 지속시간 늘어남 | 너무 쉽게 죽어 위협 요소 상실 |

> 상호작용 주의: `MinEngageRange`/`MaxEngageRange`는 Dash/Evasion(미설계)의 대쉬 거리와 맞물림 — 대쉬 한 번에 MinEngageRange 안쪽까지 파고들 수 있어야 근접 압박 전략이 성립. Dash/Evasion 설계 시 재검증 필요.

## Visual/Audio Requirements

| Event | Visual Feedback | Audio Feedback | Priority |
|-------|-----------------|-----------------|----------|
| Idle → Alert (Investigate) | 물음표/경계 아이콘 등 상태 인디케이터, 몸 방향 소음원 쪽으로 회전 | 경계 진입 보이스/그럴 SFX | Medium |
| Alert → Chase (Sight 확보) | 확실한 어그로 인디케이터(느낌표), 눈/무기 발광 등 즉각적 인지 신호 | 어그로 보이스/그럴 SFX | High |
| `OnAttackTelegraphed` (윈드업 시작) | 무기/신체에 명확한 윈드업 포즈 + 강조 이펙트(빛나는 궤적 예고선, Ranged는 조준 레이저/차징 파티클) | 윈드업 SFX(스웰링 사운드) | High — 저스트회피 판정의 시각적 근거 |
| `OnAttackCommitted` (판정 프레임) | 타격 이펙트/임팩트 파티클(Health/Damage Core 히트플래시와 연동) | 타격 SFX | High |
| Stagger 진입 | 휘청임 애니메이션, 짧은 스턴 이펙트(별/링) | 피격 그럴 SFX | Medium |
| Death | 래그돌 전환 + 사망 파티클(코어 적출 연출과는 구분) | 사망 SFX | High |

> Solo mode — `art-director` 미상담. Production 전 수동 확인 필요.

## UI Requirements

이 문서는 UI 레이아웃/스타일을 소유하지 않는다. 다음 상태만 노출한다:

- 현재 AI State(Idle/Alert/Chase/Attack/Stagger/Dead) — 월드스페이스 인디케이터(느낌표/물음표) 표시 트리거용
- `OnAttackTelegraphed` 이벤트 — 조준선/차징 이펙트 등 액터 부착 시각 효과 트리거용(스크린 UI 아님)

Combat HUD가 직접 참조하는 데이터는 없음(체력바 등 화면 UI는 Health/Damage Core의 UI Requirements 참조).

## Acceptance Criteria

1. **GIVEN** Grunt 개체가 Idle 상태, **WHEN** 플레이어가 SightRange(1200uu)+SightAngle(90°) 원뿔 안에 들어옴, **THEN** Chase 상태로 전환.
2. **GIVEN** Grunt 개체가 Idle 상태, Sight 밖, **WHEN** NoiseLoudness=1.0 소음이 BaseHearingRadius(800uu) 안에서 발생, **THEN** Alert(Investigate)로 전환, 소음 발생 지점으로 이동.
3. **GIVEN** Alert 상태에서 5초(AlertToIdleTimeout) 동안 Sight 미확보, **WHEN** 타이머 만료, **THEN** Idle로 복귀.
4. **GIVEN** 적이 Attack 상태로 윈드업 애니메이션 재생 중, **WHEN** `OnAttackTelegraphed` AnimNotify 발생, **THEN** 해당 이벤트가 정확히 1회 델리게이트 브로드캐스트됨.
5. **GIVEN** 윈드업 시작 후 0.45초 뒤 `OnAttackCommitted` AnimNotify 발생(TelegraphWindow=0.45초 ≥ MinTelegraphWindow 0.4초), **WHEN** 판정 프레임 도달, **THEN** 데미지 GameplayEffect가 정확히 이 시점에만 적용됨(윈드업 중엔 데미지 없음).
6. **GIVEN** `OnAttackTelegraphed` 이후 커밋 전, **WHEN** `TriggerStagger()` 호출, **THEN** 공격 캔슬, `OnAttackCommitted` 미발행, 데미지 미적용.
7. **GIVEN** `OnAttackTelegraphed` 이후 커밋 전, **WHEN** Health/Damage Core `OnDeath` 이벤트 수신, **THEN** 즉시 Dead 전환, `OnAttackCommitted` 미발행.
8. **GIVEN** Ranged 개체, PlayerDistance=350uu(MinEngageRange 400uu 미만), **WHEN** 상태 평가, **THEN** ShouldRetreat=true로 후퇴 이동 시작.
9. **GIVEN** Ranged 개체가 후퇴 시도 중 NavMesh 경로 탐색 2초(RetreatPathfindFailTimeout) 이상 실패, **WHEN** 타이머 만료, **THEN** 거리 밴드 무시하고 제자리에서 공격 허용.
10. **GIVEN** Grunt Health=30, **WHEN** RawDamage=25 데미지 2회 연속 적용(Health/Damage Core 경유), **THEN** 1회차 Health=5(생존), 2회차 `OnDeath` 발행 후 Dead 전환.
11. **GIVEN** 개체가 Dead 상태, **WHEN** CorpseHoldDuration(4초) 경과, **THEN** 디스폰(액터 제거), 이후 충돌/인지 판정 완전 정지.
12. **GIVEN** 개체에 `State.Executable` 태그 존재, **WHEN** 정상 상태머신 틱 발생(Chase/Attack 등), **THEN** 상태 전환이 태그와 무관하게 그대로 진행됨(오버레이 확인).

## Open Questions

- **`MinTelegraphWindow`(0.4초) 가정값 미검증** — Dash/Evasion의 실제 저스트회피 입력 버퍼 창과 대조되지 않음. Owner: Dash/Evasion 설계 시 재검증.
- **아키타입 확장 여부** — MVP는 Grunt/Ranged 2종. 강습형/폭발형 등 추가 아키타입은 이 리비전 범위 밖, 필요 시 후속 리비전.
- **MaxHealth 값(Grunt 30 / Ranged 20)은 밸런스 패스 없이 잠정 설정** — solo mode로 systems-designer 미상담. 프로토타입 플레이테스트 후 조정 필요.
- **대규모 스웜 퍼포먼스 미검증** — 환경 연쇄파괴(챕터 1/2)에서 "수십 마리 동시 몰림" 시나리오의 AIPerception/BT tick 비용이 측정되지 않음. Arena Morphing 스파이크와 유사한 별도 perf 테스트가 Production 진입 전 필요할 수 있음.
- **엔진 문서 갭** — `docs/engine-reference/unreal/modules/navigation.md`가 UE5.7 기준으로만 verified, 프로젝트 핀 버전(5.8) 재검증 안 됨. 구현 시 담당 프로그래머가 실제 5.8 헤더로 대조 확인 필수.
