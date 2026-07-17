# Luna Overdrive (Blood Moon)

> **Status**: Designed (pending review)
> **Author**: user + game-designer (solo design mode)
> **Last Updated**: 2026-07-17
> **Implements Pillar**: Dopamine Driven Design — 텐션 곡선의 최고점(각성 발동 → 무제한의 학살 → 급격한 하락)

## Overview

Luna Overdrive는 Combo/Tension Gauge가 Max 도달 시 발행하는 `OnOverdriveTriggered` 이벤트의 **소비자**로서, 게임 환경을 '피의 달(Blood Moon)' 상태로 전환시키는 각성 시스템이다. 발동 즉시 Spell Casting (base)의 `CostBypass.Active` GameplayTag를 플레이어에게 부여해 지속시간(기본 10초) 동안 마나 무한 + 전 스펠 쿨타임 제로를 실현하고, 만료 시 태그를 해제해 일상 전투로 복귀시킨다. 이 시스템은 "발동된 이후 무엇이 일어나는가"만 소유한다 — 발동 임계값/게이지 축적은 Combo/Tension Gauge 소유이고, 크림슨 레드 화면 반전의 본격 연출은 Overdrive Visual State(#18, 미설계) 소유이며, 이 문서는 각성 상태의 시작/지속/종료라는 상태 기계와 그것을 알리는 이벤트 계약을 소유한다. 태그 하나의 On/Off 타이밍이 곧 오버드라이브 지속시간의 실제 구현이다.

## Player Fantasy

게이지를 꽉 채운 순간, 세계가 붉게 뒤집히며 "이제부터 10초간 아무것도 아낄 필요 없다"는 완전한 해방이 온다. 마나 잔량도, 쿨다운도 보지 않고 화면을 가득 채우는 마법을 난사하는 무제한의 학살 — 그 직전까지 "블랙홀을 아껴야 하나"를 저울질하던 긴장(Spell Casting Player Fantasy의 availability 게이트)이 한꺼번에 풀리는 대비가 이 시스템의 정체성이다. 그리고 10초가 끝나는 순간의 급격한 하락(게이지 0, 정상 코스트 복귀)까지가 텐션 곡선의 완성이다 — 각성은 영원하지 않아서 달콤하다.

**Feel Reference**: *DOOM Eternal*의 Crucible/버서크 파워업(짧고 확실한 전능감), *Devil May Cry 5* 진마인(Sin Devil Trigger)의 "쌓아서 터트리는" 해방감.
**Anti-reference**: 지속시간이 긴 소극적 버프(딜 증가 몇 %류) — 수치 강화가 아니라 **규칙 자체가 바뀌는**(코스트/쿨다운 소멸) 체험이어야 함.

> **Note**: `creative-director` 미컨설트 — Solo 모드. Production 전 수동 리뷰 필요.

## Detailed Design

### Core Rules

1. **트리거는 단일 소비 계약** — Combo/Tension Gauge의 `OnOverdriveTriggered` 이벤트를 구독하고, 수신 프레임에 즉시 Overdrive Active 상태로 진입한다. **페이로드 결정(상류 Open Question 해소)**: 페이로드 없음(무인자 이벤트)으로 확정 — 각성 효과가 전부 플레이어 중심(태그 부여 + 타이머)이고, 발동 연출에 필요한 위치/시점 정보는 수신 시점에 플레이어 참조로 직접 조회 가능하므로 추가 데이터가 불필요하다(가장 단순한 계약 선택, combo-tension-gauge.md Open Questions에 해소 반영).
2. **발동 효과 = `CostBypass.Active` 태그 부여** — Spell Casting (base) Core Rule 10의 훅을 그대로 소비한다. 이 단일 태그가 마나비용과 쿨다운을 **동시에** 우회한다(부분 우회 모드 없음 — spell-casting-base.md Edge Cases에서 MVP 확정). MVP 범위에서 오버드라이브의 게임플레이 효과는 이 태그가 전부다 — 별도 데미지 배율/이동속도 버프 없음(컨셉 문서에 명시된 효과는 "마나 무한 + 쿨타임 제로 + 시각 반전"뿐이므로 가장 보수적으로 그 범위만 구현).
3. **지속시간은 타이머 기반** — 발동 시점부터 `OverdriveDuration`(Tuning Knob, 기본 10초 — game-concept.md 명시값) 카운트다운. 만료 프레임에 종료 처리.
4. **종료 = 태그 해제, 순서 계약 준수** — 종료 프레임에 `CostBypass.Active`를 해제하며, **태그 상태 변경이 그 프레임의 캐스트 게이트 평가보다 먼저 해소된다**(spell-casting-base.md Edge Cases의 bypass-release same-frame race 순서 계약을 그대로 채택 — 이 문서가 따라야 한다고 상류가 명시한 유일한 순서). 종료 프레임에 입력된 캐스트는 정상 코스트/쿨다운 검사를 받는다.
5. **Active 중 재트리거 = 지속시간 리프레시** — 오버드라이브 중 게이지가 재차 Max에 도달해 `OnOverdriveTriggered`가 다시 오면(combo-tension-gauge.md Core Rule 6이 명시적으로 허용), 타이머를 `OverdriveDuration` 풀로 리셋한다. 태그는 이미 부여된 것을 유지(재부여 없음 — 단일 grant를 유지해 참조카운트 오염/이중해제 경합을 원천 차단). 스택/연장가산 없음 — 리프레시가 상류의 "재발동 허용" 계약과 "무제한의 학살" 판타지에 부합하는 가장 단순한 해석.
6. **플레이어 사망 시 즉시 강제 종료** — Health/Damage Core의 플레이어 Death 상태 진입 시 타이머 취소 + 태그 즉시 해제. 재시작 후 오버드라이브 이월 없음(Combo/Tension Gauge의 사망 시 게이지 0 리셋과 일관).
7. **발동 임계값을 재설계하지 않는다** — 게이지 Max(100), 축적 공식, 감쇠는 전부 Combo/Tension Gauge 소유(systems-index.md High-Risk 표의 소유 경계). 이 문서는 트리거 수신 이후만 소유.
8. **상태 이벤트를 하류에 노출** — `OnOverdriveStarted`(발동 프레임), `OnOverdriveEnded(EndReason)`(종료 프레임, EndReason ∈ {Expired, PlayerDeath}) 이벤트와 잔여시간 조회 인터페이스(`OverdriveTimeRemaining`)를 노출한다. Combat HUD(각성 표시/잔여시간)와 Overdrive Visual State(크림슨 레드 연출 시작/해제)가 소비.
9. **시각 반전의 트리거만 소유** — '푸른색 → 크림슨 레드' 전환의 본격 연출(라이팅/포스트프로세스/이펙트 팔레트 교체)은 Overdrive Visual State(#18, Vertical Slice, 미설계) 소유. 이 문서는 Rule 8의 시작/종료 이벤트가 그 연출의 유일한 트리거라는 계약과, MVP용 최소 피드백(Visual/Audio Requirements)만 소유.
10. **거대 파편 흡수 트리거는 MVP 범위 밖** — game-concept.md의 "일정 콤보 게이지 도달 **또는 거대 파편 흡수** 시" 중 후자는 소유 시스템이 아직 없어(파편 드랍/흡수 시스템 미설계) 이번 범위에서 제외. 발동 경로가 추가되어도 이 문서의 "트리거 수신 → Active" 계약은 재사용 가능(Open Questions 등록).

### States and Transitions

| State | Entry Condition | Exit Condition | Behavior |
|---|---|---|---|
| Inactive | 기본 상태 / 종료 처리 완료 | `OnOverdriveTriggered` 수신 | 태그 없음, 정상 코스트/쿨다운 |
| Active | `OnOverdriveTriggered` 수신 프레임 | 타이머 만료(→Expired) 또는 플레이어 Death(→PlayerDeath) | `CostBypass.Active` 유지, 잔여시간 카운트다운, 재트리거 시 타이머 리프레시 |

전이 시 이벤트: Inactive→Active에서 `OnOverdriveStarted` 1회, Active→Inactive에서 `OnOverdriveEnded(EndReason)` 1회 — 각 전이당 정확히 1회 발행(Health/Damage Core `OnDeath` 1회 발행 패턴과 동일 원칙). 리프레시(Active 유지)는 전이가 아니므로 Started 재발행 없음.

### Interactions with Other Systems

- **Combo/Tension Gauge** (상류, 하드) — `OnOverdriveTriggered` 구독(무페이로드, Rule 1). 게이지 리셋/재누적은 상류 소유 — 이 문서는 관여하지 않음.
- **Spell Casting (base)** (상류, 하드) — `CostBypass.Active` 태그 부여/해제(Rule 2/4). 태그의 동작 의미("켜지면 코스트/쿨다운 검사를 완전히 건너뛴다")는 상류 소유, On/Off 타이밍은 이 문서 소유.
- **Health/Damage Core** (상류, 소프트) — 플레이어 Death 상태(`OnDeath`) 구독(Rule 6, 강제 종료).
- **Combat HUD** (하류, 소프트) — Rule 8의 상태 이벤트 + 잔여시간 read-only 소비. 없어도 오버드라이브 로직 자체는 정상 동작.
- **Overdrive Visual State** (하류, 미설계) — Rule 8/9의 시작/종료 이벤트를 연출 트리거로 소비 예정. 연출 내용 전부 그쪽 GDD 소유.

## Formulas

### Overdrive End Time (발동/리프레시)

`OverdriveEndTime = CurrentTime + OverdriveDuration`

**Variables:**
| Variable | Symbol | Type | Range | Description |
|----------|--------|------|-------|-------------|
| CurrentTime | — | float (s) | 런타임 | 발동 또는 리프레시가 처리된 시각 |
| OverdriveDuration | — | float (s) | 6.0–15.0 (기본 10.0) | 각성 지속시간 (Tuning Knob, game-concept.md 명시 10초가 기본값) |

**Output Range**: CurrentTime + 6.0 ~ CurrentTime + 15.0.
**Example**: t=32.0s에 발동 → EndTime=42.0s. t=38.0s에 재트리거(리프레시) → EndTime=48.0s (잔여 4초가 10초로 리셋 — 가산 아님).

### Overdrive Time Remaining (매 프레임, HUD 노출값)

`OverdriveTimeRemaining(t) = max(0, OverdriveEndTime - t)`

**Variables:**
| Variable | Type | Range | Description |
|----------|------|-------|-------------|
| t | float (s) | 런타임 | 현재 시각 |
| OverdriveEndTime | float (s) | 위 공식 결과 | — |

**Output Range**: 0 ~ OverdriveDuration. 0 도달 프레임에 종료 처리(Rule 4).
**Example**: EndTime=42.0s, t=39.5s → 잔여 2.5s (Combat HUD 표시값).

### Effective Overdrive Uptime (체인 분석용, 구현 공식 아님)

`TheoreticalChainCondition = (오버드라이브 중 10초 내 누적 TensionGain) >= TensionGaugeMax`

**Notes**: 쿨타임 제로 상태에서 Blackhole(텐션 70/명중) + Fire/Lightning(25/명중)을 난사하면 명중 2~3회로 게이지가 재차 Max에 도달할 수 있다 — 즉 적이 충분히 밀집한 상황에서는 리프레시 체인으로 오버드라이브가 사실상 연장 유지될 수 있다. 이것은 상류(combo-tension-gauge.md Core Rule 6)가 명시적으로 허용한 동작이며 "Endless Catharsis" 필라와 방향은 일치하나, 밸런스 붕괴(상시 각성) 위험은 플레이테스트 검증 대상이다(Open Questions 및 systems-index.md High-Risk 표의 "Combo/Tension Gauge → Luna Overdrive" 리스크와 동일 계열).

## Edge Cases

| Scenario | Expected Behavior | Rationale |
|----------|-------------------|-----------|
| 종료 조건 경합: 시간 만료 vs 마나 소모 | **종료 조건은 시간 만료(및 사망)뿐** — 마나는 오버드라이브 중 소모 자체가 발생하지 않으므로(EffectiveManaCost=0) "마나 고갈로 조기 종료" 경로는 존재하지 않음 | Spell Casting의 코스트 우회가 게이트 자체를 건너뛰므로 마나 기반 종료는 정의 불가능 — 미정의 상태로 남기지 않고 "발생 안 함"을 명시(spell-casting-base.md의 부분 우회 명시 패턴과 동일) |
| 만료 프레임에 캐스트 입력이 겹침 | 태그 해제가 먼저 해소된 후 게이트 평가 — 해당 캐스트는 정상 코스트/쿨다운 검사를 받음(공짜 마지막 한 발 없음) | spell-casting-base.md Edge Cases의 bypass-release same-frame race 순서 계약 그대로 채택(Rule 4) — 프레임 복불복 제거 |
| 만료 프레임과 재트리거(`OnOverdriveTriggered`)가 같은 프레임에 겹침 | 트리거 수신 처리를 만료 판정보다 먼저 수행 — 리프레시 성립, 오버드라이브 연속 유지(Ended/Started 이벤트 발행 없음) | 게이지를 Max까지 채운 성과가 프레임 타이밍 때문에 증발하면 안 됨 — 플레이어에게 유리한 쪽으로 결정론적 순서 고정 |
| 오버드라이브 중 플레이어 사망 | 즉시 종료: 타이머 취소, 태그 해제, `OnOverdriveEnded(PlayerDeath)` 1회 발행. 재시작 시 Inactive에서 시작 | Rule 6 — 사망 후 태그 잔존 시 부활 직후 공짜 캐스트가 성립하는 버그 방지. Combo/Tension Gauge의 사망 시 게이지 0 리셋과 일관 |
| `OnOverdriveTriggered`와 플레이어 Death가 같은 프레임에 겹침 | Death 처리 우선 — 오버드라이브 발동 무효(Started 이벤트 미발행, 태그 미부여) | 죽는 프레임에 각성이 켜졌다 즉시 꺼지는 이벤트 노이즈(Started+Ended 동시 발행)가 하류(HUD/Visual State)에 잔상을 남기는 것을 방지 |
| 오버드라이브 중 마나/쿨다운 상태 | 마나는 캐스트로 소모되지 않고 패시브 리젠은 계속 틱(대개 종료 시점에 만충) — 쿨다운 타이머는 진입 자체가 없어 종료 즉시 3속성 모두 Ready | spell-casting-base.md Edge Cases("우회 중 쿨다운 GE 진입 자체가 없음") 상속 — 종료 후 "숨겨진 쿨다운/빈 마나"가 판타지를 깨지 않음 |
| `CostBypass.Active` 태그가 이 시스템 외부에서 조작됨 | 발생 금지 계약 — 이 태그의 부여/해제 권한은 Luna Overdrive 단독 소유(테스트 하네스 제외, spell-casting-base.md AC5의 스텁 예외). 외부 시스템이 필요해지면 참조카운트 설계로 리비전 | 단일 소유자 + 단일 grant(Rule 5)여야 이중 해제/조기 해제 경합이 구조적으로 불가능 |
| 리프레시 직후 즉시 또 리프레시(연속 재트리거) | 각 트리거마다 EndTime 재계산만 반복 — 상한 없음(체인 허용) | 상류가 재발동을 허용한 이상 인위적 상한은 이 문서가 발명할 새 규칙 — 밸런스 상한이 필요하면 상류 게이지 쪽 축적 억제로 해결할 문제(Open Questions) |

## Dependencies

| System | Direction | Nature of Dependency |
|--------|-----------|----------------------|
| Combo/Tension Gauge | 상류 (이 시스템이 의존, 하드) | `OnOverdriveTriggered` 이벤트 구독(무페이로드 — 페이로드 결정은 이 문서 Rule 1이 확정) |
| Spell Casting (base) | 상류 (이 시스템이 의존, 하드) | `CostBypass.Active` 태그 부여/해제 — Core Rule 10 훅의 유일한 정규 구동자 |
| Health/Damage Core | 상류 (이 시스템이 의존, 소프트) | 플레이어 Death 상태(`OnDeath`) 구독 — 강제 종료 트리거 |
| Combat HUD (미설계) | 하류 (이 시스템에 의존, 소프트) | `OnOverdriveStarted`/`OnOverdriveEnded` + `OverdriveTimeRemaining` read-only 소비 |
| Overdrive Visual State (미설계) | 하류 (이 시스템에 의존, 하드) | 시작/종료 이벤트를 크림슨 레드 연출 트리거로 소비 — 연출 내용은 그쪽 소유 |

## Tuning Knobs

| Knob | Default | Safe Range | Too High | Too Low |
|---|---|---|---|---|
| OverdriveDuration | 10.0초 (game-concept.md 명시값) | 6.0–15.0초 | 각성이 일상화 → 희소성/텐션곡선 붕괴, 리프레시 체인과 결합 시 상시 각성 위험 | 난사 리듬이 몸에 붙기 전에 종료 → "해방감" 미성립 |

**교차 제약**: `OverdriveDuration`은 Combo/Tension Gauge의 축적 속도(`TensionGainCoefficient`, `TensionGaugeMax`)와 한 세트로 튜닝할 것 — 지속시간을 늘리면 오버드라이브 중 재충전(리프레시 체인) 성립이 쉬워져 사실상 지속시간이 기하급수적으로 늘어난다(Formulas의 체인 분석 참조). 상류 게이지 노브를 건드리지 않고 이 노브만 올리는 것은 금지 패턴.

> 이 시스템의 튜닝 노브는 위 1개뿐이다 — 데미지/이속 배율이 없고(Rule 2), 발동 임계값은 상류 소유(Rule 7). 노브가 적은 것이 의도된 설계: 각성의 정체성은 수치가 아니라 규칙 변경이다.

## Visual/Audio Requirements

> **Note**: `art-director` 미컨설트 — Solo 모드. Production 전 수동 리뷰 필요.
> 본격 크림슨 레드 반전 연출(라이팅/포스트프로세스/이펙트 팔레트 전환)은 Overdrive Visual State(#18) GDD 소유 — 아래는 그 시스템이 없어도 MVP가 성립하기 위한 최소 피드백만.

| Event | Visual Feedback | Audio Feedback | Priority |
|-------|-----------------|-----------------|----------|
| 발동(`OnOverdriveStarted`) | 화면 전역 크림슨 틴트 포스트프로세스 즉시 적용(단순 컬러 그레이딩 — 본격 라이팅 반전의 MVP 대역) + 발동 순간 버스트 이펙트 | 각성 스팅어(강렬, 짧게) + 오버드라이브 전용 BGM 레이어/필터 전환 | High |
| 지속 중 | 크림슨 틴트 유지, 플레이어 캐릭터 발광(오라) | 저음 강조 드론 루프(심박 고조감) | High |
| 종료(`OnOverdriveEnded`) | 틴트 1초 내 페이드아웃 — 급격한 하락(텐션 곡선)을 시각적으로도 체감 | 종료 디센딩 스팅어(해제감) + BGM 원복 | High |
| 잔여시간 경고(예: 3초 이하) | 틴트 펄스(깜빡임) 시작 — "곧 끝난다" 예고 | 심박 루프 템포 상승 | Medium |

> 📌 **Asset Spec** — Art Bible 승인 후 `/asset-spec system:luna-overdrive`로 구체 스펙화 필요. 크림슨 레드 팔레트 정의는 Art Bible/Overdrive Visual State와 단일 소스로 공유할 것.

## UI Requirements

이 문서는 UI 레이아웃/스타일을 소유하지 않음(Combat HUD GDD 소유) — 다음 인터페이스 데이터만 노출한다:

- `OnOverdriveStarted` / `OnOverdriveEnded(EndReason)` 이벤트 (HUD 각성 인디케이터 On/Off + 크림슨 전환 트리거용)
- `OverdriveTimeRemaining` 실시간 값 (잔여시간 표시용, Formulas 참조)
- (간접) `CostBypass.Active` 태그 존재 여부 — spell-casting-base.md UI Requirements가 이미 노출하는 "무한마나 표시 트리거"와 동일 신호이므로 HUD는 중복 구독 없이 한쪽만 쓰면 됨 (권장: 이 문서의 Started/Ended 이벤트 — 의미가 더 명확)
- 패드 접근성: 별도 입력 불필요(순수 읽기 전용 표시), 패드 내비게이션 영향 없음.

> **📌 UX Flag — Luna Overdrive**: 이 시스템은 UI 요구사항 있음. Pre-Production 단계에서 `/ux-design`으로 Combat HUD 화면 스펙(`design/ux/combat-hud.md`) 작성 시 위 인터페이스를 참조할 것.

## Acceptance Criteria

1. **GIVEN** Inactive 상태, TensionGauge가 Max 도달로 `OnOverdriveTriggered` 발행, **WHEN** 이벤트 수신 프레임, **THEN** 같은 프레임에 `CostBypass.Active` 태그 부여 + `OnOverdriveStarted` 1회 발행 + `OverdriveTimeRemaining`=10.0(기본값).
2. **GIVEN** Overdrive Active, **WHEN** Blackhole 연속 3회 캐스트(쿨다운 무시), **THEN** 3회 모두 즉시 발동, Mana 변화 없음(spell-casting-base.md AC5와 동일 결과가 실제 트리거 경로로 재현됨).
3. **GIVEN** Overdrive Active 발동 후 10.0초 경과(리프레시 없음), **WHEN** 만료 프레임, **THEN** `CostBypass.Active` 해제 + `OnOverdriveEnded(Expired)` 1회 발행 + 3속성 쿨다운 모두 Ready(잔존 쿨다운 없음).
4. **GIVEN** 만료 프레임에 Fire 캐스트 입력이 동시 발생, **WHEN** 프레임 처리, **THEN** 태그 해제가 먼저 반영되어 해당 캐스트는 정상 코스트(25) 차감 + 정상 쿨다운 진입(공짜 캐스트 아님).
5. **GIVEN** Overdrive Active(잔여 4.0초), 게이지가 재차 Max 도달, **WHEN** `OnOverdriveTriggered` 재수신, **THEN** `OverdriveTimeRemaining`=10.0으로 리프레시, 태그는 연속 유지, `OnOverdriveStarted` 재발행 없음.
6. **GIVEN** Overdrive Active, **WHEN** 플레이어 Death 상태 진입, **THEN** 즉시 태그 해제 + `OnOverdriveEnded(PlayerDeath)` 1회 발행, 재시작 후 Inactive 상태(태그 잔존 없음).
7. **GIVEN** 같은 프레임에 `OnOverdriveTriggered`와 플레이어 Death가 동시 발생, **WHEN** 프레임 처리, **THEN** 오버드라이브 미발동(`OnOverdriveStarted` 미발행, 태그 미부여).
8. **GIVEN** Overdrive Active 진입 시 Mana=30, **WHEN** 지속 10초간 캐스트 다수 수행 후 종료, **THEN** Mana는 캐스트로 감소하지 않고 패시브 리젠만 적용된 값(기본 리젠 8/s 기준 만충 100)으로 종료.

## Open Questions

- **Owner: game-designer, Target: 프로토타입 플레이테스트** — 리프레시 체인(오버드라이브 중 재충전 → 사실상 상시 각성) 성립 여부와 체감 검증. 억제가 필요하면 상류(Combo/Tension Gauge)의 오버드라이브 중 획득 계수 노브 추가로 해결할 것 — 이 문서에 상한을 발명하지 않기로 결정(Edge Cases 참조).
- **Owner: game-designer, Target: 파편 드랍/흡수 시스템 설계 시** — game-concept.md의 제2 발동 경로 "거대 파편 흡수"는 소유 시스템 미존재로 MVP 제외(Rule 10). 해당 시스템 설계 시 `OnOverdriveTriggered`와 동일한 트리거 계약으로 합류할지, 별도 이벤트로 둘지 확정 필요.
- **Owner: art-director, Target: Overdrive Visual State GDD 설계 시** — MVP 크림슨 틴트(단순 포스트프로세스)와 본격 라이팅 반전 연출의 경계/이관 범위 확정. 이 문서의 Visual/Audio Requirements는 임시 대역임.
- **Owner: qa-lead, Target: Production 진입 전** — 위 Acceptance Criteria는 solo 모드로 작성, qa-lead 미검증.

> **Note**: `systems-designer`/`qa-lead`/`art-director` 미컨설트 — Solo 모드. Production 전 수동 리뷰 필요.
