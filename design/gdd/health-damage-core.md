# Health/Damage Core

> **Status**: Approved
> **Author**: user + game-designer (solo review mode)
> **Last Updated**: 2026-07-16
> **Implements Pillar**: Dopamine Driven Design — foundation for high-risk/high-return damage moments (Core Extraction, Supernova, Overdrive)

## Overview

Health/Damage Core는 이 게임의 모든 피해/사망 처리를 관장하는 공용 기반 시스템이다. 데이터/인프라 관점에서는 GAS(`AbilitySystemComponent` + `AttributeSet`)를 통해 Health/MaxHealth 어트리뷰트를 관리하고, `GameplayEffect`로 데미지/회복/방어무시(true damage)를 적용하는 단일 진입점 역할을 한다. 하지만 이 시스템은 절대 배경 인프라로 느껴지면 안 된다 — F키 코어 적출의 즉사 처형, 블랙홀+화염 초신성의 방어력 무시 광역뎀, 루나 오버드라이브 중 화면을 가득 채우는 학살까지, 콘셉트 문서의 모든 "하이리스크-하이리턴" 카타르시스 순간은 결국 이 시스템이 얼마나 즉각적이고 명확하게 데미지를 처리하고 죽음을 선언하느냐에 달려 있다. Player Movement가 "손맛"의 뼈대였다면, Health/Damage Core는 "타격감과 처형의 확실함"의 뼈대다. 이 시스템 없이는 어떤 전투도, 처형도, 붕괴도 실제로 끝나지 않는다.

## Player Fantasy

플레이어는 자신이 주는 데미지가 "확실하다"고 느껴야 한다 — 때리면 맞았다는 게 애매하지 않고, 죽어야 할 적은 주저 없이 죽는다. 코어 적출(F키)은 그 확실함의 절정: 근접 조건이 맞으면 카메라가 클로즈업되고 적은 반드시 즉사한다 — "혹시 실패했나?" 하는 불확실성이 존재해선 안 된다. 초신성/플라즈마 스톰 같은 방어무시뎀도 마찬가지로 "이 데미지는 무조건 관통한다"는 확신을 줘야 하며, 루나 오버드라이브 중에는 화면을 가득 채우는 학살이 끊김없이 처리되어 처리 지연이나 판정 씹힘이 카타르시스를 깨지 않아야 한다.

**Feel Reference**: *DOOM* (2016/Eternal)의 Glory Kill — 처치가 즉각적이고 확실하며, 즉시 자원(체력/마나) 보상으로 이어지는 루프. *Devil May Cry*류의 과장되지만 명확한 처형 판정.
**Anti-reference**: Souls류의 느리고 모호한 사망 판정("맞았는지 애매함", 지연된 사망 애니메이션)은 지향하지 않음 — Endless Catharsis 목표(즉각적 도파민)와 정면 충돌.

> Solo mode — `creative-director` 미상담. Production 전 수동 확인 필요.

## Detailed Design

### Core Rules

1. **모든 데미지는 단일 진입점을 통과한다** — 스펠, 근접공격, 환경 연쇄파괴(다리 붕괴, 낙사 유도), DoT까지 예외 없이 하나의 `ApplyDamage` 경로(GAS `GameplayEffect` 적용)로 들어온다. 소스가 무엇이든 이 시스템은 "어떤 수단으로 맞았는가"를 구분하지 않고 "얼마나 맞았는가"만 처리한다.
2. **방어 스탯은 이 MVP 범위에 존재하지 않는다** — Defense/Armor 감쇄 공식은 정의하지 않음. 데미지는 원값 그대로 Health에 적용되거나, 실드/아머 레이어가 있으면 그 레이어가 전부 흡수한다(불리언 인터셉트, 부분감쇄 아님). 부분 방어 감쇄가 필요해지면 후속 리비전 대상.
3. **실드/아머 인터셉트 훅** — 데미지가 Health 어트리뷰트에 도달하기 *전에*, 대상에게 활성 실드/아머 레이어가 있는지 검사하는 지점을 이 시스템이 소유한다(GAS `GameplayEffect ExecutionCalculation` 단계). 실드 자체의 HP/파괴 조건/파괴 시 이벤트는 **Enemy Elite Shield GDD 소유** — 이 문서는 "인터셉트 지점이 존재해야 한다"는 계약만 소유.
4. **방어무시(true damage) 플래그** — `bBypassDefense`가 설정된 GameplayEffect(초신성, 코어 적출 등)는 Rule 3의 실드/아머 인터셉트를 완전히 건너뛰고 Health에 직접 적용된다. 방어무시는 예외가 아니라 이 시스템이 1급으로 지원하는 데미지 타입.
5. **사망 판정은 즉시, 애매함 없음** — `Health <= 0`이 되는 프레임에 즉시 Death 상태로 전환(Player Fantasy의 "확실함" 계약). 지연된 사망 애니메이션으로 판정을 늦추는 구현은 금지 — 사망 확정과 사망 프레젠테이션(모션/이펙트)은 분리되며, 프레젠테이션이 판정을 지연시키지 않는다(Player Movement Core Rule 9의 히트스탑 원칙과 동일 철학: 게임플레이 판정과 비주얼은 분리).
6. **플레이어 사망 = 즉시 결투지(checkpoint) 재시작** — 정확한 체크포인트 저장/복귀 메커니즘은 아직 존재하지 않는 Persistence 시스템 소유로 전제(가정, Open Questions 등록). 이 문서는 "즉시 재시작, 게임오버 화면으로 막지 않음"이라는 방향성 계약만 소유.
7. **적 사망**은 이 시스템이 `Health<=0` 판정과 사망 이벤트(`OnDeath`) 발행까지만 소유 — 루트/드롭/AI 클린업/시체 처리는 Enemy AI (base) GDD 소유.
8. **무적(i-frame) 게이트는 태그 기반 차단** — `State.Invulnerable` GameplayTag가 대상에 존재하면, 이 시스템은 들어오는 모든 데미지 GameplayEffect를 적용 자체를 막는다(Health를 0으로 클램프하는 바닥값 트릭이 아니라 `ApplicationRequirement`에서 원천 차단). 이 태그를 **누가** 부여하는지는 이 문서가 정의하지 않음 — Dash/Evasion(저스트회피 프레임), Status Effect, Core Extraction Execution(처형 연출 중 대상 무적) 등 소비자가 각자 소유. 이 문서는 태그 이름과 "존재하면 100% 차단"이라는 계약만 소유.
9. **처형 가능(Executable) 상태는 태그 기반, 접근권만 이 문서 소유** — `State.Executable` GameplayTag와 `TryExecute(Target)` API를 이 시스템이 정의한다. 이 태그를 **부여할 수 있는 것은 Dash/Evasion(저스트회피 성공)과 Enemy Elite Shield(실드 파괴)뿐** — Core Extraction Execution은 태그를 소비(읽기+근접 판정 후 `TryExecute` 호출)만 할 수 있다. `TryExecute` 성공 시: Rule 4와 동일하게 방어무시로 즉시 Health=0, 태그 제거, `OnExecuted` 이벤트 발행(마나 100% 회복은 이 이벤트를 구독하는 Spell Casting/Mana 시스템 소유 — 이 문서는 회복량을 정의하지 않음).
10. **HP % 임계값 이벤트를 노출한다** — `OnHealthPercentCrossed(threshold)` 이벤트를 지원(예: 50%, 25% 통과 시 발행). Boss Phase(미설계)가 페이즈 전환 트리거로 이 이벤트에 의존할 것이 확실시되므로, Foundation 레이어에서 인터페이스만 먼저 열어둠 — 정확한 임계값 목록은 Boss Phase GDD가 소유.

### States and Transitions

| State/Tag | Entry Condition | Exit Condition | Behavior |
|-----------|-----------------|-----------------|----------|
| Alive | 기본 상태 | Health ≤ 0 | 정상 데미지/회복 수신 |
| `State.Invulnerable` (오버레이, Alive와 동시 가능) | 소비 시스템(Dash/Evasion 등)이 태그 부여 | 소비 시스템이 태그 제거 | 모든 들어오는 데미지 GameplayEffect 적용 차단 (Rule 8) |
| `State.Executable` (오버레이, Alive와 동시 가능) | Dash/Evasion 저스트회피 성공 또는 Enemy Elite Shield 실드파괴 시 부여 | `TryExecute` 성공(→Death) 또는 소비 시스템이 직접 제거(윈도우 만료 등) | F키 상호작용 프롬프트 표시 대상(프롬프트 UI 자체는 Combat HUD 소유), `TryExecute` 호출 가능 |
| Death | Health ≤ 0 (일반 데미지) 또는 `TryExecute` 성공(방어무시 즉사) | 플레이어: 즉시 재시작(Rule 6) / 적: Enemy AI 클린업 | `OnDeath` 이벤트 발행, 이후 데미지 수신 정지 |

### Interactions with Other Systems

- **Enemy AI (base)** (하류): `OnDeath` 이벤트를 구독해 사망 처리(루트/클린업/AI 상태 정리)를 트리거. 이 시스템은 사망 판정까지만 소유.
- **Spell Casting (base)** (하류): 모든 스펠 데미지는 Rule 1의 단일 진입점(GameplayEffect)을 통해 적용. 방어무시가 필요한 스펠(초신성 등)은 Rule 4의 `bBypassDefense` 사용.
- **Status Effect (미설계, 하류)**: DoT/버프/디버프는 이 시스템의 GameplayEffect Duration/Periodic 적용 경로를 재사용 — 구체 독/화상 등 정의는 Status Effect GDD 소유(사용자 결정: 훅만 열어둠). `State.Invulnerable` 부여 권한도 Status Effect가 스턴/CC 면역 등에 활용 가능.
- **Dash/Evasion (미설계, 하류)**: 저스트회피 성공 시 `State.Invulnerable`(회피 프레임) 및 `State.Executable`(처형 개시) 태그 부여 권한을 가짐.
- **Enemy Elite Shield (미설계, 하류)**: 자체 실드 HP/파괴 로직 소유, 파괴 시 Rule 3의 인터셉트 레이어 해제 + `State.Executable` 태그 부여.
- **Super Armor / CC Interrupt (미설계, 하류)**: Rule 3의 인터셉트 지점을 CC 면역(경직 무효화)에도 재사용할 것으로 예상 — 구체 조건은 해당 GDD 소유.
- **Core Extraction Execution (미설계, 하류)**: `State.Executable` 태그 소비 + `TryExecute` API 호출자. 근접 판정/카메라 연출은 해당 GDD 소유.
- **Combo/Tension Gauge (하류)**: 데미지 적용 이벤트(추후 `OnDamageApplied`류)를 구독해 콤보 누적에 활용할 것으로 예상 — 무엇이 "콤보 가치가 있는 히트"인지 판단은 해당 GDD 소유.
- **Boss Phase (미설계, 하류)**: Rule 10의 `OnHealthPercentCrossed` 이벤트를 페이즈 전환 트리거로 소비할 것으로 예상.
- **Luna Overdrive (하류)**: 플레이어 Death 상태(`OnDeath`)를 구독해 각성 강제 종료 트리거로 사용 — 이 문서는 이벤트 발행까지만 소유. (2026-07-17 Luna Overdrive 설계 시 양방향 의존 명기 목적으로 추가된 항목 — 계약 변경 없음.)
- **Environmental Chain-Destruction (미설계, 하류)**: 환경 데미지(낙사, 붕괴)도 Rule 1의 동일 진입점을 통해 적용 — 별도 경로 불필요.

## Formulas

### Default Max Health

| Variable | Type | Range | Source | Description |
|----------|------|-------|--------|-------------|
| MaxHealth (Player) | float | 튜닝값, 기본 **100**, Safe Range 80–150 | Tuning Knob | 플레이어 최대 체력 |
| MaxHealth (Enemy, per-archetype) | float | 아키타입별 상이 | Enemy AI GDD 소유 (이 시스템은 어트리뷰트 슬롯만 제공) | 몬스터별 최대 체력 |

**Output Range**: 플레이어 80–150 (Safe Range). 몬스터는 이 문서가 값을 정의하지 않음 — Enemy AI GDD가 어트리뷰트를 채움.
**Example**: 기본 튜닝(100) 기준, Health는 로드 시 100/100으로 초기화.

### Effective Damage Applied

```
EffectiveDamageApplied = IsInvulnerable ? 0
                        : (HasActiveShield AND NOT bBypassDefense) ? 0
                        : RawDamage
```

| Variable | Symbol | Type | Range | Description |
|----------|--------|------|-------|-------------|
| RawDamage | — | float | 0 이상, 소스(스펠/근접/환경) 정의 | 데미지 소스가 요청한 원본 데미지량 |
| IsInvulnerable | — | bool | `State.Invulnerable` 태그 존재 여부 | Core Rule 8 |
| HasActiveShield | — | bool | Enemy Elite Shield GDD 소유 상태 | Core Rule 3 |
| bBypassDefense | — | bool | 이펙트별 플래그 | Core Rule 4 |

**Output Range**: 0 또는 RawDamage — 이 시스템에는 부분감쇄가 없으므로 값은 항상 이 두 값 중 하나(Core Rule 2).
**Example**: RawDamage=25, IsInvulnerable=false, HasActiveShield=false → EffectiveDamageApplied=25. 같은 조건에서 HasActiveShield=true, bBypassDefense=false → 0(실드가 전량 흡수, 흡수 자체의 실드 HP 차감은 Elite Shield GDD 소유).

### New Health

```
NewHealth = clamp(CurrentHealth - EffectiveDamageApplied, 0, MaxHealth)
```

| Variable | Type | Range | Description |
|----------|------|-------|-------------|
| CurrentHealth | float | 0–MaxHealth | 적용 전 현재 체력 |
| EffectiveDamageApplied | float | 위 공식 결과 | — |
| MaxHealth | float | Tuning Knob | 회복 시 상한 클램프에도 동일 사용 |

**Output Range**: 0–MaxHealth. 0 도달 시 Core Rule 5(즉시 Death) 트리거.
**Example**: CurrentHealth=40, EffectiveDamageApplied=25 → NewHealth=15 (Death 아님). CurrentHealth=20, EffectiveDamageApplied=25 → NewHealth=0 (클램프, Death 즉시 트리거).

> 자연 회복(regen) 없음 — 회복은 전량 아이템/스펠/코어 적출 등 명시적 소스에서만 발생(사용자 결정).

## Edge Cases

| Scenario | Expected Behavior | Rationale |
|----------|-------------------|-----------|
| `RawDamage`가 음수/malformed 이펙트로 전달됨 | 즉시 0으로 하드 클램프, 경고 로그 출력(회복 이펙트로 오인/역작용 방지) | 음수 데미지가 그대로 적용되면 회복처럼 작동해 Rule 5의 "확실한 사망"과 모순 |
| 동일 프레임에 다중 히트로 `Health`가 이미 0 이하인 대상에게 추가 데미지 도달(예: 광역기 다단히트) | 이미 Death 상태인 대상은 이후 데미지를 완전 무시 — `OnDeath`는 정확히 1회만 발행 | 중복 사망 이벤트가 Enemy AI/루트 시스템에서 중복 처리(이중 드롭 등)를 유발하는 것을 방지 |
| `bBypassDefense=true`인 데미지가 `State.Invulnerable` 대상에게 도달 | 여전히 100% 차단됨 — 방어무시는 Rule 3(실드/아머)만 우회하고 Rule 8(무적)은 우회하지 않음 | "방어무시"와 "무적 무시"는 별개 계약. 보스 페이즈 전환 무적 등이 초신성 방어무시뎀으로 뚫리면 판타지·밸런스 모두 붕괴 |
| `TryExecute`가 `State.Executable` 태그 없는 대상에게 호출됨(예: 프롬프트 사라진 후 늦은 입력) | 아무 효과 없이 실패 반환 — 사이드이펙트 없음(데미지 없음, 이벤트 없음) | 처형 API 오남용/타이밍 어긋난 호출이 조용히 무시되어야 판정 불확실성이 생기지 않음(Player Fantasy 계약) |
| `State.Executable` 태그가 부여된 대상이 처형 전에 일반 데미지로 먼저 사망 | 태그는 자동 소멸(Death 전환 시 모든 오버레이 태그 클리어), 마나 100% 회복 등 처형 전용 보상은 발생하지 않음 | 처형 보상은 `TryExecute` 성공 경로에만 연결됨 — 일반 킬과 처형킬은 결과(사망)는 같지만 보상 트리거는 다름 |
| 두 소비 시스템이 같은 대상에게 동시에 `State.Invulnerable`(또는 `State.Executable`)을 부여/해제 시도(예: Dash/Evasion과 Status Effect가 겹침) | 태그는 참조 카운트 기반으로 적용(`AddLooseGameplayTag` 카운트 방식) — 한 시스템이 제거해도 다른 시스템이 아직 부여 중이면 태그 유지 | 단순 불리언이면 한 시스템의 해제가 다른 시스템이 걸어둔 무적/처형가능 상태를 조기에 꺼버리는 경합 버그가 발생 |
| 런타임에 `MaxHealth`가 변경됨(버프/디버프로 상한 자체가 변동) | `CurrentHealth`는 절대값 그대로 유지, 새 `MaxHealth`로 상한만 재클램프(비례 스케일링 없음). 새 `MaxHealth`가 `CurrentHealth`보다 낮아지면 즉시 새 상한으로 하향 클램프(단, 이 클램프 자체는 Death를 트리거하지 않음 — 0 초과라면 생존) | 비례 스케일링은 버프 시점의 체력비율에 따라 결과가 직관과 다르게 튈 수 있어(예: 1/100일 때 MaxHealth 2배 → 겨우 2) 절대값 유지가 더 예측 가능 |

## Dependencies

| System | Direction | Nature of Dependency |
|--------|-----------|----------------------|
| Enemy AI (base) | Enemy AI가 이 시스템에 의존 | `OnDeath` 이벤트 구독, Health 어트리뷰트 슬롯 사용 |
| Spell Casting (base) | Spell Casting이 이 시스템에 의존 | 모든 스펠 데미지가 Rule 1 단일 진입점을 통해 적용 |
| Status Effect (미설계) | Status Effect가 이 시스템에 의존 | GameplayEffect Duration/Periodic 경로 재사용(DoT), `State.Invulnerable` 부여 권한 활용 |
| Dash/Evasion (미설계) | Dash/Evasion이 이 시스템에 의존 | `State.Invulnerable`(회피 프레임)/`State.Executable`(처형 개시) 태그 부여 권한 |
| Enemy Elite Shield (미설계) | Enemy Elite Shield가 이 시스템에 의존 | Rule 3 실드 인터셉트 지점 사용, 파괴 시 `State.Executable` 부여 |
| Super Armor / CC Interrupt (미설계) | 이 시스템에 의존 | Rule 3 인터셉트 지점을 CC 면역에 재사용 예상 |
| Core Extraction Execution (미설계) | 이 시스템에 의존 | `State.Executable` 태그 소비, `TryExecute` API 호출 |
| Combo/Tension Gauge | 이 시스템에 의존 | 데미지 적용 이벤트를 콤보 누적 트리거로 소비 예상 |
| Boss Phase (미설계) | 이 시스템에 의존 | `OnHealthPercentCrossed` 이벤트를 페이즈 전환 트리거로 소비 |
| Luna Overdrive | 이 시스템에 의존 | 플레이어 Death 상태(`OnDeath`) 구독 — 각성 강제 종료 트리거 (2026-07-17 추가) |
| Environmental Chain-Destruction (미설계) | 이 시스템에 의존 | 환경 데미지도 Rule 1 동일 진입점 사용 |

> 이 시스템은 어떤 것에도 의존하지 않음 (Foundation Layer, systems-index 일치).

## Tuning Knobs

| Parameter | Current Value | Safe Range | Effect of Increase | Effect of Decrease |
|-----------|---------------|------------|--------------------|--------------------|
| MaxHealth (Player) | 100 | 80–150 | 실수 여지 늘어남, 처형/방어무시뎀의 "위기감" 희석 위험 | 너무 낮으면 하이리스크가 아니라 즉사 러시로 변질, Endless Catharsis 목표(지속 전투)와 충돌 |

> 이 시스템에는 방어/저항/회복률 튜닝 노브가 없음(Core Rule 2 — 방어 스탯 자체가 미존재, 회복은 자연회복 없음, Formulas 참조). 실드/아머 관련 노브는 Enemy Elite Shield GDD 소유.

## Visual/Audio Requirements

| Event | Visual Feedback | Audio Feedback | Priority |
|-------|-----------------|-----------------|----------|
| 데미지 수신 | 적/플레이어 머티리얼 히트 플래시(짧은 화이트/컬러 오버레이) + 데미지 넘버 팝업(방어무시는 다른 색상으로 구분) | 히트 SFX(데미지 종류별 상이 — 소스 GDD 소유) | High |
| 플레이어 저체력(HP % 임계값 이하) | 화면 가장자리 붉은 비네트(Rule 10 이벤트 활용) | 저체력 심장박동 루프 SFX | Medium |
| 사망(일반) | 사망 모션/파티클(구체 연출은 각 시스템 소유 — 이 문서는 트리거 지점만) | 사망 SFX | High |
| 처형 성공(`TryExecute`) | 이 문서는 트리거만 소유 — 실제 클로즈업/파티클은 Core Extraction Execution GDD 소유 | 〃 | — |

> Solo mode — `art-director` 미상담. Production 전 수동 확인 필요.

## UI Requirements

이 문서는 UI 레이아웃/스타일을 소유하지 않음(Combat HUD GDD 소유) — 다음 인터페이스 데이터만 노출한다:

- `Health` / `MaxHealth` 어트리뷰트 값 및 변경 델리게이트(Combat HUD의 체력바 바인딩용)
- `OnHealthPercentCrossed(threshold)` 이벤트(저체력 비네트/경고 트리거용, Rule 10)
- `State.Executable` 태그 존재 여부(F키 상호작용 프롬프트 표시 트리거용) — 프롬프트의 실제 배치/스타일은 Combat HUD 소유

> **📌 UX Flag**: Combat HUD가 이 데이터를 소비하는 화면이므로, Production 진입 전 `/ux-design`으로 Combat HUD 화면의 UX 스펙(`design/ux/combat-hud.md`)을 작성할 때 위 3개 인터페이스를 명시적으로 참조할 것.

## Acceptance Criteria

1. **GIVEN** 대상 Health=100, MaxHealth=100, **WHEN** RawDamage=25 데미지 이펙트 적용, **THEN** NewHealth=75, `OnDeath` 미발행.
2. **GIVEN** 대상에 `HasActiveShield=true`인 실드가 있고 `bBypassDefense=false`, **WHEN** RawDamage=25 데미지 도달, **THEN** Health 변화 없음(EffectiveDamageApplied=0).
3. **GIVEN** 대상에 실드가 있고 `bBypassDefense=true`(예: 초신성), **WHEN** RawDamage=50 데미지 도달, **THEN** 실드 무시하고 Health -50 적용.
4. **GIVEN** 대상 Health=10, **WHEN** RawDamage=999 데미지 도달, **THEN** NewHealth=0(클램프), `OnDeath` 정확히 1회 발행.
5. **GIVEN** 플레이어 Health가 0에 도달, **WHEN** Death 상태 전환, **THEN** 게임오버 화면 없이 즉시 결투지 재시작 트리거(체크포인트 시스템 인터페이스 호출).
6. **GIVEN** 대상에 `State.Invulnerable` 태그 존재, **WHEN** `bBypassDefense=true` 데미지 포함 임의 데미지 도달, **THEN** Health 변화 전혀 없음(예외 없음).
7. **GIVEN** 대상에 `State.Executable` 태그 존재 및 근접 조건 충족, **WHEN** `TryExecute` 호출, **THEN** Health 즉시 0, `OnExecuted` 이벤트 1회 발행, 실드 존재 여부와 무관하게 성공.
8. **GIVEN** 대상에 `State.Executable` 태그 없음, **WHEN** `TryExecute` 호출, **THEN** 반환값 실패(false), Health/이벤트 변화 없음.
9. **GIVEN** 대상 Health가 60%→45%로 하락(50% 임계값 통과), **WHEN** 데미지 적용 프레임, **THEN** `OnHealthPercentCrossed(0.5)` 정확히 1회 발행.
10. **GIVEN** 두 시스템(A, B)이 동일 대상에 `State.Invulnerable`을 각각 참조카운트로 부여, **WHEN** A만 태그 제거, **THEN** 태그 여전히 존재(B의 참조가 남아있으므로 데미지 계속 차단됨).
11. **GIVEN** 데미지 이펙트의 `RawDamage=-10`(malformed), **WHEN** 적용 시도, **THEN** 0으로 클램프되어 적용, 경고 로그 1건 발생, Health 변화 없음.

## Open Questions

- **체크포인트/재시작 메커니즘 미정** — Rule 6은 "즉시 재시작"이라는 방향만 계약, 실제 저장 위치/체크포인트 정의는 아직 없는 Persistence 시스템 소유로 가정. Persistence GDD 작성 시 이 계약과 맞는지 재검증 필요.
- **몬스터별 MaxHealth 값 미정** — 이 문서는 어트리뷰트 슬롯만 제공, 실제 수치는 Enemy AI (base) GDD에서 아키타입별로 정의될 예정.
- **UE5.8 GAS 어트리뷰트 초기화 패턴 재검증 필요** — `deprecated-apis.md`가 "레거시 GAS 어트리뷰트 세트 초기화 함수" deprecated를 언급하나 신규 패턴 자체는 문서화되어 있지 않음. 구현 시 `ue-gas-specialist`가 실제 UE5.8 GAS 헤더로 대조 확인 필수(엔진 버전이 두 모델 학습 데이터보다 최신이라 환각 위험 높음).
- **참조카운트 태그 구현 세부** — `AddLooseGameplayTag`류 카운트 방식 사용을 Edge Cases에서 계약으로 명시했으나, 정확한 구현(예: 커스텀 wrapper 필요 여부)은 구현 시 `ue-gas-specialist` 판단 영역.
