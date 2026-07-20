# Combat HUD

> **Status**: Designed (2026-07-18 cross-GDD review: NEEDS REVISION — W3 finding, see gdd-cross-review-2026-07-18.md)
> **Author**: user + game-designer (solo design mode)
> **Last Updated**: 2026-07-17
> **Implements Pillar**: Dopamine Driven Design — "보상의 가시화"(Endless Catharsis 목표) — 게이지가 차오르는 긴장, 각성 잔여시간, 처형 기회를 플레이어가 한눈에 읽게 만드는 정보 표면

## Overview

Combat HUD는 전투 중 상시 노출되는 읽기 전용 정보 표시 계층이다. 상류 시스템들이 UI Requirements에서 "Combat HUD GDD 소유"로 위임해 둔 표시 책임 전부 — 체력바(Health/Damage Core), 마나바 + 3속성 쿨다운 + 무한마나 표시(Spell Casting), 대쉬 차지(Dash/Evasion), 텐션 게이지 + Building/Decaying 구분(Combo/Tension Gauge), 오버드라이브 인디케이터 + 잔여시간(Luna Overdrive), F키 처형 프롬프트(Core Extraction Execution, 미설계 — 가정 인터페이스) — 를 단일 위젯 계층에서 소유한다. 이 문서는 위젯 인벤토리, 데이터 바인딩 계약(어느 시스템의 어떤 델리게이트/태그를 구독하는가), 갱신 정책(이벤트 기반, 60fps 예산 준수)을 소유하고, 레이아웃/스타일/배치는 후속 `/ux-design`(`design/ux/combat-hud.md`)과 Art Bible 소유로 남긴다. HUD는 어떤 게임플레이 상태도 소유하지 않는다 — 모든 값의 진실원은 상류 시스템이며 HUD는 순수 미러다.

## Player Fantasy

플레이어의 시선은 적과 이펙트에 있어야 하고, HUD는 곁눈으로 읽힌다 — "게이지가 거의 찼다", "대쉬 한 번 남았다", "저놈 처형 가능"이 0.2초 안에 인지되는 주변시(peripheral) 정보 설계가 목표다. 특히 텐션 게이지는 이 게임 도파민 루프의 압력계(Combo/Tension Gauge Player Fantasy: "게이지를 위해 플레이한다")이므로, 차오름·샘(Decaying)·임계 근접(Charged 강조)이 각각 구분되는 시각 언어를 가져야 한다. 오버드라이브 중에는 HUD 스스로도 크림슨 상태로 전환되어 "지금은 규칙이 다르다"(마나·쿨다운 무시)를 표시 자체가 증언한다. HUD가 판정을 만들거나 지연시키는 일은 절대 없다 — 프레젠테이션은 게임플레이를 비추기만 한다(Player Movement Rule 9 / Health-Damage Rule 5와 동일 철학).

**Feel Reference**: *DOOM Eternal* HUD(주변시로 읽히는 굵은 자원 표시), *Devil May Cry 5* 스타일 랭크 게이지(차오름 자체가 보상 연출).
**Anti-reference**: MMO식 고밀도 정보 패널 — 전투 중 읽기를 요구하는 텍스트/수치 나열은 Endless Catharsis(시선이 전장에 고정)와 상충.

> **Note**: `creative-director` 미컨설트 — Solo 모드. Production 전 수동 리뷰 필요.

## Detailed Design

### Core Rules

1. **HUD는 100% 읽기 전용** — 이번 MVP 범위의 모든 위젯(아래 7종)은 입력을 받지 않는다. F키 처형 등 액션 입력은 게임플레이 시스템이 직접 처리하고 HUD는 상태만 반영한다(handoff 확정 사항). 따라서 HUD 위젯은 전부 non-focusable이며 패드 포커스 내비게이션 그래프에서 제외된다 — technical-preferences.md의 "UI 패드 내비게이션 지원"은 메뉴/포커스형 UI에 적용되는 요구이고, 포커스 대상이 없는 HUD는 "패드 전환 시에도 동작 차이 없음"으로 그 요구를 충족한다.
2. **위젯 인벤토리 (MVP 7종)** — 각 위젯의 데이터 계약은 상류 GDD의 UI Requirements 섹션이 노출한 인터페이스만 사용한다(새 인터페이스 요구 금지):
   | # | Widget | Source System | Bound Interface |
   |---|--------|---------------|-----------------|
   | W1 | 체력바 | Health/Damage Core | `Health`/`MaxHealth` 어트리뷰트 변경 델리게이트 |
   | W2 | 저체력 경고(비네트 트리거) | Health/Damage Core | `OnHealthPercentCrossed(threshold)` — HUD가 요청하는 임계값은 `LowHealthWarningThreshold`(Tuning Knob) |
   | W3 | 마나바 | Spell Casting (base) | `Mana`/`MaxMana` 어트리뷰트 변경 델리게이트 + `CostBypass.Active` 태그(무한마나 표시 전환) |
   | W4 | 스펠 슬롯 ×3 (쿨다운 오버레이) | Spell Casting (base) | 3속성 각각의 `OnCooldown` 상태 + 남은 쿨다운 시간 |
   | W5 | 대쉬 차지 | Dash/Evasion | `CurrentCharges`(float) — 표시는 `floor()` 정수 핍 + 진행중 스택의 충전 프랙션 게이지 |
   | W6 | 텐션 게이지 | Combo/Tension Gauge | 게이지 값(0~Max) + Building/Decaying 상태 |
   | W7 | 오버드라이브 인디케이터 + 처형 프롬프트 | Luna Overdrive / Core Extraction Execution(미설계) | `OnOverdriveStarted`/`OnOverdriveEnded(EndReason)` + `OverdriveTimeRemaining`; 처형 프롬프트는 `State.Executable` 태그 기반 가정 인터페이스(Rule 6) |
3. **갱신은 이벤트 기반이 기본, 틱 폴링 금지** — 모든 위젯은 상류 델리게이트/태그 이벤트 수신 시에만 상태를 갱신한다. 예외 두 가지만 틱을 쓴다: (a) 연속 시각 보간(게이지 바 lerp, W6)과 (b) 시간 표시 스윕(쿨다운 오버레이 W4, 오버드라이브 잔여시간 W7) — 이들도 해당 상태가 활성일 때만 틱하고 유휴 시 틱 0. 60fps/16.6ms 프레임 예산(technical-preferences.md) 준수가 이 규칙의 근거이며, 기본/폴링 모드 선택은 Tuning Knobs의 `HUDUpdateMode`로 노출한다.
4. **프레임당 갱신 코얼레싱** — 같은 프레임에 동일 위젯으로 이벤트가 폭주해도(오버드라이브 중 대량 히트로 텐션 이벤트 다발 등) 위젯 리빌드/레이아웃 무효화는 프레임당 최대 1회. 마지막 값이 이긴다(중간값 표시 불필요 — 표시는 미러일 뿐).
5. **표시 보간은 게임플레이를 왜곡하지 않는다** — W6 게이지 바의 lerp는 순수 시각 스무딩이며, 임계 강조(Charged)·오버드라이브 발동 플래시 등 게임플레이 의미가 있는 신호는 항상 **실제 값/실제 이벤트** 기준으로 트리거한다(보간된 표시값 기준 금지). 판정 불확실성 방지(Health/Damage Core Player Fantasy 계약).
6. **처형 프롬프트 — 가정 인터페이스 (Core Extraction Execution 미설계)** — MVP 가정: 대상 적에게 `State.Executable` 태그(Health/Damage Core 소유, Dash/Evasion·Elite Shield가 부여)가 존재하면 해당 대상 위에 월드공간 처형 마커를 표시하고, 근접 조건 충족 시 화면 F키 프롬프트를 점멸시킨다. 근접 판정 기준·정확한 트리거 이벤트/데이터는 Core Extraction Execution GDD가 확정할 사항 — 그때까지 HUD는 태그 존재 여부만으로 최소 구현 가능하다고 가정하고, 확정 시 이 위젯의 바인딩만 교체한다(Open Questions 등록, handoff 지정 사항).
7. **오버드라이브 상태 전환** — `OnOverdriveStarted` 수신 시: 마나바를 "무한" 표시 상태로 전환(수치 대신 ∞/풀차지 고정 연출), 3속성 쿨다운 오버레이 강제 클리어 표시, HUD 액센트 컬러를 크림슨 레드로 전환, 잔여시간 카운트다운 노출. `OnOverdriveEnded` 수신 시 전부 원복. 화면 전역 크림슨 반전은 Luna Overdrive/Overdrive Visual State 소유 — HUD는 자기 위젯의 팔레트 전환만 소유.
8. **텐션 게이지 상태 언어** — Building(밝음/차오름 강조), Decaying(어두운 틴트 — "지금 새고 있다"를 플레이어가 인지해야 리스크 판단 성립, combo-tension-gauge.md UI Requirements 계약), Charged 근접 강조(게이지 ≥ `TensionChargedHighlightThreshold`×Max 시 펄스/글로우 — combo-tension-gauge.md가 "정확한 임계값 표시 로직은 Combat HUD 소유"로 위임한 값을 여기서 확정: 기본 0.9).
9. **사망 시 HUD 리셋** — 플레이어 Death 진입 시 처형 프롬프트/마커 즉시 제거, 오버드라이브 인디케이터 off, 게이지 표시 0 (상류 리셋 이벤트 미러링 — HUD가 자체 판단으로 리셋하는 게 아니라 상류 값 변화를 그대로 반영하되, 프롬프트류 잔상만은 즉시 제거를 명시).
10. **레이아웃/스타일 비소유** — 배치·크기·타이포·컬러 팔레트는 `/ux-design`(`design/ux/combat-hud.md`)과 Art Bible 소유. 이 문서가 소유하는 것은 위젯 존재, 데이터 계약, 갱신 정책, 상태 언어(무엇이 구분되어야 하는가)까지다.

### States and Transitions

| State | Entry Condition | Exit Condition | Behavior |
|---|---|---|---|
| Normal | 기본 상태 | `OnOverdriveStarted` 수신 | 전 위젯 일반 팔레트, 마나/쿨다운 실값 표시 |
| Overdrive | `OnOverdriveStarted` | `OnOverdriveEnded` | 크림슨 액센트, 마나바 ∞ 표시, 쿨다운 오버레이 클리어, 잔여시간 카운트다운 |
| ExecutePromptVisible (오버레이, W7) | `State.Executable` 보유 대상 존재 (+근접 시 화면 프롬프트) | 태그 소멸/소비 또는 대상 사망 | 월드 마커 + F키 프롬프트 점멸 |
| LowHealthWarning (오버레이, W2) | `OnHealthPercentCrossed(LowHealthWarningThreshold)` 하향 통과 | 동일 임계값 상향 통과 | 비네트/경고 상태 유지 |

### Interactions with Other Systems

- **Health/Damage Core** (상류) — W1/W2 바인딩. `State.Executable` 태그 존재 여부도 이 시스템의 UI Requirements가 노출(W7 프롬프트 트리거).
- **Spell Casting (base)** (상류) — W3/W4 바인딩. `CostBypass.Active` 태그는 spell-casting이 노출하지만, 오버드라이브 표시 전환은 Luna Overdrive의 Started/Ended 이벤트를 정규 신호로 사용(luna-overdrive.md UI Requirements 권장 — 의미가 더 명확, 중복 구독 회피).
- **Dash/Evasion** (상류) — W5 바인딩 (dash-evasion.md Dependencies 표가 이미 Combat HUD를 하류로 명기).
- **Combo/Tension Gauge** (상류) — W6 바인딩. 오버드라이브 트리거 순간의 화면 전역 플래시는 combo-tension-gauge.md Visual/Audio 소유(트리거 발행 순간 즉각 피드백) — HUD는 게이지 위젯 자체의 상태만.
- **Luna Overdrive** (상류) — W7 인디케이터 + Overdrive 상태 전환(Rule 7).
- **Core Extraction Execution** (상류, 미설계) — W7 프롬프트의 정밀 트리거 신호 소유 예정(Rule 6의 가정 인터페이스를 대체).
- **ux-designer / Art Bible** (후속) — 레이아웃/스타일 확정(Rule 10).

## Formulas

### Displayed Dash Charges

`DisplayedCharges = floor(CurrentCharges)`, `RechargeFraction = CurrentCharges - floor(CurrentCharges)`

**Variables:**
| Variable | Type | Range | Description |
|----------|------|-------|-------------|
| CurrentCharges | float | 0–MaxCharges(2) | Dash/Evasion의 런타임 차지 값(그쪽 공식 소유 — "정수 UI 표시는 floor"를 dash-evasion.md가 이미 명시, 이 문서는 그대로 채택) |

**Output Range**: DisplayedCharges 0–2(정수 핍), RechargeFraction 0.0–1.0(충전중 스택 게이지).
**Example**: CurrentCharges=1.4 → 핍 1개 점등 + 두 번째 핍 40% 채움.

### Cooldown Overlay Fraction (W4, per element)

`CooldownFraction[E] = RemainingCooldown[E] / CooldownDuration[E]`

**Variables:**
| Variable | Type | Range | Description |
|----------|------|-------|-------------|
| RemainingCooldown[E] | float (s) | 0–CooldownDuration[E] | spell-casting-base.md가 노출하는 남은 쿨다운 |
| CooldownDuration[E] | float (s) | Blackhole 6.0 / Fire 2.0 / Lightning 2.5 (spell-casting-base.md 소유값 재사용) | 속성별 쿨다운 총 길이 |

**Output Range**: 0.0(Ready)–1.0(방금 캐스트). 오버레이 스윕 각도/높이에 그대로 매핑.
**Example**: Blackhole 캐스트 3.0초 후 → 3.0/6.0 = 0.5 (오버레이 절반).

### Displayed Tension (시각 보간, W6)

`DisplayedTension += (TensionGauge - DisplayedTension) × min(1, ΔT × GaugeLerpSpeed)`

**Variables:**
| Variable | Type | Range | Description |
|----------|------|-------|-------------|
| TensionGauge | float | 0–TensionGaugeMax(100) | 실제 게이지 값(Combo/Tension Gauge 소유) |
| DisplayedTension | float | 0–TensionGaugeMax | 화면 표시용 보간값 — 순수 시각, 판정에 미사용(Rule 5) |
| ΔT | float (s) | 프레임 델타 | 보간 틱(게이지 변동 중에만 활성, Rule 3) |
| GaugeLerpSpeed | float | 5–20 (기본 10) | 보간 수렴 속도(Tuning Knob) |

**Output Range**: 0–100. 기본값 기준 실값과의 차이가 매초 e^-10 비율로 수렴(체감상 ~0.3초 내 따라붙음).
**Example**: 표시 60, 실값 95(Blackhole+Fire 연속 명중) → 다음 프레임(ΔT=0.016)에 60+(35×0.16)=65.6, 약 0.3초에 표시 94+.

### Charged Highlight Gate (W6)

`bChargedHighlight = TensionGauge >= TensionGaugeMax × TensionChargedHighlightThreshold`

**Variables:**
| Variable | Type | Range | Description |
|----------|------|-------|-------------|
| TensionChargedHighlightThreshold | float (ratio) | 0.80–0.95 (기본 0.90) | 임계 근접 강조 시작 비율 — combo-tension-gauge.md가 "예: 90%+"로 예시하고 소유를 이 문서에 위임한 값의 확정 |

**Output Range**: boolean. 판정은 실값(TensionGauge) 기준 — DisplayedTension 기준 금지(Rule 5).
**Example**: 기본값에서 게이지 90 도달 프레임에 펄스/글로우 시작.

## Edge Cases

| Scenario | Expected Behavior | Rationale |
|----------|-------------------|-----------|
| 오버드라이브 종료 프레임의 쿨다운 오버레이 | 종료 즉시 3속성 모두 Ready 표시(오버레이 없음) — 우회 중 쿨다운 진입 자체가 없었으므로(spell-casting-base.md Edge Cases) 표시할 잔존 쿨다운이 존재하지 않음 | "숨겨진 쿨다운" 표시가 생기면 상류 계약 위반을 HUD가 발명하는 꼴 — 상류 상태를 그대로 미러 |
| `State.Executable` 대상이 동시에 여럿 | 월드 마커는 대상별 전부 표시, 화면 F키 프롬프트는 1개만(최근접 대상 기준) | 다중 처형 기회는 카타르시스 요소(dash-evasion.md Edge Cases)라 마커는 전부 보여야 하나, 프롬프트 다중 표시는 시선 분산 — 근접 기준은 가정 인터페이스라 CEE 설계 시 재확정(Open Questions) |
| 태그 소멸(3초 만료/처형 소비/대상 사망)과 프롬프트 표시 경합 | 태그 제거 이벤트 수신 즉시 마커/프롬프트 제거 — 페이드아웃 등 잔상 연출 금지 | 프롬프트가 남아 있는데 `TryExecute`가 실패하는 "판정 불확실성"은 Health/Damage Core Player Fantasy 계약 위반 |
| 표시 보간(DisplayedTension)이 아직 따라가는 중에 실값이 Max 도달 → 오버드라이브 발동 | 발동 플래시/크림슨 전환은 실제 `OnOverdriveTriggered`/`OnOverdriveStarted` 이벤트 프레임에 즉시 — 표시값이 100에 도달하길 기다리지 않음. 게이지 표시는 리셋(0)을 향해 보간 | Rule 5 — 보간이 게임플레이 신호를 지연시키면 안 됨 |
| 같은 프레임 이벤트 폭주(오버드라이브 중 다단히트로 텐션/마나 이벤트 다발) | 프레임당 위젯 갱신 1회 코얼레싱, 마지막 값 표시(Rule 4) | 60fps 예산 보호 — HUD 리빌드가 프레임을 잡아먹으면 "학살이 끊김없이"라는 상류 판타지를 HUD가 깨는 역설 방지 |
| 플레이어 사망 프레임 | 프롬프트/마커 즉시 제거, 오버드라이브 인디케이터 off(`OnOverdriveEnded(PlayerDeath)` 수신), 게이지 0 미러링 — 재시작 후 Normal 상태에서 재구성 | Rule 9 — 리스폰 화면에 이전 전투의 잔상이 남으면 안 됨 |
| `OnHealthPercentCrossed` 하향/상향 통과가 한 프레임에 연속 발생(대미지+회복 겹침) | 마지막 통과 방향이 이긴다 — 최종 Health 값 기준 상태로 수렴(Rule 4의 마지막 값 우선과 동일 원칙) | 경고 비네트가 켜졌다-꺼졌다 깜빡이는 1프레임 노이즈 방지 |
| HUD 초기화(레벨 로드) 시점에 상류 값이 아직 미초기화 | 위젯은 첫 델리게이트 수신 전까지 숨김(0/빈 값 렌더 금지) — 초기화 완료 이벤트 수신 후 일괄 표시 | "체력 0/마나 0"으로 잘못 읽히는 첫 프레임 방지 — 미표시가 오표시보다 안전 |

## Dependencies

| System | Direction | Nature of Dependency |
|--------|-----------|----------------------|
| Health/Damage Core | 상류 (이 시스템이 의존, 하드) | `Health`/`MaxHealth` 델리게이트, `OnHealthPercentCrossed`, `State.Executable` 존재 여부 — 전부 그쪽 UI Requirements가 노출한 인터페이스 |
| Spell Casting (base) | 상류 (이 시스템이 의존, 하드) | `Mana`/`MaxMana` 델리게이트, 3속성 `OnCooldown`+남은 시간, `CostBypass.Active` 태그 |
| Dash/Evasion | 상류 (이 시스템이 의존, 하드) | `CurrentCharges` 값(floor 표시 계약 포함) |
| Combo/Tension Gauge | 상류 (이 시스템이 의존, 하드) | 게이지 값(0~Max) + Building/Decaying 상태 read-only 구독 |
| Luna Overdrive | 상류 (이 시스템이 의존, 하드) | `OnOverdriveStarted`/`OnOverdriveEnded(EndReason)`, `OverdriveTimeRemaining` |
| Core Extraction Execution (미설계) | 상류 (이 시스템이 의존, 소프트 — 가정 인터페이스) | 처형 프롬프트 정밀 트리거 신호(근접 판정 포함) — 확정 시 W7 바인딩 교체(Rule 6) |
| ux-design (`design/ux/combat-hud.md`, 미작성) | 후속 산출물 (이 문서를 입력으로 소비) | 레이아웃/배치/스타일 스펙 — 상류 GDD 3종의 UX Flag가 전부 이 화면을 지목 |

> 이 시스템에 의존하는 게임플레이 시스템은 없다 — 모든 상류 GDD가 "HUD 없어도 로직은 정상 동작"(소프트 하류)으로 명기(Presentation Layer, systems-index 일치).

## Tuning Knobs

| Knob | Default | Safe Range | Too High | Too Low |
|---|---|---|---|---|
| HUDUpdateMode | EventDriven | EventDriven / TickPolling(디버그 전용) | — (TickPolling은 프레임 예산 소모 — 60fps 검증/디버그 비교용으로만, 출하 금지) | — |
| GaugeLerpSpeed | 10 /s | 5–20 | 보간 체감 소멸(스냅) — 차오름 연출 상실 | 표시가 실값에 한참 뒤처짐 — 임계 직전 판단(큰 스펠 아낄지)이 표시 지연으로 왜곡 |
| TensionChargedHighlightThreshold | 0.90 | 0.80–0.95 | 강조가 발동 직전에만 켜져 예고 기능 상실 | 강조가 너무 일찍/오래 켜져 긴장 신호가 상시등화(둔감화) |
| LowHealthWarningThreshold | 0.25 | 0.15–0.40 | 경고가 너무 일찍 시작 — 상시 경고로 둔감화 | 죽기 직전에야 경고 — 대응 시간 없음 |
| ExecutePromptBlinkRate | 2.0 Hz | 1.0–4.0 | 점멸이 산만 — 시선 강탈 과다 | 점멸 인지 실패 — 3초 태그 윈도우(executable_duration) 내 기회 놓침 |

**교차 제약**: `ExecutePromptBlinkRate`는 `executable_duration`(3.0초, dash-evasion.md 소유)과 한 세트 — 태그 지속시간이 짧아지면 점멸 주기도 올려 최소 3~4회 점멸이 윈도우 안에 보이도록 유지할 것. `LowHealthWarningThreshold`는 Health/Damage Core의 `OnHealthPercentCrossed` 임계값 목록에 HUD가 등록을 요청하는 값이므로, 변경 시 코드가 아니라 이 노브만 바꾸면 되도록 구현할 것.

## Visual/Audio Requirements

> **Note**: `art-director`/`ux-designer` 미컨설트 — Solo 모드. 레이아웃/스타일 전부 `/ux-design` + Art Bible에서 확정(Rule 10). 아래는 상태 언어 요구까지만.

| Event | Visual Feedback | Audio Feedback | Priority |
|-------|-----------------|-----------------|----------|
| 텐션 Building | 게이지 밝은 채움 + 상승 방향 강조 | (게이지 상승 지속음은 combo-tension-gauge.md 소유 — HUD 자체 SFX 없음) | High |
| 텐션 Decaying | 게이지 어두운 틴트("새는 중" 인지) | — | High |
| Charged(≥90%) | 게이지 펄스/글로우 | — | High |
| 오버드라이브 진입/지속 | HUD 액센트 크림슨 전환, 마나바 ∞ 표시, 쿨다운 클리어, 잔여시간 카운트다운(마지막 3초 펄스 — luna-overdrive.md 경고 연출과 동기화) | (스팅어는 luna-overdrive.md 소유) | High |
| 처형 가능 | 대상 월드 마커(글리치 링 — dash-evasion.md 시안 계승) + F키 프롬프트 점멸 | (처형 대기 공명음은 dash-evasion.md 소유) | High |
| 저체력 | 화면 가장자리 붉은 비네트(health-damage-core.md Visual/Audio 항목의 실행 주체가 이 위젯) | (심장박동 루프는 health-damage-core.md 소유) | Medium |
| 캐스트 실패 피드백(아이콘 흔들림/딤) | spell-casting-base.md Open Question("거부 UX 완화") 결과를 따름 — HUD는 어느 쪽이든 표현만 담당 | — | Low |

> 📌 **Asset Spec** — Art Bible 승인 후 `/asset-spec system:combat-hud`로 구체 스펙화 필요. HUD 사운드는 전부 상류 시스템 소유로 정리됨(HUD 자체 발신 SFX 없음 — 중복 발음 방지).

## UI Requirements

이 문서 자체가 UI 시스템이다 — 구현 요구사항:

- **UMG + CommonUI 기반**(technical-preferences.md UI Specialist 라우팅: ue-umg-specialist). HUD 위젯 전부 non-focusable — CommonUI 입력 라우팅/포커스 그래프에 HUD가 끼어들지 않음을 구현 시 검증할 것(Rule 1).
- 키보드/마우스 ↔ 게임패드 전환 시 HUD 표시 차이 없음 — 단, F키 프롬프트의 키 글리프는 활성 입력 디바이스에 따라 교체(키보드: F / 패드: 해당 버튼 — technical-preferences.md "F키 처형 등 액션 입력은 게임패드 버튼 매핑 고려" 반영). 매핑 자체는 Core Extraction Execution/입력 설계 소유, HUD는 글리프 스왑만.
- 접근성 최소선: Building/Decaying 및 크림슨 전환은 색상만으로 구분하지 말 것(밝기/패턴 병행) — 색각 이상 대응. 상세 기준은 `/ux-review` 단계에서 검증.
- 레이아웃/앵커/세이프존은 `design/ux/combat-hud.md`(미작성) 소유 — 상류 3개 GDD의 UX Flag(health-damage-core, combo-tension-gauge, spell-casting-base)가 전부 이 화면을 지목하므로, Pre-Production에서 `/ux-design` 실행 시 이 GDD의 위젯 인벤토리(Rule 2 표)를 입력으로 사용할 것.

## Acceptance Criteria

1. **GIVEN** 플레이어 Health=100/100, **WHEN** RawDamage 25 피격, **THEN** 체력바가 해당 프레임의 어트리뷰트 변경 델리게이트 수신 즉시 75/100 반영(폴링 지연 없음).
2. **GIVEN** Blackhole 캐스트 직후(쿨다운 6.0s 진입), **WHEN** 3.0초 경과, **THEN** Blackhole 슬롯 오버레이가 fraction 0.5 표시, Fire/Lightning 슬롯은 오버레이 없음.
3. **GIVEN** CurrentCharges=1.4, **WHEN** 대쉬 차지 위젯 렌더, **THEN** 정수 핍 1개 점등 + 두 번째 핍 40% 충전 게이지 표시.
4. **GIVEN** TensionGauge 실값 88→92 상승, **WHEN** 92 도달 프레임, **THEN** Charged 강조(펄스)가 그 프레임에 시작(표시 보간값과 무관, 실값 기준 — 기본 임계 0.90).
5. **GIVEN** 텐션 게이지 Decaying 상태 진입, **WHEN** 위젯 렌더, **THEN** Building 상태와 시각적으로 구분되는 상태(어두운 틴트) 표시.
6. **GIVEN** `OnOverdriveStarted` 수신, **WHEN** 같은 프레임, **THEN** 마나바 ∞ 표시 전환 + 3속성 쿨다운 오버레이 클리어 + 크림슨 액센트 + 잔여시간 10.0 카운트다운 시작.
7. **GIVEN** `OnOverdriveEnded(Expired)` 수신, **WHEN** 같은 프레임, **THEN** 마나바 실값 표시 복귀 + 3속성 슬롯 전부 Ready 표시(잔존 쿨다운 오버레이 없음) + 일반 팔레트 복귀.
8. **GIVEN** 적 3명에게 동시에 `State.Executable` 부여(저스트회피), **WHEN** 프롬프트 표시, **THEN** 월드 마커 3개 + 화면 F키 프롬프트 정확히 1개(최근접 대상).
9. **GIVEN** `State.Executable` 태그가 만료(3.0s)로 제거됨, **WHEN** 제거 이벤트 수신 프레임, **THEN** 해당 마커/프롬프트가 같은 프레임에 제거(페이드 잔상 없음).
10. **GIVEN** 오버드라이브 중 1프레임에 텐션 변경 이벤트 5회 수신, **WHEN** 프레임 종료, **THEN** 게이지 위젯 갱신은 1회만 수행되고 마지막 값이 표시됨(코얼레싱 검증 — 성능 카운터로 확인).
11. **GIVEN** 플레이어 Death 진입(오버드라이브 중), **WHEN** 사망 프레임, **THEN** 프롬프트/마커 제거 + 오버드라이브 인디케이터 off + 게이지 0 표시.
12. **GIVEN** 게임패드 연결로 활성 입력 디바이스 전환, **WHEN** 처형 프롬프트 표시 중, **THEN** 키 글리프가 F에서 해당 패드 버튼 글리프로 교체, 그 외 HUD 동작 차이 없음.

## Open Questions

- **Owner: game-designer, Target: Core Extraction Execution GDD 설계 시** — 처형 프롬프트의 실제 이벤트명/데이터(근접 판정 기준 포함) 확정 필요. 현재 W7은 `State.Executable` 태그 존재 + 근접 가정만으로 설계된 가정 인터페이스(Rule 6) — CEE 확정 시 바인딩 교체 및 "화면 프롬프트 1개(최근접)" 규칙 재검토(handoff 명시 사항).
- **Owner: ux-designer, Target: Pre-Production `/ux-design`** — 레이아웃/앵커/세이프존/스타일 전부 미정 — 이 GDD의 위젯 인벤토리(Rule 2)를 입력으로 `design/ux/combat-hud.md` 작성 필요. 상류 3개 GDD의 UX Flag가 전부 이 화면을 대기 중.
- **Owner: gameplay-programmer, Target: 구현 시** — 활성 입력 디바이스 감지 및 키 글리프 스왑(F↔패드 버튼)은 UE5.8 통합 Input System(Enhanced+Common Input 통합 — VERSION.md) 기준으로 재검증 필요. 엔진 버전이 학습 데이터보다 최신이므로 `ue-umg-specialist` 헤더 대조 필수.
- **Owner: performance-analyst, Target: 프로토타입 계측** — 이벤트 코얼레싱(Rule 4)으로 충분한지, 오버드라이브 풀부하(대량 히트 + 파괴 + HUD 갱신 동시)에서 HUD 몫의 프레임 예산 실측 필요 — systems-index.md High-Risk의 60fps 미검증 항목과 동일 계열.
- **Owner: qa-lead, Target: Production 진입 전** — 위 Acceptance Criteria는 solo 모드로 작성, qa-lead 미검증.

> **Note**: `ue-umg-specialist`/`ux-designer`/`art-director`/`qa-lead` 미컨설트 — Solo 모드. Production 전 수동 리뷰 필요.
