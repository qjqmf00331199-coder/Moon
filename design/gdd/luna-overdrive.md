# Luna Overdrive (Blood Moon)

> **Status**: Approved (2026-07-23 re-review: fixed-window rewrite confirmed internally consistent with spell-casting-base.md and ADR-0004; registry gap fixed — `overdrive_recovery_duration` registered)
> **Author**: user + game-designer (solo design mode) + 2026-07-17 full design-review (game-designer, systems-designer, qa-lead, ue-gas-specialist, creative-director) + 2026-07-18 full re-review (APPROVED, lean solo pass) + 2026-07-20 Antigravity sync review + 2026-07-23 Claude Code re-review
> **Last Updated**: 2026-07-23
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
   **구현 노트 (2026-07-17 design-review 반영, blocking)**: 이 태그는 정규 grantor가 Luna Overdrive **단 하나**이므로, 프로젝트의 참조카운트 loose tag 관례를 따르지 않는다. 구현은 비카운트 set/clear(`SetLooseGameplayTagCount(1/0)`)만 사용한다. Active/Recovery 재트리거는 태그를 다시 부여하지 않는다.
3. **지속시간은 타이머 기반** — 발동 시점부터 `OverdriveDuration`(Tuning Knob, 기본 10초 — game-concept.md 명시값) 카운트다운. 만료 프레임에 종료 처리.
   **구현 계약 (2026-07-18 재리뷰 반영, blocking 해소)**: 지속시간은 **일반 타이머 변수(`OverdriveEndTime`)로만 구현** — Duration형 GameplayEffect가 태그를 대신 소유하는 방식은 **채택하지 않는다**. 이전 리뷰(2026-07-17)의 GE 권장은 Rule 2의 비카운트 loose tag 계약과 정면으로 충돌한다는 사실이 2026-07-18 재리뷰(systems-designer, ue-gas-specialist 독립 수렴)에서 확인됨 — GE가 태그를 부여하면 태그는 GE 전용 컨테이너에 귀속되어, health-damage-core.md의 Death 전환 시 "모든 오버레이 태그 클리어"가 loose tag만 대상으로 하는 이상 Rule 6의 이중 해제 안전망(HDC 블랭킷 클리어)이 더 이상 이 태그에 적용되지 않게 된다 — Rule 6이 "확인됨"이라 적어둔 전제가 조용히 무너지는 경로였다. 따라서 GE 소유 패턴은 폐기하고, 태그는 끝까지 Rule 2의 `SetLooseGameplayTagCount(1/0)` 단독 경로로만 다룬다. `OverdriveTimeRemaining`은 타이머 변수(`OverdriveEndTime - CurrentTime`)로 직접 계산 — GE 잔여시간 쿼리 API 대조는 더 이상 필요 없음(Open Questions의 해당 knowledge-gap 항목 폐기).
4. **종료 = 태그 해제, 순서 계약 준수 — 강제 메커니즘: lazy 시간 비교** — 종료 프레임에 `CostBypass.Active`를 해제하며, **태그 상태 변경이 그 프레임의 캐스트 게이트 평가보다 먼저 해소된다**(spell-casting-base.md Edge Cases의 bypass-release same-frame race 순서 계약을 그대로 채택). 종료 프레임에 입력된 캐스트는 정상 코스트/쿨다운 검사를 받는다.
   **구현 계약 (2026-07-17 design-review 반영, blocking)**: 이 순서(및 Edge Cases의 모든 same-frame tie-break)는 콜백/타이머 실행 순서에 의존해서 달성하지 **않는다** — 타이머 콜백과 Enhanced Input 게이트 평가는 각자 고정된 tick group에서 결정론적으로 실행되지만(2026-07-18 재리뷰에서 "구성만으로 보장 안 됨"이라는 이전 표현이 과장임을 확인 — 실제로는 검증 가능한 고정 스케줄), 그 스케줄을 문서 차원에서 못 박고 유지보수하는 것보다 **판정 지점 자체가 실행 순서에 무관하게 항상 같은 답을 내도록 만드는 쪽**이 더 견고하다. 따라서 **모든 판정 지점(캐스트 게이트, 만료 처리, 트리거/사망 핸들러)이 평가 시점에 `CurrentTime` vs `OverdriveEndTime`을 직접 비교**한다(lazy evaluation) — 이는 "엔진이 순서를 보장 안 해서"가 아니라 "타이머/입력 서브시스템 등록이 바뀌어도 계약이 깨지지 않게 하려는" 설계 선택이다. 태그의 물리적 해제가 프레임 내 언제 일어나든, `CurrentTime >= OverdriveEndTime`이면 게이트는 이미 "우회 없음"으로 판정 — 순서 계약이 실행 순서 운에 의존하지 않고 구성상 참이 된다. Spell Casting Rule 10의 태그 검사 지점 2개(CanActivateAbility/커밋 분기)에는 이 시간 비교를 병행 조건으로 전달(태그 AND 미만료)하는 형태로 구현.
5. **Active 중 재트리거·텐션 획득 금지** — 오버드라이브는 `OverdriveDuration`(기본 10초)의 고정 창이다. Active 중 들어오는 `OnOverdriveTriggered`는 무시하며 EndTime을 갱신하지 않는다. 모든 텐션 획득원도 Active 동안 0으로 처리한다. 멀티타깃 Blackhole이 한 번에 100 이상을 재충전해 사실상 영구 각성을 만드는 경로를 구조적으로 닫는다.
6. **플레이어 사망 시 즉시 강제 종료** — Health/Damage Core의 플레이어 Death 상태 진입 시 타이머 취소 + 태그 즉시 해제. 재시작 후 오버드라이브 이월 없음(Combo/Tension Gauge의 사망 시 게이지 0 리셋과 일관).
   **이중 해제 소유권 정리 (2026-07-17 design-review 반영, blocking)**: health-damage-core.md의 Death 전환 시 "모든 오버레이 태그 클리어" 규칙과 이 Rule의 해제가 같은 프레임에 겹칠 수 있다 — 태그가 비카운트형(Rule 2 구현 노트)이므로 이중 remove는 무해(멱등)하며, **정규 해제 소유자는 Luna Overdrive**다(HDC의 블랭킷 클리어는 안전망일 뿐). 향후 이 태그에 참조카운트를 도입하는 리비전이 생기면(예: 파편 흡수 트리거 추가) 이 이중 해제 경로를 먼저 해소해야 한다 — 그 전까지 카운트형 전환 금지.
7. **발동 임계값을 재설계하지 않는다** — 게이지 Max(100), 축적 공식, 감쇠는 전부 Combo/Tension Gauge 소유(systems-index.md High-Risk 표의 소유 경계). 이 문서는 트리거 수신 이후만 소유.
8. **상태 이벤트를 하류에 노출** — `OnOverdriveStarted`(발동 프레임), `OnOverdriveEnded(EndReason)`(종료 프레임, EndReason ∈ {Expired, PlayerDeath}) 이벤트와 잔여시간 조회 인터페이스(`OverdriveTimeRemaining`)를 노출한다. Combat HUD(각성 표시/잔여시간)와 Overdrive Visual State(크림슨 레드 연출 시작/해제)가 소비. 만료 뒤 `OverdriveRecoveryDuration`(기본 1.5초) 동안 텐션 획득만 잠기며, 마나 회복·유료 캐스트·이동·대쉬는 즉시 정상화한다.
9. **Active 중 마나 회복 정지** — 무료 캐스트 동안 패시브 Mana Regen을 정지하고 진입 시점의 Mana를 보존한다. 종료 시 소급 비용이나 강제 드레인을 적용하지 않는다. 이로써 종료 직후 플레이어는 진입 전 자원 제약으로 즉시 돌아가며, 코어 적출의 100% 회복 보상 가치도 유지된다.
10. **시각 반전의 트리거만 소유** — '푸른색 → 크림슨 레드' 전환의 본격 연출(라이팅/포스트프로세스/이펙트 팔레트 교체)은 Overdrive Visual State(#18, Vertical Slice, 미설계) 소유. 이 문서는 Rule 8의 시작/종료 이벤트가 그 연출의 유일한 트리거라는 계약과, MVP용 최소 피드백(Visual/Audio Requirements)만 소유.
11. **거대 파편 흡수 트리거는 MVP 범위 밖** — game-concept.md의 "일정 콤보 게이지 도달 **또는 거대 파편 흡수** 시" 중 후자는 소유 시스템이 아직 없어(파편 드랍/흡수 시스템 미설계) 이번 범위에서 제외. 발동 경로가 추가되어도 이 문서의 "트리거 수신 → Active" 계약은 재사용 가능(Open Questions 등록).

### States and Transitions

| State | Entry Condition | Exit Condition | Behavior |
|---|---|---|---|
| Inactive | 기본 상태 / Recovery 종료 | `OnOverdriveTriggered` 수신 | 태그 없음, 정상 코스트/쿨다운/텐션 획득 |
| Active | `OnOverdriveTriggered` 수신 프레임 | 타이머 만료(→Recovery) 또는 플레이어 Death(→Inactive) | `CostBypass.Active` 유지, 잔여시간 카운트다운, 재트리거·텐션 획득 무시, Mana Regen 정지 |
| Recovery | Active 만료 | `OverdriveRecoveryDuration` 경과 | 태그 없음, 정상 코스트/쿨다운/Mana Regen, 텐션 획득만 무시 |

전이 시 이벤트: Inactive→Active에서 `OnOverdriveStarted` 1회, Active→Recovery 또는 Active→Inactive(Death)에서 `OnOverdriveEnded(EndReason)` 1회 발행한다. Active/Recovery 중 재트리거는 상태 전이가 아니며 이벤트를 발행하지 않는다.

### Interactions with Other Systems

- **Combo/Tension Gauge** (상류, 하드) — `OnOverdriveTriggered` 구독(무페이로드, Rule 1). 게이지 리셋과 평상시 축적은 상류 소유이며 Active/Recovery 잠금은 이 문서의 상태 조회 계약을 따른다.
- **Spell Casting (base)** (상류, 하드) — `CostBypass.Active` 태그 부여/해제(Rule 2/4). 태그의 동작 의미("켜지면 코스트/쿨다운 검사를 완전히 건너뛴다")는 상류 소유, On/Off 타이밍은 이 문서 소유.
- **Health/Damage Core** (상류, 소프트) — 플레이어 Death 상태(`OnDeath`) 구독(Rule 6, 강제 종료).
- **Combat HUD** (하류, 소프트) — Rule 8의 상태 이벤트 + 잔여시간 read-only 소비. 없어도 오버드라이브 로직 자체는 정상 동작.
- **Overdrive Visual State** (하류, 미설계) — Rule 8/9의 시작/종료 이벤트를 연출 트리거로 소비 예정. 연출 내용 전부 그쪽 GDD 소유.

## Formulas

### Overdrive End Time (발동)

`OverdriveEndTime = CurrentTime + OverdriveDuration`

**Variables:**
| Variable | Symbol | Type | Range | Description |
|----------|--------|------|-------|-------------|
| CurrentTime | — | float (s) | 런타임 | 발동이 처리된 시각 |
| OverdriveDuration | — | float (s) | 6.0–15.0 (기본 10.0) | 각성 지속시간 (Tuning Knob, game-concept.md 명시 10초가 기본값) |

**Output Range**: CurrentTime + 6.0 ~ CurrentTime + 15.0.
**Example**: t=32.0s에 발동 → EndTime=42.0s. t=38.0s에 재트리거가 와도 EndTime은 42.0s로 유지된다.

### Overdrive Time Remaining (매 프레임, HUD 노출값)

`OverdriveTimeRemaining(t) = max(0, OverdriveEndTime - t)`

**Variables:**
| Variable | Type | Range | Description |
|----------|------|-------|-------------|
| t | float (s) | 런타임 | 현재 시각 |
| OverdriveEndTime | float (s) | 위 공식 결과 | — |

**Output Range**: 0 ~ OverdriveDuration. 0 도달 프레임에 종료 처리(Rule 4).
**Example**: EndTime=42.0s, t=39.5s → 잔여 2.5s (Combat HUD 표시값).

### Recovery End Time

`RecoveryEndTime = OverdriveEndTime + OverdriveRecoveryDuration`

**Output Range**: OverdriveEndTime + 1.0~2.0초(기본 1.5초). Recovery 중 텐션 획득은 0이며 다른 행동은 정상 처리한다.

## Edge Cases

| Scenario | Expected Behavior | Rationale |
|----------|-------------------|-----------|
| 종료 조건 경합: 시간 만료 vs 마나 소모 | **종료 조건은 시간 만료(및 사망)뿐** — 마나는 오버드라이브 중 소모 자체가 발생하지 않으므로(EffectiveManaCost=0) "마나 고갈로 조기 종료" 경로는 존재하지 않음 | Spell Casting의 코스트 우회가 게이트 자체를 건너뛰므로 마나 기반 종료는 정의 불가능 — 미정의 상태로 남기지 않고 "발생 안 함"을 명시(spell-casting-base.md의 부분 우회 명시 패턴과 동일) |
| 만료 프레임에 캐스트 입력이 겹침 | 태그 해제가 먼저 해소된 후 게이트 평가 — 해당 캐스트는 정상 코스트/쿨다운 검사를 받음(공짜 마지막 한 발 없음). **강제 수단**: 게이트가 평가 시점에 `CurrentTime >= OverdriveEndTime` lazy 비교(Rule 4 구현 계약) — 태그 물리 해제 시점과 무관하게 성립 | spell-casting-base.md Edge Cases의 bypass-release same-frame race 순서 계약 그대로 채택(Rule 4) — 실행 순서 운이 아니라 구성상 참 |
| 만료 프레임과 재트리거(`OnOverdriveTriggered`)가 같은 프레임에 겹침 | `CurrentTime >= OverdriveEndTime`이면 만료가 우선이며 재트리거는 Recovery 규칙에 따라 무시된다 | 고정 10초 창을 프레임 순서로 우회할 수 없어야 함 |
| 오버드라이브 중 플레이어 사망 | 즉시 종료: 타이머 취소, 태그 해제, `OnOverdriveEnded(PlayerDeath)` 1회 발행. 재시작 시 Inactive에서 시작 | Rule 6 — 사망 후 태그 잔존 시 부활 직후 공짜 캐스트가 성립하는 버그 방지. Combo/Tension Gauge의 사망 시 게이지 0 리셋과 일관 |
| `OnOverdriveTriggered`와 플레이어 Death가 같은 프레임에 겹침 | Death 처리 우선 — 오버드라이브 발동 무효(Started 이벤트 미발행, 태그 미부여). **강제 수단**: 트리거 핸들러가 발동 확정 전에 플레이어 Death 상태를 평가 시점 조회(lazy) — Death면 발동 자체를 시작하지 않음. 핸들러 실행 순서에 의존하지 않도록, 발동 확정도 만료와 동일하게 프레임 말미 평가에서 Death 상태를 최종 재확인 후 커밋 | 죽는 프레임에 각성이 켜졌다 즉시 꺼지는 이벤트 노이즈(Started+Ended 동시 발행)가 하류(HUD/Visual State)에 잔상을 남기는 것을 방지 |
| 오버드라이브 중 마나/쿨다운 상태 | 마나는 캐스트로 소모되지 않지만 패시브 Regen도 정지하여 진입값을 유지한다. 우회 중 캐스트는 쿨다운 GE에 진입하지 않는다 | 무료 창이 종료 자원을 자동 만충시키지 않게 하되 소급 비용/쿨다운 부채는 만들지 않음 |
| `CostBypass.Active` 태그가 이 시스템 외부에서 조작됨 | 발생 금지 계약 — 이 태그의 부여/해제 권한은 Luna Overdrive 단독 소유(테스트 하네스 제외, spell-casting-base.md AC5의 스텁 예외). 외부 시스템이 필요해지면 참조카운트 설계로 리비전 | 단일 소유자 + 단일 grant(Rule 5)여야 이중 해제/조기 해제 경합이 구조적으로 불가능 |
| Active 또는 Recovery 중 연속 재트리거 | 전부 무시하며 EndTime/RecoveryEndTime과 태그 카운트가 변하지 않음 | 입력·멀티타깃 이벤트 수와 무관하게 고정 창 보장 |
| 오버드라이브 중 플레이어가 스턴/CC 상태에 진입 | 타이머는 계속 흐른다 — CC는 오버드라이브 지속시간을 일시정지/연장하지 않음(별도의 정지 규칙 없음, 이 문서가 CC 시스템을 소유하지 않으므로 CC의 존재 자체가 계약에 영향 없음) | CC 중 소모된 지속시간을 보상하는 규칙은 CC를 소유하는 시스템(Status Effect, 미설계)이 필요하다고 판단하면 그쪽에서 추가할 문제 — 이 문서가 선제적으로 발명하지 않음(2026-07-18 재리뷰, game-designer 지적) |
| Active 중 재트리거와 플레이어 Death가 같은 프레임에 겹침 | Death 우선 — 재트리거는 원래 무시되며 즉시 종료(`OnOverdriveEnded(PlayerDeath)` 발행, 태그 해제) | 사망한 플레이어에게 Active/Recovery 상태가 남지 않음 |

## Dependencies

| System | Direction | Nature of Dependency |
|--------|-----------|----------------------|
| Combo/Tension Gauge | 상류 (이 시스템이 의존, 하드) | `OnOverdriveTriggered` 이벤트 구독(무페이로드 — 페이로드 결정은 이 문서 Rule 1이 확정) |
| Spell Casting (base) | 상류 (이 시스템이 의존, 하드) | `CostBypass.Active` 태그 부여/해제 — Core Rule 10 훅의 유일한 정규 구동자. **교차 의존 RESOLVED (2026-07-17 BLOCKING → spell-casting-base.md Core Rule 11로 해소, 2026-07-18 재리뷰에서 문구 정리)**: 오버드라이브 중 캐스트 레이트 제약(터보/매크로 악용 방지)은 **Spell Casting (base) 소유** — 스펠당 프레임당 1캐스트 cap + 전역 `MaxCastsPerSecond=20`(Core Rule 11)로 이미 구현 전제조건 해소됨. 이 문서는 여전히 캡을 발명하지 않음(소유권 규율 유지) |
| Health/Damage Core | 상류 (이 시스템이 의존, 소프트) | 플레이어 Death 상태(`OnDeath`) 구독 — 강제 종료 트리거 |
| Combat HUD | 하류 (이 시스템에 의존, 소프트) | `OnOverdriveStarted`/`OnOverdriveEnded` + `OverdriveTimeRemaining` read-only 소비 |
| Overdrive Visual State (미설계) | 하류 (이 시스템에 의존, 하드) | 시작/종료 이벤트를 크림슨 레드 연출 트리거로 소비 — 연출 내용은 그쪽 소유 |

## Tuning Knobs

| Knob | Default | Safe Range | Too High | Too Low |
|---|---|---|---|---|
| OverdriveDuration | 10.0초 (game-concept.md 명시값) | 6.0–15.0초 | 각성이 일상화 → 희소성/텐션곡선 붕괴 | 난사 리듬이 몸에 붙기 전에 종료 → "해방감" 미성립 |
| OverdriveRecoveryDuration | 1.5초 | 1.0–2.0초 | 다음 상승 시작이 과도하게 늦어 전투 리듬 단절 | 하강을 인지하기 전에 새 게이지 축적 시작 |

**교차 제약**: `OverdriveDuration + OverdriveRecoveryDuration`은 다음 상승이 시작되기까지의 최소 주기다. Duration을 늘릴 때 Recovery까지 함께 늘리지 않는다. Active/Recovery 텐션 획득은 항상 0이며 배율 노브로 다시 열 수 없다.

> 이 시스템의 튜닝 노브는 위 2개뿐이다 — 데미지/이속 배율이 없고(Rule 2), 발동 임계값은 상류 소유(Rule 7). 노브가 적은 것이 의도된 설계다.

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

> **테스트 진입 계약(전 AC 공통)**: "Overdrive Active" 진입은 테스트 하네스가 `OnOverdriveTriggered`를 직접 발행해 달성한다(상류 경계 스텁 — Combo/Tension Gauge 실축적 불요). `CostBypass.Active` 태그 직접 부여는 금지 — 그건 이 시스템의 검증 대상인 상태 기계 자체를 우회한다(spell-casting-base.md AC5의 스텁이 태그 직접 부여인 것과 반대인 이유: 그쪽은 태그 소비자, 이쪽은 태그 소유자). *(2026-07-17 design-review 반영)*
> **증거 유형**: Logic(unit) = 이 시스템 상태 기계 단독, 캐스팅 스텁 / Integration = 실제 Spell Casting 코스트·쿨다운 코드 필요.

1. **GIVEN** Inactive 상태, 하네스가 `OnOverdriveTriggered` 발행, **WHEN** 이벤트 수신 프레임, **THEN** 같은 프레임에 `CostBypass.Active` 태그 부여 + `OnOverdriveStarted` 1회 발행 + `OverdriveTimeRemaining`=`OverdriveDuration`(기본 10.0). *(Logic/unit)*
2. **GIVEN** Overdrive Active(AC1 경로로 진입 — 트리거 스텁, 태그 직접 부여 아님), **WHEN** Blackhole 연속 3회 캐스트(쿨다운 무시), **THEN** 3회 모두 즉시 발동, Mana 변화 없음(spell-casting-base.md AC5와 동일 결과가 실제 트리거 경로로 재현됨). *(Integration — 실제 Spell Casting 게이트 코드)*
3. **GIVEN** Overdrive Active 발동 후 `OverdriveDuration`(기본 10.0초) 경과(리프레시 없음), **WHEN** 만료 프레임, **THEN** `CostBypass.Active` 해제 + `OnOverdriveEnded(Expired)` 1회 발행 + 3속성 쿨다운 모두 Ready(잔존 쿨다운 없음). *(Logic/unit — 쿨다운 Ready 확인만 Integration)*
4. **GIVEN** 만료 프레임에 Fire 캐스트 입력이 동시 발생, **WHEN** 프레임 처리, **THEN** lazy 시간 비교(Rule 4)에 의해 해당 캐스트는 정상 코스트(25) 차감 + 정상 쿨다운 진입(공짜 캐스트 아님). *(Integration — 실제 캐스트 게이트)*
5. **GIVEN** Overdrive Active(잔여 4.0초), **WHEN** `OnOverdriveTriggered` 재수신, **THEN** 잔여시간은 4.0초를 유지하고 태그 카운트/Started 이벤트가 변하지 않는다. *(Logic/unit)*
6. **GIVEN** Overdrive Active, **WHEN** 플레이어 Death 상태 진입, **THEN** 즉시 태그 해제 + `OnOverdriveEnded(PlayerDeath)` 1회 발행, 재시작 후 Inactive 상태(태그 잔존 없음 — HDC 블랭킷 클리어와의 이중 remove 무해 확인 포함, Rule 6 구현 노트). *(Logic/unit — Death 상태 스텁)*
7. **GIVEN** 같은 프레임에 `OnOverdriveTriggered`와 플레이어 Death가 동시 발생, **WHEN** 프레임 처리, **THEN** 오버드라이브 미발동(`OnOverdriveStarted` 미발행, 태그 미부여) — 핸들러 발화 순서 양방향 모두에서 성립(Edge Cases 강제 수단). *(Logic/unit)*
8. **GIVEN** Overdrive Active 진입 시 Mana=30, **WHEN** 지속 10초간 캐스트 다수 수행 후 종료, **THEN** Mana는 캐스트와 패시브 Regen 양쪽 모두로 변하지 않아 30으로 종료한다. *(Integration — 실제 마나/리젠 코드)*
9. **GIVEN** Overdrive Active, `OverdriveTimeRemaining`이 정확히 0에 도달하는 프레임에 `OnOverdriveTriggered`가 같은 프레임 수신, **WHEN** 프레임 처리, **THEN** 만료가 성립해 Recovery로 진입하고 태그가 해제되며 재발동하지 않는다. *(Logic/unit)*
10. **GIVEN** Active 또는 Recovery 중 Blackhole이 4개 대상에 명중, **WHEN** 각 대상의 텐션 이벤트가 들어와도, **THEN** TensionGauge=0 유지, EndTime/RecoveryEndTime 불변, Started 이벤트 추가 발행 없음. *(Integration)*
11. **GIVEN** 만료 후 Recovery 진입, **WHEN** 1.5초 동안 텐션 이벤트와 유료 스펠 입력이 함께 발생, **THEN** 텐션만 무시되고 Mana Regen과 정상 코스트/쿨다운은 적용되며 1.5초 경과 후 텐션 획득이 재개된다. *(Integration)*

## Open Questions

- **Owner: game-designer, Target: 시그니처 연쇄 스파이크 이후** — 고정 10초 + Recovery 1.5초의 하강 체감을 검증한다. 충분히 명확하면 유지하고, 숙련 보상이 부족할 때만 Execution 한정 연장(+최대 총 연장 상한)을 별도 리비전으로 검토한다.
- ~~**Owner: systems-designer, Target: Spell Casting (base) 리비전 — 이 시스템 구현 전 (BLOCKING)**~~ **RESOLVED (2026-07-17)** — spell-casting-base.md에 Core Rule 11(캐스트 레이트 바닥: per-element per-frame cap + 전역 MaxCastsPerSecond=20)이 추가되어 터보/매크로 익스플로잇 구조적 차단 완료. `max_casts_per_second` 상수 레지스트리 등록 완료. 이 문서의 구현 전제조건(BLOCKING) 해소.
- ~~**Owner: gameplay-programmer/ue-gas-specialist, Target: 구현 시 (knowledge-gap)** — Duration GE 잔여시간 쿼리 API, refresh-duration 스태킹 정책 API 명칭~~ **RESOLVED (2026-07-18)** — GE 소유 패턴 자체를 채택하지 않기로 결정(Rule 3 재작성, HDC 블랭킷 클리어 호환성 문제로 폐기). 태그는 끝까지 loose tag(`SetLooseGameplayTagCount`)로만 다루므로 이 API 대조 필요성이 사라짐.
- **Owner: gameplay-programmer, Target: 구현 시 (knowledge-gap, 축소된 범위)** — `SetLooseGameplayTagCount` 자체의 UE5.8 시그니처/세만틱스 변경 여부만 헤더 실물 대조 필요(engine-reference에 미수록, 학습 데이터 밖 버전) — GE 관련 API는 더 이상 이 문서의 관심사가 아님(위 항목 참조).
- **Owner: game-designer, Target: 파편 드랍/흡수 시스템 설계 시** — game-concept.md의 제2 발동 경로 "거대 파편 흡수"는 소유 시스템 미존재로 MVP 제외(Rule 10). 해당 시스템 설계 시 `OnOverdriveTriggered`와 동일한 트리거 계약으로 합류할지, 별도 이벤트로 둘지 확정 필요. 리뷰 지적(game-designer, 2026-07-17): 이 컷으로 MVP의 모든 발동이 동일한 slow-build 케이던스가 됨 — 프로토타입에서 즉발(spike) 체감을 미리 확인하려면 디버그 트리거(치트 키로 `OnOverdriveTriggered` 발행)로 충분, 별도 시스템 불요.
- **Owner: art-director, Target: Overdrive Visual State GDD 설계 시** — MVP 크림슨 틴트(단순 포스트프로세스)와 본격 라이팅 반전 연출의 경계/이관 범위 확정. 이 문서의 Visual/Audio Requirements는 임시 대역임.
- **Owner: qa-lead, Target: Production 진입 전** — 위 Acceptance Criteria는 solo 모드로 작성, qa-lead 미검증.

> **Note**: `systems-designer`/`qa-lead`/`art-director` 미컨설트 — Solo 모드. Production 전 수동 리뷰 필요.
