# 기능정의서: Moon Fragment Hunt — 전투 시스템 확장

> **Status**: Draft
> **Created**: 2026-07-21
> **Source**: design/gdd/game-concept.md, design/gdd/systems-index.md, design/gdd/*.md (9개 승인 문서)
> **Scope**: `systems-index.md`에 정의된 전투 시스템 확장(Dopamine Driven Design) 범위. Progression/Economy/Persistence/Narrative/Audio 카테고리는 제외.

---

## 1. 문서 개요

이 문서는 개별 시스템 GDD(design/gdd/*.md, 각 8섹션 포맷)에 분산된 기능들을 **기능 단위(feature-level)** 로 재정리한 통합 요약이다. GDD가 "왜/어떻게 설계했는가"를 다룬다면, 이 문서는 "무엇이 있고 지금 어떤 상태인가"를 빠르게 훑기 위한 참조용이다.

- 상세 수치의 안전범위(Safe Range), 공식 유도과정, Edge Case 전체 목록은 반드시 원본 GDD를 참조할 것 — 이 문서는 요약이며 원본을 대체하지 않는다.
- 상태 표기: **Approved**(설계완료, 리뷰통과) / **Not Started**(미설계, TBD)

---

## 2. 게임 컨셉 요약

**핵심 테마**: Dopamine Driven Design — 콤보 누적 → 루나 오버드라이브(각성) → 스펠 위빙 시너지 → 환경 연쇄파괴 → 코어 적출 처형 → 보스전 아레나 몰핑으로 이어지는 텐션 곡선 하나를 완성하는 것이 설계 목표.

**텐션 곡선**: 전투 진입 → 스펠 위빙 콤보(텐션 상승) → 일시 하락(완급조절) → 루나 오버드라이브(최고점, 각성) → 급격한 하락(쿨다운).

**디자인 목표**: Endless Catharsis — 파괴의 스케일을 넓히고 보상을 가시화하여 유저가 손을 뗄 수 없는 액션 루프 완성.

자세한 컨셉 설명(초신성/플라즈마스톰 시너지, 그린패스 포자, 신의 다리 붕괴, 아레나 몰핑 등 서사적 설명)은 `design/gdd/game-concept.md` 참조.

---

## 3. 시스템별 기능정의 (승인 9개)

### 3.1 Player Movement — `design/gdd/player-movement.md` · Foundation · MVP · Approved

| 기능명 | 트리거 | 효과/동작 | 핵심 수치·공식 |
|---|---|---|---|
| 카메라-상대 이동 입력 | WASD/좌스틱 | 카메라 Yaw 기준 forward/right 벡터 변환, 대각선 1.0 클램프 | `EffectiveSpeed = clamp(InputMagnitude,0,1) × MaxWalkSpeed`, MaxWalkSpeed=600uu/s |
| 즉시 회전 스냅(Facing) | 컨트롤러 Yaw 변화 | 인터폴 없이 즉시 스냅 | `bOrientRotationToMovement=false`, `bUseControllerRotationYaw=true` |
| 지상 이동(즉시가속/즉시정지) | 이동입력 | 33ms 이내 시작/정지 반응 | MaxAcceleration 18000uu/s², BrakingDecelerationWalking 18000, GroundFriction 16 |
| 싱글 점프 + 공중 조향 | 점프입력(Grounded만) | 다중점프 없음 | JumpZVelocity 600uu/s, AirControl 0.6, AirTime≈1.2245s (Joint Bound 0.5~3.0s) |
| 코요테 타임 | 낭떠러지 이탈 후 150ms 내 점프 | Grounded 점프로 취급 | 150ms |
| 점프 입력 버퍼 | 착지 전 150ms 내 점프입력 | 착지 즉시 소비 | 150ms |
| 착지 히트스탑 | 착지 순간 | 애니/카메라 레이어만 프리즈, 게임플레이 틱 정상 | 40ms |
| 이동-캐스팅 독립성 (Core Rule 7) | 캐스팅/처형 연출 중 | `AddMovementInput` 항상 활성 | `MovementLocked`는 Status Effect만 설정 가능 |

**Edge Cases 요지**: 수치 하드클램프 위반 시 로드거부/되돌림, 코요테 경계값 149/150/151ms, Falling→Ascending 재진입 시 외부 임펄스 처리.

**Dependencies**: Camera System(양방향) / Spell Casting, Dash-Evasion, Status Effect(미설계), Arena Morphing(미설계) — 하류.

---

### 3.2 Camera System (base) — `design/gdd/camera-system-base.md` · Core · MVP · Approved

| 기능명 | 트리거 | 효과/동작 | 핵심 수치 |
|---|---|---|---|
| 회전 입력 매핑 | `IA_Look` | Yaw/Pitch AddControllerInput | Pitch 클램프 -60°~30° |
| 스프링암 트래킹 래그 | 캐릭터 이동 | Yaw/Pitch 추적, Roll 미추적 | CameraLagSpeed 18.0, CameraLagMaxDistance 60uu |
| 환경 충돌 처리 | 카메라-지형 겹침 | 파괴물은 Ignore 처리 | ProbeSize 12uu, `ECC_Camera` |
| 오버드라이브 FOV 확장 | Overdrive 시작/종료 | 90°↔100° | InterpSpeed 6.0(약0.5s) / 4.0(약0.8s) |
| 처형 컷신 전환 | 처형 발동 | 클로즈업, Look Input 차단 | ArmLength 450→150uu, 0.2s 블렌드 / 복구 0.3s |
| 카메라 쉐이크 | 피격/초신성/저스트회피 | 각기 다른 진폭·감쇠 프로필 | — |

**Edge Cases 요지**: 좁은 모퉁이 클리핑 시 캐릭터 디더링, 텔레포트 시 `ResetCameraLag()` 필수, Launch 3000uu/s+ 오프스크린 방지.

**⚠ 문서 내 수치 불일치**: Arena Morphing 하류 의존 항목이 CameraLagMaxDistance 200uu를 언급하나 본문 Rule은 60uu — Arena Morphing 설계 시 확인 필요.

**Dependencies**: Player Movement(양방향) / Dash-Evasion, Core Extraction Execution, Arena Morphing(모두 미설계) — 하류.

---

### 3.3 Health/Damage Core — `design/gdd/health-damage-core.md` · Foundation · MVP · Approved

| 기능명 | 트리거 | 효과/동작 | 핵심 수치·공식 |
|---|---|---|---|
| 단일 데미지 진입점 | 모든 데미지 소스 | 1개 `ApplyDamage`(GameplayEffect) 경로로 통일 | 방어감쇄 공식 없음(불리언 인터셉트만) |
| 실드/아머 인터셉트 | 실드 보유 대상 피격 | 전량 흡수(부분감쇄 없음) | — |
| 방어무시(bBypassDefense) | 처형 등 | 인터셉트 우회, Health 직접 적용 | — |
| 즉시 사망 판정 | `Health<=0` | 해당 프레임 즉시 Death | — |
| 무적 게이트 | `State.Invulnerable` 부여 | 데미지 적용 자체 차단 | 참조카운트 방식 |
| 처형 가능 상태 | `State.Executable` 태그 | `TryExecute(Target)` 성공 시 방어무시 즉사 | 태그 부여자: Dash/Evasion, Enemy Elite Shield(미설계)만 |
| HP% 임계값 이벤트 | 체력 비율 통과 | `OnHealthPercentCrossed(threshold)` | Boss Phase(미설계)가 소비 예정 |

**공식**: `EffectiveDamageApplied = IsInvulnerable?0 : (HasActiveShield&&!bBypassDefense)?0 : RawDamage`. MaxHealth(Player) 기본 100.

**Edge Cases 요지**: 이미 사망한 대상 중복데미지 무시, bBypassDefense도 Invulnerable은 못 뚫음, Executable 태그 보유 대상이 일반사망하면 태그 자동소멸(처형보상 없음).

**Dependencies**: 이 시스템에 상류 의존 없음(Foundation). 하류: Enemy AI, Spell Casting, Status Effect(미설계), Dash-Evasion, Enemy Elite Shield(미설계), Core Extraction Execution(미설계), Combo/Tension Gauge, Boss Phase(미설계), Luna Overdrive.

---

### 3.4 Enemy AI (base) — `design/gdd/enemy-ai-base.md` · Core · MVP · Approved

| 기능명 | 트리거 | 효과/동작 | 핵심 수치 |
|---|---|---|---|
| 아키타입 2종 | — | Grunt(근접)/Ranged(원거리) | Grunt MaxHealth 30, Ranged 20 |
| 인지(시야+청각) | 시야 원뿔/소음 | AIPerception 기반 | Grunt 청각반경 800uu, Ranged 600uu |
| 공격 텔레그래프 | 윈드업 AnimNotify | `OnAttackTelegraphed`→`OnAttackCommitted` | TelegraphWindow 기본 0.4s |
| Ranged 거리밴드 유지 | 플레이어와의 거리 | 후퇴/공격 판정 | MinEngageRange 400uu, MaxEngageRange 1200uu |
| 사망 처리 | Health/Damage Core `OnDeath` | 충돌/인지 비활성화→래그돌→디스폰 | CorpseHoldDuration 4초 |
| Stagger 슬롯 | 외부 호출(`TriggerStagger`/`ClearStagger`) | 상태 전환만 소유, 트리거 조건은 미소유 | — |

**States**: Idle→Alert(5초 타임아웃)→Chase→Attack→Stagger→Dead

**Edge Cases 요지**: 텔레그래프 후 Stagger/Death 시 공격 커밋 취소, Sight 이탈해도 이미 시작된 공격은 커밋까지 진행, 다수 적 그룹협응 없음(MVP 범위 밖).

**Dependencies**: Health/Damage Core(상류) / Dash-Evasion, Enemy Elite Shield(미설계), Super Armor/CC Interrupt(미설계), Core Extraction Execution(미설계), Boss Phase(미설계, 간접) — 하류.

---

### 3.5 Spell Casting (base) — `design/gdd/spell-casting-base.md` · Core · MVP · Approved

| 기능명 | 트리거 | 효과/동작 | 핵심 수치·공식 |
|---|---|---|---|
| 3속성 고정 슬롯 | 각 속성 입력 | 블랙홀/화염/번개 독립 바인딩 | ManaCost: Blackhole 70, Fire 25, Lightning 25 |
| 즉시발동 | 코스트+쿨다운 통과 | 위액/차지 없이 동일 프레임 완결 | — |
| 이동 불간섭 | 캐스팅 중 | `AddMovementInput` 100% 정상 | — |
| 자유 캔슬/전환 | 다른 스펠/행동 입력 | GAS 캔슬 API 미사용, 프레젠테이션만 캔슬 | — |
| 단일 마나 자원 | 캐스트/시간경과 | 소비+회복 | Mana/MaxMana 100, ManaRegenRate 8/s + 코어적출 100% 스냅 |
| 속성별 개별 쿨다운 | 캐스트 성공 | 속성마다 독립 쿨다운 | Blackhole 6.0s, Fire 2.0s, Lightning 2.5s |
| 소음 발생 의무 | 임팩트 | `MakeNoise` 호출 | NoiseLoudness Blackhole 1.0 > Lightning 0.7 > Fire 0.6 |
| 코스트/쿨다운 우회 훅 | `CostBypass.Active` 태그(Luna Overdrive 전용) | 마나소비+쿨다운 동시 우회 | — |
| 캐스트 레이트 바닥 (Rule 11) | 프레임당 캐스트 | 속성당 프레임당 1회 + 전역 상한 | `MaxCastsPerSecond` 기본 20 |

**교차 제약**: ① `ManaCost>Regen×Cooldown`(무한스팸 방지) ② 쿨다운이 Blackhole 실질제약 ③ `MaxMana-BlackholeCost≥max(Fire,Lightning)` → 100-70=30≥25 (스펠위빙 1회 보장)

**Edge Cases 요지**: Mana==Cost 정확일치 시 캐스트 가능, 오버드라이브 중 캐스트는 쿨다운 진입 자체 없음.

**Dependencies**: Player Movement, Health-Damage Core(상류) / Combo-Tension Gauge, Spell Weaving(미설계), Luna Overdrive — 하류.

---

### 3.6 Dash/Evasion — `design/gdd/dash-evasion.md` · Core · MVP · Approved

| 기능명 | 트리거 | 효과/동작 | 핵심 수치·공식 |
|---|---|---|---|
| 차지 기반 대쉬 | 대쉬 입력 | 차지 1 소비 | MaxCharges 2, RechargeRate 2.0s |
| 즉시 위치 이동 | 대쉬 입력 | 카메라 기준 방향(무입력 시 정면) | DashDuration 0.08s, 대쉬거리 = 600×6.25×0.08 = 300uu |
| 지상/공중 사용(Air-Dash) | 체공 중 대쉬 | 낙하방지 임펄스 포함 | — |
| 임펄스 Override | 대쉬 발동 순간 | 기존 velocity 완전 덮어쓰기(가산 아님) | — |
| 회피 프레임(I-frame) | 대쉬 시작 | `State.Invulnerable` 부여/해제 | DashInvulnDuration 0.08s |
| 저스트회피 판정 | 적 공격 커밋 직전 대쉬 | `IsJustDodge` 판정 | JustDodgeWindow 0.2s |
| 저스트회피 보상 | 저스트회피 성공 | 조건만족 전원에 `State.Executable` 부여 + 차지 1 반환(상한1) | ExecutableDuration 3.0s |

**교차검증**: 대쉬거리 300uu < MinEngageRange 400uu / MinTelegraphWindow(0.4s) > JustDodgeWindow(0.2s) → 회피 항상 성립 가능.

**Edge Cases 요지**: 다수 적 겹침 시 조건만족 전원 Executable 부여, `MovementLocked` 시 대쉬 불가.

**Dependencies**: Player Movement, Health-Damage Core, Enemy AI, Camera System(상류) / Combat HUD(하류).

---

### 3.7 Combo/Tension Gauge — `design/gdd/combo-tension-gauge.md` · Feature · MVP · Approved

| 기능명 | 트리거 | 효과/동작 | 핵심 수치·공식 |
|---|---|---|---|
| 단일 GAS Attribute | — | `TensionGauge` 0~100, 플레이어 직접조작 불가 | TensionGaugeMax 100 |
| 획득(스펠명중) | `OnSpellHit` | 마나코스트 비례 획득 | `TensionGain=ManaCost×TensionGainCoefficient(1.0)` |
| 획득(저스트회피) | `OnTagAdded(State.Executable)` | 다수타겟이어도 1회만 | `JustDodgeTensionBonus`=20 |
| 감쇠(Decay) | 획득 없음 3초 경과 | 감쇠 시작 | TensionDecayGracePeriod 3.0s, Rate 10/s |
| 피격 페널티 | `OnDamageApplied` | 유효데미지 시 텐션 비례감소 | `Tension×(1-0.20)` |
| 오버드라이브 트리거 | Max 도달 | `OnOverdriveTriggered` 1회 발행 + 즉시 0 리셋 | — |
| 오버드라이브 중 획득 감쇠 (Rule 7) | Overdrive Active 중 스펠명중 | 리프레시 체인 억제 | `OverdriveTensionGainMultiplier` 0.4 (2026-07-18 재리뷰로 1.0→0.4 변경) |

**공식 예시**: Blackhole명중(70×1.0=70)→+Fire(25)=95.

**Edge Cases 요지**: Max초과분 버림(이월없음), Gain→Penalty→Decay 순서 고정, 사망 시 즉시 0 강제리셋.

**Dependencies**: Spell Casting, Health-Damage Core(상류,하드) / Dash-Evasion(상류,소프트) / Luna Overdrive(하류,하드) / Combat HUD(하류,소프트).

---

### 3.8 Luna Overdrive (Blood Moon) — `design/gdd/luna-overdrive.md` · Feature · MVP · Approved

| 기능명 | 트리거 | 효과/동작 | 핵심 수치·공식 |
|---|---|---|---|
| 트리거 소비 | `OnOverdriveTriggered` 수신 | Active 진입 | — |
| 발동 효과 | Active 진입 | `CostBypass.Active` 태그 부여(마나무한+쿨타임제로) | 별도 데미지/이속 배율 없음 |
| 지속시간 타이머 | Active 진입 | `OverdriveEndTime=CurrentTime+Duration` | OverdriveDuration 기본 10.0s |
| 종료(태그해제) | 시간경과 | lazy 시간비교로 판정 순서 무관 동일결과 | — |
| Active 중 재트리거=리프레시 | Active 중 재도달 | 타이머만 Duration 풀로 리셋, 상한 없음(체인 허용) | — |
| 플레이어 사망 시 강제종료 | 플레이어 Death | 타이머취소+태그즉시해제 | `OnOverdriveEnded(PlayerDeath)` |
| 상태 이벤트 노출 | — | HUD/연출 소비용 | `OnOverdriveStarted/Ended`, `OverdriveTimeRemaining` |

**⚠ 알려진 설계 리스크**: TensionGainCoefficient가 안전상한(1.5)이면 Blackhole 1발(105)로 매캐스트 리프레시 가능 — Duration 자체가 무의미해질 위험. 억제 레버(`OverdriveTensionGainMultiplier`)는 Combo/Tension Gauge 문서가 소유, 이 문서는 상한을 규정하지 않음 — **연동 확인 필요**.

**Edge Cases 요지**: 마나고갈로 인한 조기종료 경로 없음, 트리거+사망 동프레임 시 Death 우선.

**Dependencies**: Combo/Tension Gauge, Spell Casting(상류,하드) / Health-Damage Core(상류,소프트) / Combat HUD(하류,소프트) / Overdrive Visual State(미설계,하류,하드).

**거대파편 흡수 트리거는 MVP 범위 밖**(미설계).

---

### 3.9 Combat HUD — `design/gdd/combat-hud.md` · UI · MVP · Approved

순수 Presentation Layer — 표시만 담당, 게임플레이 판정 소유 없음. 레이아웃/스타일은 `design/ux/combat-hud.md`가 소유.

| 위젯 | 데이터 소스 | 표시 내용 | 핵심 수치 |
|---|---|---|---|
| W1 체력바 | Health/Damage Core | `Health`/`MaxHealth` | — |
| W2 저체력 경고 | Health/Damage Core | `OnHealthPercentCrossed` | LowHealthWarningThreshold 0.25 |
| W3 마나바 | Spell Casting | `Mana`/`MaxMana` + 무한마나 표시 | `CostBypass.Active` |
| W4 스펠슬롯×3 쿨다운 | Spell Casting | 속성별 쿨다운 오버레이 | `CooldownFraction=Remaining/Duration` |
| W5 대쉬 차지 | Dash/Evasion | 차지 수 + 리차지 진행률 | `DisplayedCharges=floor(CurrentCharges)` |
| W6 텐션 게이지 | Combo/Tension Gauge | 값 + Building/Decaying 상태 | ChargedHighlight 임계 0.90 |
| W7 오버드라이브/처형프롬프트 | Luna Overdrive, `State.Executable` | 오버드라이브 인디케이터 + F키 프롬프트 | ExecutePromptBlinkRate 2.0Hz (CEE 미설계, 가정 인터페이스) |

**갱신 정책**: 이벤트기반(틱폴링 금지), 프레임당 1회 코얼레싱, 보간은 표시전용(판정은 실값 기준).

**Edge Cases 요지**: 오버드라이브 종료 시 쿨다운 오버레이 즉시 클리어, HUD 초기화 전 값은 숨김(0 오표시 금지), 사망 시 프롬프트/게이지 즉시 리셋.

**Dependencies**: 승인 5개 시스템 전부(상류,하드) + Core Extraction Execution(상류,소프트,가정) — 이 시스템에 의존하는 게임플레이 시스템 없음.

---

## 4. 미설계 시스템 (11개, TBD)

`systems-index.md` 기준, GDD 미작성 상태. 실제 세부 기능은 설계 착수 시 `/design-system [system-name]`으로 정의.

| # | 시스템명 | 카테고리 | 목표 마일스톤 | 알려진 기능 가정(컨셉 문서 기준, TBD) | 의존 시스템 |
|---|---|---|---|---|---|
| 5 | Destructible Geometry | Core | Vertical Slice | 포자/다리 기둥 등 파괴 지오메트리, Nanite per-fragment 완료(스파이크 검증됨) | — |
| 8 | Status Effect | Core | Vertical Slice | `MovementLocked` 등 상태이상 부여 권한 소유 예정 | Health/Damage Core |
| 10 | Spell Weaving / Synergy | Feature | Vertical Slice | 블랙홀+화염=초신성(방어무시 광역), 블랙홀+번개=플라즈마스톰(다단히트 스턴) | Spell Casting, Status Effect |
| 12 | Core Extraction Execution | Feature | Vertical Slice | F키 처형, `State.Executable` 소비, 성공 시 마나 100% 즉시회복(Spell Casting 소유) | Dash-Evasion, Enemy AI, Health-Damage Core |
| 13 | Enemy Elite Shield | Feature | Alpha | 실드 파괴 시 처형 조건 부여(Core Extraction Execution과 트리거 조건 충돌 리스크 — 이미 식별됨) | Enemy AI, Health-Damage Core |
| 14 | Super Armor / CC Interrupt | Feature | Alpha | 보스 슈퍼아머 패턴, CC(스턴 등)로 인터럽트 | Status Effect, Enemy AI, Health-Damage Core |
| 15 | Environmental Chain-Destruction | Feature | Vertical Slice | 그린패스 포자 연쇄폭발(독데미지), 신의 다리 붕괴(환경킬) | Destructible Geometry, Status Effect, Health-Damage Core |
| 16 | Boss Phase | Feature | Alpha | HP% 임계값(`OnHealthPercentCrossed`) 소비, 페이즈 전환 | Enemy AI, Health-Damage Core |
| 17 | Arena Morphing | Presentation | Alpha | 2페이즈 진입 시 바닥 붕괴→상승 발판, 체공전투, 파편낙하 엄폐 (60fps 예산 미검증 — 리스크 항목) | Boss Phase, Destructible Geometry, Dash-Evasion, Camera System |
| 18 | Overdrive Visual State | Presentation | Vertical Slice | 크림슨 레드 반전(맵 라이팅/이펙트) | Luna Overdrive |
| 19 | Execution Camera/Cutscene | Presentation | Vertical Slice | 처형 클로즈업 연출(Camera System base 훅 이미 존재) | Core Extraction Execution, Camera System |

**설계 순서 권장**: systems-index.md의 Recommended Design Order 참조 (Status Effect → Destructible Geometry → Spell Weaving → Core Extraction Execution → Environmental Chain-Destruction → Overdrive Visual State → Execution Camera → Enemy Elite Shield → Super Armor/CC → Boss Phase → Arena Morphing).

---

## 5. 공용 GameplayTag 레지스트리

여러 시스템에 걸쳐 반복 사용되는 태그. 통합 정의는 `Moon/Config/DefaultGameplayTags.ini` 참조.

| 태그 | 부여자(Grantor) | 소비자(Consumer) | 방식 |
|---|---|---|---|
| `State.Invulnerable` | Dash/Evasion(회피 프레임) | Health/Damage Core(데미지 차단 게이트) | 참조카운트 |
| `State.Executable` | Dash/Evasion(저스트회피), Enemy Elite Shield(미설계) | Health/Damage Core(`TryExecute`), Combat HUD(프롬프트), Combo/Tension Gauge(보너스 획득) | 참조카운트 |
| `CostBypass.Active` | Luna Overdrive | Spell Casting(코스트/쿨다운 우회) | 단일 grantor, 비카운트형 set/clear |
| `Enemy.Archetype.Grunt` / `Enemy.Archetype.Ranged` | Enemy AI(스폰 시) | Enemy Elite Shield(미설계) | 정적 |
| `Spell.Element.Blackhole` / `Fire` / `Lightning` | Spell Casting(슬롯 바인딩) | Spell Weaving(미설계) | 정적 |

---

## 6. 시스템 의존관계 요약

전체 의존 그래프(Foundation → Core → Feature → Presentation → Polish 5계층)는 `design/gdd/systems-index.md`의 Dependency Map 섹션이 원본이다 — 이 문서에서 중복 기술하지 않음.

**핵심만 요약**:
- Foundation(무의존): Player Movement, Health/Damage Core, Destructible Geometry(미설계)
- 순환 의존 없음(검증됨, systems-index.md 참조)
- 알려진 설계 충돌 리스크 2건: (1) Core Extraction Execution ↔ Enemy Elite Shield의 처형 트리거 조건 중복 — Core Extraction Execution GDD가 단일 소유 예정. (2) Luna Overdrive의 리프레시 체인 상한을 Combo/Tension Gauge가 소유하나 두 문서 간 연동이 아직 교차검증되지 않음(본 문서 §3.8 참조).

---

## 7. 변경 이력

| 날짜 | 변경 내용 |
|---|---|
| 2026-07-21 | 최초 작성 — 승인된 9개 GDD 기반 기능 통합 요약 |
