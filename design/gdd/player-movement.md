# Player Movement

> **Status**: Approved
> **Author**: user + game-designer
> **Last Updated**: 2026-07-16
> **Last Verified**: 2026-07-16
> **Implements Pillar**: (no game-pillars.md yet — physical foundation for Dopamine Driven Design tension curve: dash, air-dash, aerial combat)
> **Creative Director Review (CD-GDD-ALIGN)**: APPROVED 2026-07-16 — Verdict: APPROVED (Gemini Antigravity design-review)

## Summary

Player Movement은 캐릭터의 이동(걷기/달리기/점프/대쉬)을 담당하는 기초 로코모션 시스템이며, Dash/Evasion, Spell Casting, Arena Morphing 등 이후 모든 전투 시스템이 이 위에 얹힌다. `CharacterMovementComponent` 기반이지만 관성 램프를 최소화한 즉각반응형 이동으로 설계, 스펠 위빙 콤보와 대쉬캔슬이 끊김없이 이어지게 한다.

> **Quick reference** — Layer: `Foundation` · Priority: `MVP` · Key deps: `None`

## Overview

Player Movement는 이 게임의 물리적 전투 기반이다. 데이터/인프라 관점에서는 `CharacterMovementComponent`를 확장해 걷기·달리기·점프·기본 대쉬를 처리하는 로코모션 레이어이며, MaxWalkSpeed/JumpZVelocity/AirControl 등 표준 파라미터로 튜닝된다. 하지만 플레이어에게는 결코 배경 인프라가 아니다 — 콤보 위빙 중 순간 방향전환, 저스트회피용 대쉬 타이밍, 아레나 몰핑 최종장의 Z축 체공 전투 전부가 이 시스템이 제공하는 "손맛"에 직접 의존한다. Foundation 레이어 시스템이지만, 이동 자체의 무게감/스퀄니스가 콘셉트 문서의 핵심 목표인 텐션 곡선(전투진입→콤보상승→오버드라이브 폭발)이 실제로 "느껴지게" 만드는 촉각적 뼈대다. 이 시스템 없이는 이후 어떤 스펠도, 처형도, 붕괴 연출도 체감되지 않는다.

## Player Fantasy

플레이어는 무겁지 않고 즉각 반응하는 "마법 검객"처럼 움직여야 한다 — 대쉬는 즉시 방향전환되고 감속 램프 없이 딱 멈추며, 이동이 스펠 캐스팅을 절대 가로막지 않아 "멈춰서 캐스팅"이 아니라 "움직이며 학살"하는 느낌을 준다. Foundation 인프라층(CharacterMovementComponent 파라미터)이 이 즉각성을 뒷받침하고, 그 위 조작층(대쉬/에어대쉬/체공기동)이 플레이어가 실제로 손에서 느끼는 감각이다.

**Feel Reference**: *NieR:Automata*의 대쉬-회피 캔슬 반응성 (입력 즉시 반영, 관성 없음) + *Dead Cells*의 감속 램프 없는 즉시정지 대쉬.
**Anti-reference**: Souls류의 무겁고 커밋된 회피(회피 후 강제 recovery 프레임)는 지향하지 않음 — 이 게임은 "Endless Catharsis"가 목표라 이동이 플레이어를 절대 막아서면 안 됨 (game-concept.md 디자인 목표와 직결).

> `creative-director` 리뷰 완료(`/design-review` full mode, 2026-07-16) — game-designer/systems-designer/qa-lead/unreal-specialist 병렬 검토 후 종합. NEEDS REVISION → 이 리비전으로 blocking 항목 반영.

## Detailed Design

### Core Rules

1. 이동 입력은 **카메라 기준(camera-relative)** 으로 처리 — W/A/S/D 또는 좌스틱은 카메라의 forward/right 축에 매핑됨 (액터 자체 forward 아님). 대각선 입력(예: W+D 동시)이 합산된 벡터의 크기는 **1.0으로 클램프/정규화**됨 — `InputMagnitude`가 직선 입력보다 커지는 것(√2배 과속)을 방지.
2. 캐릭터 방향(facing)은 이동 방향과 **독립** — `bOrientRotationToMovement = false` **and** `bUseControllerRotationYaw = true` (둘 다 필요 — 후자 없이는 전자만으로 카메라 방향을 바라보지 않음), 캐릭터는 항상 카메라 요(yaw) 방향을 바라봄. **`bUseControllerRotationPitch`/`bUseControllerRotationRoll`은 명시적으로 false 유지**(design-review 2026-07-16 추가 — Yaw만 지정되어 있던 기존 서술은 Pitch/Roll 처리를 암묵적으로 남겨둬 향후 다른 시스템이 실수로 켤 경우 캐릭터 메시가 카메라 피치를 상속하는 회귀 위험이 있었음). 회전은 **즉시 스냅**(인터폴레이션 없음, `RotationRate` 미적용) — 매 틱 컨트롤러 yaw를 즉시 반영, "즉각반응형" 판타지와 일관성 유지. 스트레이프하면서도 정면으로 스펠을 조준할 수 있어야 하기 때문.
3. 지상 이동은 아날로그 입력 크기에 비례한 연속 속도(단일 이동 시스템, 걷기/달리기 토글 없음) — 최대 `MaxWalkSpeed`까지. 가속/감속은 `MaxAcceleration`/`BrakingDecelerationWalking`/`GroundFriction` 튜닝값으로 **의도적으로 매우 높게** 설정해 사실상 즉시가속·즉시정지를 구현 (표준 CMC 기본값으로는 이 목표 달성 불가 — Tuning Knobs 참조).
4. 점프는 기본 **싱글 점프만**. `JumpZVelocity` 고정값, `AirControl`로 공중 방향 보정 허용(전투 중 리포지셔닝 목적) — 다중 점프는 기본 키트에 없음. **공중 이동은 지상과 동일한 반응성 철학을 따름**: `AirControl` 상향 + `BrakingDecelerationFalling`(기본 5000uu/s², 2026-07-16 상향)로 입력 0일 때도 지상에 근접한 크리스프한 감속 적용 — 완전한 관성 유지(에어브레이킹 전무)는 지양하되, 지상-공중 손맛 격차는 대폭 축소(600uu/s 정지시간 기준 지상 33ms 대비 공중 약 120ms). Souls류 "커밋된 공중 궤적"으로 읽히지 않게 함. Arena Morphing의 체공전투 페이싱이 이 기반 위에 얹히므로 base 레이어부터 반응성을 보장.
5. 낙하/중력은 표준 `GravityScale` 적용, 이 레이어에서 종단속도 캡 변경 없음 (Arena Morphing의 체공전투용 오버라이드는 해당 시스템 GDD 소유). `GravityScale`은 데이터애셋 로드 시 **최소값 0.1로 하드 클램프**됨(코드 강제, 마크다운 Safe Range는 권고치일 뿐 별도) — 0 또는 음수값으로 인한 AirTime division-by-zero/부호반전을 원천 차단.
6. **낙하 데미지는 이 Foundation 레이어에 존재하지 않음** — 낙하로 인한 데미지 로직은 정의되지 않음. Arena Morphing이 "낙하데미지 제거"를 가정하는 것(Interactions with Other Systems 참조)은 이 전제(현재 낙하데미지 없음)와 일치하며 모순 아님. 향후 낙하데미지가 필요해지면 별도 시스템(Health/Damage Core 또는 신규 GDD)이 소유.
7. **이동은 절대 캐스팅을 막지 않음** — 이동 입력 처리와 스펠 캐스팅 상태는 완전히 독립. 캐스팅 중에도 `AddMovementInput` 호출이 항상 살아있어야 함 (Player Fantasy의 핵심 제약). 이 계약은 Spell Casting (base) 시스템이 아직 존재하지 않아도 **정적 검증 가능한 아키텍처 불변식**으로 취급됨: Movement 모듈은 Spell Casting의 타입/상태를 컴파일 타임에 전혀 참조하지 않음.
8. **Core Rule 7의 일반화**: 이동을 막지 않는다는 보장은 캐스팅에만 한정되지 않고 **모든 로코모션 애니메이션**(점프 이탈, 착지 등)에도 동일하게 적용됨. Animation Feel Targets 표의 Startup/Active/Recovery 프레임은 순수 프레젠테이션 레이어이며, `AddMovementInput` 처리나 점프 실행 타이밍을 지연/차단하지 않음 — 몬타주 기반 록(root motion lock, `bStopAllMontages` 스타일 입력 차단) 구현 금지. **모든 로코모션 애니메이션(점프 이탈/체공/착지/착지 recovery 포함)은 반드시 non-root-motion(in-place)으로 제작** — 루트모션 트랜슬레이션이 CMC가 계산한 velocity/위치와 동시에 캐릭터를 움직이면 서로 충돌(pose와 실제 이동 경로 불일치, 특히 착지 recovery 중 발이 미끄러지는 것처럼 보이는 아티팩트)이 발생함. 포즈는 항상 CMC가 결정한 실제 이동 위에 얹히는 프레젠테이션일 뿐이며, 이동 자체를 절대 구동하지 않음.
9. **착지 히트스탑은 애니메이션/카메라 레이어 전용** — `SetGlobalTimeDilation`이나 액터 단위 `CustomTimeDilation` 등 어떤 형태의 Time Dilation도 사용하지 않음. 순수 시각적 프리즈(포즈/카메라)로만 구현하며 게임플레이 틱(이동 입력 처리, 캐스팅 상태 처리)은 히트스탑 중에도 100% 정상 틱레이트로 계속됨 — Core Rule 7/8의 보장을 착지 순간에도 깨지 않기 위함. **구현 필수 요건(design-review 2026-07-16 — 이전엔 권장사항이었으나 매 착지마다 발생하는 고빈도 판타지-크리티컬 리스크로 재분류, blocking)**: 캡슐/액터 위치는 히트스탑 중에도 정상 틱으로 계속 이동하므로("100% 정상 틱레이트"), 포즈/카메라를 액터 트랜스폼에 단순히 붙여둔 채 애니메이션만 멈추면 "정지된 포즈가 실제 위치를 따라 미끄러지듯 이동"하는 아티팩트나(특히 히트스탑 중 대쉬캔슬 등으로 이동 입력이 들어온 경우) 언프리즈 시 카메라가 실제 위치로 순간 스냅되는 아티팩트가 발생할 수 있음. **필수 기법**: 프리즈 시작 시점의 액터 트랜스폼을 캡처해 포즈/카메라에 델타 보정 오프셋으로 적용(포즈는 캡처 시점 트랜스폼에 고정), 언프리즈 시 보정 오프셋을 1~2프레임 내 블렌드아웃해 실제 트랜스폼으로 복귀 — 순간 스냅이 아닌 짧은 블렌드로 시각적 팝 최소화. 이 기법 채택 여부/대안은 gameplay-programmer 최종 결정 영역이나, "액터 트랜스폼에 포즈를 단순 부착"하는 구현은 금지. 검증은 아래 Feel Acceptance Criteria(히트스탑 시각 아티팩트) 참조 — 더 이상 playtest 재량 확인이 아닌 블로킹 게이트. **추가 주의(design-review 2026-07-16)**: 애니메이션/포즈 틱 정지는 동일 프레임에 스케줄된 `AnimNotify`(착지 SFX/임팩트 노티파이 등)도 함께 지연/누락시킬 수 있음 — 히트스탑 윈도우(40ms) 내 노티파이는 프리즈 시작 전에 미리 트리거하거나 프리즈와 무관하게 별도 타임라인으로 발화시키는 방식 검증 필요(sound-designer/technical-artist 확인 대상, 착지 SFX 슬립 방지).
10. **코요테 타임(공중 진입 그레이스 윈도우)** — 지면에서 걸어나간 직후(입력 없이 낙하 시작) **150ms** 이내의 점프 입력은 여전히 Grounded 점프로 취급됨(착지 버퍼와 대칭 구조). **근거(design-review 2026-07-16 추가, 이전 Open Question에서 승격)**: Weight and Responsiveness Profile(하단)의 "이동 실패의 원인이 입력 누락이면 안 된다"는 원칙이 착지 버퍼(150ms)만 구현하고 이탈 버퍼는 0으로 두는 비대칭을 이미 위반하고 있었음 — 특히 Arena Morphing의 붕괴 발판 클라이맥스에서 낭떠러지 이탈 직후 점프가 씹히는 경우 판단 실패가 아닌 입력 실패로 읽혀 판타지와 정면 충돌. 착지 버퍼(Jump Input Buffer Window)와 동일하게 `Character` 클래스가 타이머 소유, Safe Range/구현 세부는 Tuning Knobs 참조.
11. **[신규] F키 코어 적출 처형(Core Extraction Execution) 연출 중 이동 처리 — 히트스탑과 동일 철학 적용(design-review 2026-07-16 추가)**: 처형 연출은 순수 프레젠테이션 레이어(카메라 클로즈업 오버레이)일 뿐, `MovementLocked`를 사용하지 않으며 게임플레이 틱(이동/캐스팅 처리)은 연출 중에도 100% 정상 진행됨 — Core Rule 9의 히트스탑 계약과 동일 패턴. 캐릭터 facing(`bUseControllerRotationYaw`)은 연출용 스크립트 카메라가 아니라 플레이어가 실제로 조작 중인 게임플레이 카메라의 컨트롤 yaw를 계속 기준으로 함(플레이어 입력은 연출 중에도 죽지 않음 — 연출 카메라는 렌더링 오버레이일 뿐 `PlayerController` rotation의 소스가 아님). `MovementLocked`는 여전히 Status Effect 시스템만 설정 가능하며, Core Extraction Execution에는 이 문서가 접근권을 부여하지 않음 — 구체 카메라 컷 연출/애니메이션 몽타주는 Core Extraction Execution GDD 소유이나, "이동을 잠그지 않는다"는 계약은 이 문서가 소유.

### States and Transitions

| State | Entry Condition | Exit Condition | Behavior |
|-------|----------------|----------------|----------|
| Grounded | 기본 상태 / 착지 | 점프 입력 또는 낙하 시작(코요테 윈도우 밖) | 풀 이동속도, 싱글 점프 가능 |
| Grounded (Coyote Window) | Grounded에서 입력 없이 낙하 시작(낭떠러지 이탈) | 150ms 경과(Airborne (Falling)로 전환) 또는 이 윈도우 내 점프 입력(Grounded 점프로 소비 후 Airborne (Ascending)) | 내부적으로는 이미 낙하 중이나 점프 입력에 한해 Grounded로 취급 — 점프 카운트/AirControl 전환 시점은 실제 낙하 시작 프레임 기준 |
| Airborne (Ascending) | Grounded에서 점프 입력 (코요테 윈도우 내 점프 입력 포함) | **`Velocity.Z`가 0 이하로 전환**(정점 통과) 또는 낙하 시작, **또는 Falling 중 외부 Z축 임펄스(대쉬/Launch Character)로 `Velocity.Z`가 다시 양수로 전환** | AirControl 비례 조향 + BrakingDecelerationFalling 적용, 중력 적용 |
| Airborne (Falling) | Ascending에서 `Velocity.Z≤0` 전환 / 낙하 시작(낭떠러지, 코요테 윈도우 만료 후) | 지면 트레이스 히트, **또는 외부 Z축 임펄스로 `Velocity.Z`가 양수로 전환되면 Airborne (Ascending)로 재진입** | AirControl 비례 조향 + BrakingDecelerationFalling 적용, 중력 적용 |
| Landed → Grounded | Airborne 중 지면 히트 | 즉시 | 점프 카운트 리셋 |

> **구현 노트**: Ascending/Falling은 UE 엔진의 네이티브 movement mode가 아님(둘 다 `MOVE_Falling`) — `Velocity.Z` 부호가 **유일한 판정 기준(authoritative)**, 이 부호 판정 규칙이 Falling→Ascending 재진입(위 표 참조)에도 예외 없이 그대로 적용됨. 커스텀 모드로 승격할 경우 UE5.8에서 레거시 `SetMovementMode()` 오버로드는 deprecated이므로 `SetMovementModeWithCustomMode()` 사용 — **주의(design-review 2026-07-16)**: 이 deprecation 주장은 공식 문서 섹션 단위 출처만 확인됐고 API 시그니처 단위 인용은 아님(포스트-cutoff 엔진 버전이라 이 클래스의 주장이 환각 위험이 가장 높음) — 코드 리뷰 게이트 전 실제 UE5.8 `CharacterMovementComponent.h`로 gameplay-programmer 실물 대조 필수.
>
> **`AirTime/2`는 타이머가 아니라 디자이너용 추정치일 뿐** — 실제 Ascending→Falling 전환 트리거로 절대 구현하지 말 것. 대쉬 임펄스나 넉백처럼 외부에서 `Velocity.Z`를 변경하는 입력이 들어오면 고정 타이머(`AirTime/2`)와 실제 정점 도달 시점이 어긋난다(Arena Morphing의 무한 대쉬/Launch Character가 정확히 이 케이스). `Velocity.Z` 부호 판정만이 상태 전환의 단일 진실 공급원(single source of truth)이며, `AirTime`/`AirTime/2`는 오직 Formulas 섹션의 페이싱 추정·UI 예측용으로만 사용. `AirTime` 공식은 발사/착지 높이가 동일하다는 가정 하에 성립하며, Arena Morphing의 비평탄·붕괴 지형에서는 실제 체공시간이 이 값과 달라짐(해당 GDD에서 별도 처리).
>
> **추가 엔진 주의(design-review 2026-07-16, unreal-specialist 지적)**: 상승 중 천장 충돌(`HandleImpact` 등)로 `Velocity.Z`가 외부 임펄스 없이도 급격히 0 근처로 꺾이거나 감소하는 경우, 이 규칙에 따라 Ascending→Falling으로 조용히 전환됨 — 이는 **의도된 동작**(부호 판정 규칙에 예외 없음, States and Transitions 표와 일관)이나 구현 시 "천장에 부딪혔는데 왜 갑자기 낙하 상태로 바뀌는가"를 별도 버그로 오인하지 않도록 이 문서에 명시. 또한 `Velocity.Z` 샘플링은 반드시 **해당 틱의 `CharacterMovementComponent` 이동 처리 이후**에 읽어야 함(처리 전 값을 읽으면 1프레임 지연된 stale 값으로 판정해 대쉬/Launch 재진입 케이스에서 상태 전환이 한 틱 늦어짐).

### Interactions with Other Systems

- **Dash/Evasion** (하류): 이 시스템의 velocity/`AddMovementInput` API 위에 대쉬 임펄스를 얹음. 대쉬 중 `MaxWalkSpeed` 임시 오버라이드 인터페이스 필요. **추가로 Z축 launch/impulse 인터페이스 필요**(예: `LaunchCharacter()` 노출 또는 동등 API) — game-concept.md의 Arena Morphing "무한 대쉬(Launch Character)" 체공전투 climax가 이 인터페이스에 의존하나, 기존 문서에는 수평 속도 오버라이드만 명시되어 있었음(design-review 2026-07-16 지적으로 추가). 구체 파라미터(임펄스 크기, 기존 momentum과의 합성 방식)는 Dash/Evasion GDD 소유 — 이 문서는 "Movement 레이어가 외부에서 Z축 velocity를 주입받을 수 있는 API를 노출해야 한다"는 계약만 명시.
- **Camera System (base)** (하류): 이 시스템의 카메라-상대 입력 매핑 기준(카메라 forward)을 제공받음.
- **Spell Casting (base)** (하류): Core Rule 7의 계약(이동이 캐스팅을 막지 않음)을 준수해야 함.
- **Arena Morphing** (하류, 미설계·가정): 체공전투 페이즈에서 AirControl 값 오버라이드/낙하데미지 제거 필요할 것으로 예상 — Arena Morphing GDD 작성 시 확정. **낙하 파편 엄폐 기믹(design-review 2026-07-16 추가)**: 순수 포지셔닝으로 명시 — 이 문서의 기존 이동기(걷기/달리기/점프/대쉬)만으로 파편 뒤에 서는 것으로 충분하며, 웅크리기/기대기 등 신규 로코모션 입력이나 상태는 필요하지 않음. Arena Morphing GDD가 파편 배치/판정 볼륨을 소유하되, Movement 레이어에 새 API를 요구하지 않는다는 계약을 이 문서가 명시.
- **Core Extraction Execution** (하류, 미설계·가정, design-review 2026-07-16 추가): F키 처형 연출 중 이동 처리는 Core Rule 11 참조 — 히트스탑과 동일하게 프레젠테이션 전용, `MovementLocked` 미사용, 게임플레이 틱 100% 정상. 구체 카메라 컷/애니메이션은 Core Extraction Execution GDD 소유.

## Formulas

### Effective Move Speed

```
EffectiveSpeed = clamp(InputMagnitude, 0.0, 1.0) * MaxWalkSpeed
```

| Variable | Type | Range | Source | Description |
|----------|------|-------|--------|-------------|
| InputMagnitude | float | 0.0–1.0 (합산 축 벡터 길이, 클램프됨) | 입력 | 아날로그 스틱/WASD 입력 크기. 대각선 입력(W+D 등)은 정규화되어 1.0을 초과하지 않음 |
| MaxWalkSpeed | float (uu/s) | 튜닝값, 기본 600 | Tuning Knob | 최대 이동속도 |

**Expected output range**: 0 ~ MaxWalkSpeed (기본 튜닝 시 0–600 uu/s)
**Example**: InputMagnitude=1.0, MaxWalkSpeed=600 → EffectiveSpeed=600uu/s. InputMagnitude=0.5 → 300uu/s. 대각선 W+D 클램프 전 원시 크기 √2(≈1.414)는 1.0으로 정규화되므로 848uu/s가 아닌 600uu/s로 캡됨.

### Jump Air Time

```
AirTime = (2 * JumpZVelocity) / (GravityScale * abs(WorldGravityZ))
```

| Variable | Type | Range | Source | Description |
|----------|------|-------|--------|-------------|
| JumpZVelocity | float (uu/s) | 튜닝값, 기본 600, Safe Range 400–800, **하드 클램프 최소 100**(코드 강제) | Tuning Knob | 점프 초기 수직속도 |
| GravityScale | float | 튜닝값, 기본 1.0, Safe Range 0.8–1.3, **하드 클램프 최소 0.1**(코드 강제, 로드 시 + 런타임 write 시 모두 적용 — Tuning Knobs 참조) | Tuning Knob | 중력 배율 |
| WorldGravityZ | const (uu/s²) | 크기(magnitude) 980, **스케일 적용 전(unscaled) 값** | 엔진 기본값 | UE `UWorld::GetGravityZ()`(월드 레벨, GravityScale 미적용)는 **-980**(음수, -Z 방향)을 반환함 — 이 식은 크기(절대값)를 사용. **경고**: `UCharacterMovementComponent::GetGravityZ()`(컴포넌트 오버라이드)는 이미 `GravityScale`이 곱해진 값을 반환하므로, 이 함수를 쓰면서 위 식의 `GravityScale`을 또 곱하면 GravityScale이 제곱 적용되어 틀어짐(기본값 1.0에서는 안 드러나고 Safe Range 끝단 1.3에서 조용히 어긋남). 구현 시 반드시 `UWorld::GetGravityZ()`(또는 프로젝트 기본 중력 설정값)의 절대값을 사용 — 컴포넌트의 `GetGravityZ()` 아님 |

**Expected output range**: 튜닝 knob의 Safe Range 전체 범위 기준 **0.63s ~ 2.04s** (JumpZVelocity=400,GravityScale=1.3 → 0.63s / JumpZVelocity=800,GravityScale=0.8 → 2.04s). 기본값(JumpZVelocity=600,GravityScale=1.0) 기준으로는 약 1.22s.

**하드 경계(Joint AirTime Bound, design-review 2026-07-16 추가)**: 개별 하드 클램프(JumpZVelocity 100/GravityScale 0.1)만으로는 안전하지 않음 — 두 값이 각자의 클램프/Safe Range 극단에서 **조합**되면 개별 클램프가 막지 못하는 비정상값이 나옴:
- `GravityScale=0.1`(하드 클램프 최소) + `JumpZVelocity=800`(Safe Range 최대) → AirTime = 1600/98 ≈ **16.33s** (기존에 문서가 우려하던 12.24s보다 나쁨)
- `JumpZVelocity=100`(하드 클램프 최소) + `GravityScale=1.3`(Safe Range 최대) → AirTime = 200/1274 ≈ **0.157s** (정점 도달 ~78ms — 사실상 점프가 지면을 벗어나지 못함)

이 두 극단을 동시에 닫기 위해 **AirTime에 별도의 플레이 가능 하드 경계 0.5s ~ 3.0s**를 선언한다 — 이는 물리 계산 결과를 사후에 재클램프하는 것이 아니라(그러면 JumpZVelocity/GravityScale 값과 실제 체공시간이 어긋나 States and Transitions의 Velocity.Z 판정과 모순됨), **데이터애셋 로드 시점에 계산된 AirTime이 이 범위를 벗어나면 로드를 거부하고(에디터) 또는 경고 로그 후 GravityScale/JumpZVelocity 중 하나를 이전 유효값으로 되돌리는(런타임) 설계 시점 검증**이다. 즉 "튜닝 조합 자체가 유효하지 않음"을 개별 변수가 아닌 두 변수의 곱 관계로 검증. 정확한 검증 로직/되돌림 정책은 구현 시 gameplay-programmer 확인 필요하나, 계약(0.5s~3.0s 밖 조합은 유효하지 않음)은 이 문서가 소유.

**검증 순서(design-review 2026-07-16 추가, blocking — 순서 미명시 시 세이프티넷이 세이프티넷을 못 지키는 역설 발생)**: 반드시 다음 순서로 실행 — ① 개별 하드 클램프(JumpZVelocity 최소 100, GravityScale 최소 0.1)를 먼저 적용해 두 값 모두 division-by-zero/음수를 만들 수 없는 상태로 만든다. ② 그 다음에만 클램프된 값으로 AirTime을 계산해 Joint Bound(0.5s~3.0s) 위반 여부를 검사한다. Joint Bound 계산이 개별 클램프보다 먼저(또는 동시에) 실행되면 원본 미검증 값(GravityScale=0 등)으로 나눗셈을 시도해 개별 클램프가 존재하는 의미가 없어짐 — 이 순서 규칙은 "튜닝 조합 자체가 유효하지 않음" 계약의 전제조건이며 선택사항이 아니다. **되돌림 정책의 부트스트랩 상태(design-review 2026-07-16 추가)**: "이전 유효값으로 되돌림" 정책은 되돌릴 이전 값이 존재한다고 전제하나, 신규 생성된 데이터애셋에는 이전 유효값이 없음 — 이 경우 정책은 로드 거부(에디터, 이전 유효값 부재 시 강제)로 확정하고, 런타임 되돌림은 "이미 한 번 유효 로드에 성공한 이후의 write 시도"에만 적용됨을 명시. 정확한 되돌림 저장 위치/구현은 여전히 gameplay-programmer 확인 필요(Open Questions 참조)하나, "부트스트랩 시 되돌릴 값이 없다"는 이 gap 자체는 이 리비전으로 닫힘.
**Example**: JumpZVelocity=600, GravityScale=1.0 → AirTime = (2×600)/(1.0×980) ≈ **1.2245s**. 전투 중 공중 회피/조준 윈도우 길이를 결정하며, Arena Morphing 체공전투 페이싱과 직결. **Time-to-apex = AirTime/2 ≈ 0.61s** — States and Transitions 표 참조. 이 공식은 발사 높이=착지 높이 가정 하에만 성립(Arena Morphing의 비평탄 지형에서는 별도 처리 필요).

### Air Steering and Deceleration

```
AirSteerAccel = AirControl * MaxAcceleration
```

| Variable | Type | Range | Source | Description |
|----------|------|-------|--------|-------------|
| AirControl | float | 0.0–1.0, 튜닝값, 기본 0.6, Safe Range 0.4–1.0 | Tuning Knob | 공중 조향 강도. Core Rule 4의 반응성 강화 결정에 따라 기존 0.2에서 상향 |
| MaxAcceleration | float (uu/s²) | 튜닝값, 지상 이동과 공유, **하드 클램프 최소 1000**(design-review 2026-07-16 추가 — 0/음수 시 지상 즉시가속(Core Rule 3)과 공중 조향(Core Rule 4)이 공유 변수라 동시에 소리 없이 무너짐) | Tuning Knob | 조향 가능 최대 가속력 산출에 재사용 |
| BrakingDecelerationFalling | float (uu/s²) | 튜닝값, **기본 5000**(2026-07-16 리비전 — 기존 1200에서 상향, 아래 근거 참조), Safe Range **3000–8000**, **하드 클램프 최소 100**(design-review 2026-07-16 추가) | Tuning Knob | InputMagnitude=0일 때 공중 수평 감속률(에어브레이킹) — 완전 관성 유지가 아니라 지상에 근접한 크리스프한 감속 적용. **이는 UE 네이티브 필드명이 아닌 설계 의도치** — 실제 CMC 구현은 `BrakingDecelerationFalling`(동일 변수명 재사용, CMC의 실제 falling-braking 필드) + `FallingLateralFriction`을 사용하며, 아래 120ms 선형 감속 계산은 **`FallingLateralFriction`을 0에 근접하게 유지**할 때만 성립(0보다 유의미하게 크면 실제 정지시간이 손계산치보다 길어짐 — 구현 시 gameplay-programmer 확인 필수) |

**Expected output range**: AirControl=0.6, MaxAcceleration=18000 → AirSteerAccel=10800uu/s².
**합성 벡터 명확화(design-review 2026-07-16 추가, blocking — Edge Cases의 "둘 다 InputMagnitude와 무관하게 항상 활성"이라는 문구가 IM=1.0에서 브레이킹이 조향을 깎아먹는 것으로 오독될 여지를 닫음)**: AirSteerAccel과 BrakingDecelerationFalling은 서로 다른 속도 성분에 작용하는 두 항이며 단순 차감 관계가 아니다 — `BrakingDecelerationFalling`은 **현재 입력 방향과 정렬되지 않은(off-axis) 속도 성분**(관성으로 남은 드리프트)에만 감속력을 가하고, `AirSteerAccel`은 **입력 방향으로 속도를 끌어올리는** 항이다. 따라서 IM=1.0이고 기존 velocity가 입력 방향과 이미 정렬돼 있다면 off-axis 성분이 거의 없어 브레이킹 기여가 ~0에 수렴하고 net accel ≈ AirSteerAccel(10800uu/s², 이 섹션 Expected output range와 일치). IM=0이면 조향 성분이 없어 사실상 순수 브레이킹만 작용(아래 120ms 예시). "둘 다 항상 활성"은 두 항이 매 틱 항상 계산·적용된다는 뜻이지, 항상 같은 크기로 서로 상쇄한다는 뜻이 아니다.
**Example**: 공중에서 입력이 살아있으면 최대 10800uu/s²로 방향 전환 가속(지상보다는 약하지만 즉각적인 코스보정 가능). 입력이 0이 되면 즉시 관성 유지가 아니라 BrakingDecelerationFalling(기본 5000uu/s²)로 감속 — 600uu/s 기준 정지시간 약 **120ms**(기존 1200uu/s² 설정의 500ms 대비 대폭 단축, `FallingLateralFriction≈0` 가정 하에 성립). **AC 연동 주의(design-review 2026-07-16 추가)**: 이 120ms 수치는 Feel Acceptance Criteria의 ±10% 하드 pass/fail 기준(공중 감속/조향 객관적 검증 AC)에 직접 사용되므로, `FallingLateralFriction≈0` 가정이 Open Questions에서 "non-blocking"으로 분류돼 있더라도 **해당 AC 자체는 엔진 스파이크(FallingLateralFriction 실측)로 가정이 확인되기 전까지 provisional(잠정) 상태**로 취급 — 스파이크 결과가 가정과 다르면 이 AC의 기준값(120ms, ±10%)과 BrakingDecelerationFalling 값 자체를 함께 재조정. **대안 검토 노트(design-review 2026-07-16, 버전 표기 수정)**: `AirControlBoostMultiplier`/`AirControlBoostVelocityThreshold`(점프 정점 부근에서 조향력 부스트)가 이 문서의 체공 리포지셔닝 목표와 정확히 부합하는 네이티브 대안으로, 향후 리비전에서 손조정 `BrakingDecelerationFalling` 단일값 대신 채택 검토 여지 있음(현재는 블로킹 아님, gameplay-programmer 판단 영역). **주의**: 이 두 프로퍼티는 UE5.8 신규 기능이 아니라 이전 엔진 버전부터 존재하던 CMC 필드임(unreal-specialist 검증) — "UE5.8의 네이티브 대안"이 아니라 그냥 "네이티브 대안"으로 정정. **추가 참고**: `BrakingDecelerationFalling`의 CMC 기본값은 **-1**(엔진 sentinel — "브레이킹 감속 미적용, 완전 관성 유지"를 의미)이며 0과는 다른 특수값이다. 이 문서는 명시적으로 양수값(5000)을 지정하므로 sentinel 이슈는 해당 없으나, 구현자가 임의로 0을 입력해 "브레이킹 없음"을 의도하면 실제로는 sentinel(-1)과 다른 거동(0 자체도 유효한 감속값 취급)이 나올 수 있어 혼동 주의.
**리비전 근거(design-review 2026-07-16)**: 최초 설계값(1200uu/s²)은 600uu/s에서 정지까지 500ms가 걸려 지상(33ms)의 15배에 달했고, Safe Range 상단(2000)조차 300ms로 9배 격차가 남아 "지상-공중 손맛 격차 해소"라는 Core Rule 4의 의도와 실제 수치가 모순됐다(체공전투 리포지셔닝 중 반 박자 늦게 반응하는 느낌으로 이어질 위험). 이번 리비전은 그 모순을 해소하는 방향(격차 축소)을 선택 — 완전한 무-에어브레이크(즉시정지)는 여전히 지양하되(공중의 "떠 있는" 감각 최소한은 유지), 지상 크리스프함에 근접하도록 상향. 최종 체감 검증은 Feel Acceptance Criteria(공중 이동 "무겁다" 응답 <25%)로 확인.

## Edge Cases

| Scenario | Expected Behavior | Rationale |
|----------|------------------|-----------|
| Destructible Geometry 조각이 발밑에서 사라짐 (연쇄파괴) | 지면 트레이스 실패 시 즉시 Airborne(Falling) 전환, 별도 특수처리 없음 | CharacterMovementComponent 표준 낙하 처리로 충분 — 별도 로직 불필요 |
| 착지 직전(≈150ms 버퍼 윈도우) 점프 입력 | 입력 버퍼링, 착지 즉시 점프 소비 | 빠른 전투 리듬에서 입력이 "씹히는" 느낌 방지 |
| 공중에서 입력 크기(InputMagnitude)=0 | `BrakingDecelerationFalling`(기본 5000uu/s², 2026-07-16 상향)로 지상에 근접한 크리스프한 수평 감속 — 완전 관성 유지 아님. `AirControl`은 입력이 있을 때 조향 가속을 담당 | Core Rule 4의 반응성 강화 결정 — 완전 무-에어브레이크는 Souls류 "커밋된 궤적"에 가까워 판타지와 충돌한다고 판단. 최초 1200 설정은 500ms 정지시간으로 지상(33ms) 대비 격차가 과도해 판타지와 모순됐음(design-review 2026-07-16 지적) — 5000으로 상향해 격차를 대폭 축소, Feel Acceptance Criteria로 최종 검증 |
| 공중에서 InputMagnitude가 0에 가깝지만 정확히 0은 아님(예: 0.02, 아날로그 스틱 중심 통과 중) | AirControl 비례 조향과 BrakingDecelerationFalling 감속을 **동시에** 적용(둘 다 InputMagnitude 크기와 무관하게 항상 활성) — "0이어야만 감속 시작"이 아니라 InputMagnitude가 얼마든 조향력(AirControl×MaxAcceleration×InputMagnitude 비례)과 감속력이 함께 작용해 데드존 없는 연속 전이 보장 | 빠른 스틱 플릭/방향 역전 중 조향도 감속도 안 걸리는 "먹통" 구간을 방지 — Formulas 섹션의 AirSteerAccel 공식이 InputMagnitude=0.02에서도 비례 적용됨을 명시 |
| `GravityScale`이 데이터애셋에 0 또는 음수로 설정됨(오조작), **또는 런타임에(예: 향후 GAS 중력 조작 어빌리티) 0/음수로 write 시도** | 로드 시점뿐 아니라 **모든 write 경로**(setter 통과)에서 즉시 최소값 0.1로 하드 클램프, 경고 로그 출력 — 로드 시점 클램프만으로는 런타임 write를 우회하지 못함 | AirTime division-by-zero(GravityScale=0) 또는 음수 AirTime/물리적으로 무의미한 상승 가속(음수 GravityScale)을 원천 차단. 클램프 최소값(0.1) 도달 시에도 AirTime≈12.24s(기본 JumpZVelocity 기준)로 비정상 체공이 되므로 완전한 안전장치는 아님(Formulas 섹션 참조) |
| `MaxWalkSpeed` 또는 `JumpZVelocity`가 데이터애셋에 0 또는 음수로 설정됨(오조작) | 로드 시 즉시 각 하드 클램프 최소값(MaxWalkSpeed 100, JumpZVelocity 100)으로 클램프, 경고 로그 출력 | MaxWalkSpeed=0/음수는 "이동이 캐스팅을 막지 않는다"는 판타지와 별개로 이동 자체가 정지/역전되는 것을 방지(Core Rule 7의 정신을 이동 전체로 확장); JumpZVelocity=0/음수는 AirTime 음수화를 방지 — GravityScale과 동일한 원칙 적용(design-review 2026-07-16 추가) |
| **[신규]** `MaxAcceleration`, `BrakingDecelerationWalking`, 또는 `GroundFriction`이 데이터애셋에 0 또는 음수로 설정됨(오조작) | 로드 시 즉시 각 하드 클램프 최소값(MaxAcceleration 1000, BrakingDecelerationWalking 1000, GroundFriction 1)으로 클램프, 경고 로그 출력 | design-review 2026-07-16 지적 — 이 세 값은 기존에 하드 클램프가 없어 0/음수 설정 시 지상 즉시가속(Core Rule 3)과(MaxAcceleration은 공유 변수라) 공중 조향(Core Rule 4)까지 동시에 소리 없이 무너지는 가장 파급이 큰 미클램프 변수였음 |
| **[신규]** `GravityScale`/`JumpZVelocity` 조합이 개별 하드 클램프·Safe Range 내부에 있음에도 계산된 AirTime이 **0.5s~3.0s Joint Bound를 벗어남**(예: GravityScale=0.1+JumpZVelocity=800→16.33s, JumpZVelocity=100+GravityScale=1.3→0.157s) | 로드 시 AirTime 재계산 후 범위 밖이면 로드 거부(에디터) 또는 경고 로그 후 이전 유효 조합으로 되돌림(런타임) | design-review 2026-07-16 지적 — 개별 변수 클램프만으로는 이 두 변수의 **곱 관계**에서 나오는 비정상 체공(과도하게 길거나 점프가 지면을 거의 벗어나지 못함)을 막지 못함(Formulas 섹션 Jump Air Time 참조) |
| 착지 히트스탑(40ms) 발생 중 이동입력 또는 캐스팅 시도 | 100% 정상 처리됨 — 히트스탑은 애니메이션/카메라 레이어 전용이며 Time Dilation 미사용 | Core Rule 8/9 보장 — 고빈도 이벤트(착지)마다 반응성 계약이 깨지는 것을 방지 |
| **[신규]** 낭떠러지 이탈(입력 없이 낙하 시작) 후 150ms 코요테 윈도우 내 점프 입력 | Grounded 점프로 취급되어 소비됨(착지 버퍼와 대칭) | Core Rule 10 참조 — Weight and Responsiveness Profile의 "이동 실패는 입력 누락이 원인이면 안 된다"는 원칙을 착지뿐 아니라 이탈 시점에도 대칭 적용 |
| 낭떠러지 이탈 후 코요테 윈도우(150ms) **밖**에서 점프 입력 | 일반 공중 입력으로 처리(Grounded 점프 아님) — 점프 미실행 | 코요테 윈도우는 무제한 유예가 아니라 착지 버퍼와 동일한 유한 그레이스일 뿐 — 윈도우 경계 이후는 정상적인 공중 상태 |
| 미래 Status Effect 시스템의 스턴/캐스팅 중 이동 고정(root) 요구 | `MovementLocked` 플래그를 **Status Effect 시스템만** 설정 가능. Spell Casting 시스템은 이 플래그에 접근 불가. 지속시간의 구체 수치(캡)는 이 문서가 정의하지 않으며 Status Effect GDD가 소유 — 이 문서는 접근 통제 계약만 소유 | Core Rule 7("캐스팅이 이동을 막지 않음") 보장 유지하면서도, 정당한 CC(스턴 등)는 Status Effect 통해서만 허용 — 소유권 명확화. "무제한 고정 금지"라는 원칙은 이 문서가 계약으로 못박되, 정확한 상한값 튜닝은 CC 밸런스를 다루는 Status Effect GDD의 책임 영역이라 이 문서에서 임의로 숫자를 정하지 않음 |

## Dependencies

| System | Direction | Nature of Dependency |
|--------|-----------|---------------------|
| Camera System (base) | 상호참조 (양방향, 순환 아님) | Movement는 Camera의 컨트롤 요(yaw)를 읽어 이동 기준축 계산. Camera는 Movement의 액터 위치를 읽어 스프링암 추적. 둘 다 PlayerController Rotation을 독립적으로 읽음 — 실행순서 의존 없음 |
| Spell Casting (base) | Spell Casting이 이 시스템에 의존 | Core Rule 7 계약(캐스팅이 이동을 막지 않음)에 의존 |
| Dash/Evasion | Dash/Evasion이 이 시스템에 의존 | `AddMovementInput`/velocity API 위에 대쉬 임펄스 레이어링. **Z축 launch/impulse 인터페이스 노출 필요**(Arena Morphing 체공전투 climax가 의존 — Interactions with Other Systems 참조, design-review 2026-07-16 추가) |
| Status Effect (미설계) | Status Effect가 이 시스템에 의존 | `MovementLocked` 플래그 인터페이스를 통해서만 이동 고정 가능. 이 문서는 "누가 설정할 수 있는가(접근 통제)"만 소유하며, "얼마나 오래 고정할 수 있는가(상한값)"는 Status Effect GDD 소유 — 이 문서는 무제한 고정을 금지하는 원칙만 계약으로 명시 |
| Arena Morphing (미설계) | Arena Morphing이 이 시스템에 의존 (가정) | AirControl 오버라이드 인터페이스 필요 예상. Base 레이어의 AirControl/BrakingDecelerationFalling 반응성 강화(Core Rule 4)를 상속하므로 별도 조율 불필요 — 값만 오버라이드. **낙하 파편 엄폐(design-review 2026-07-16 추가)**: 순수 포지셔닝, 신규 API 불필요 |
| Core Extraction Execution (미설계) | Core Extraction Execution이 이 시스템에 의존 (가정, design-review 2026-07-16 추가) | Core Rule 11 계약(처형 연출이 이동을 막지 않음, MovementLocked 미사용, 히트스탑과 동일 철학)에 의존 |

## Tuning Knobs

| Parameter | Current Value | Safe Range | Effect of Increase | Effect of Decrease |
|-----------|--------------|------------|-------------------|-------------------|
| MaxWalkSpeed | 600 uu/s | 400–900 (**코드 하드 클램프 최소 100** — 0/음수 시 이동 정지/역방향 방지) | 너무 빠르면 조작감/스펠 조준 난이도↑ | 너무 느리면 "즉각반응형" 판타지 훼손 |
| MaxAcceleration | 18000 uu/s² | 12000–24000 (**코드 하드 클램프 최소 1000**, design-review 2026-07-16 추가 — 0/음수 시 지상 즉시가속(Core Rule 3)과 공중 조향(Core Rule 4)이 동시에 무너짐, 지상/공중 공유 변수라 기존에 클램프 없던 것이 가장 큰 리스크였음) | 즉시가속 강화(33ms 목표 달성에 필요) — 과도하면 저속 틱에서 오버슈트/떨림 위험 | 낮으면 지상 이동이 램프처럼 느껴짐, 33ms Input Responsiveness 목표 실패 |
| BrakingDecelerationWalking | 18000 uu/s² | 12000–24000 (**코드 하드 클램프 최소 1000**, design-review 2026-07-16 추가 — 0/음수 시 지상 정지 자체가 불가능해져 Core Rule 3 위반) | 즉시정지 강화 | 낮으면 정지 시 미끄러짐("slippery") 재발 — Feel Acceptance Criteria 위반 위험 |
| GroundFriction | 16 | 8–24 (**코드 하드 클램프 최소 1**, design-review 2026-07-16 추가 — 0 설정 시 정지 마찰 완전 소실로 "빙판" 아티팩트) | 정지 마찰 강화, 과도하면 저속 방향전환이 부자연스러움 | 낮으면 감속 지연, 관성 과다 |
| bUseSeparateBrakingFriction | false | — | true 시 `BrakingFriction`을 `GroundFriction`과 별도로 튜닝 가능(현 스코프에서는 불필요 — 필요해지면 재검토) | — |
| JumpZVelocity | 600 uu/s | 400–800 (**코드 하드 클램프 최소 100** — 0/음수 시 음수 AirTime 방지) | 점프 높아져 템포 느려짐(공중체류↑) | 회피/체공전투용 고도 부족 |
| GravityScale | 1.0 | 0.8–1.3 (**코드 하드 클램프 최소 0.1** — 0/음수 방지, **데이터애셋 로드 시뿐 아니라 런타임 write 시에도 적용**되는 setter를 통해서만 값 변경 허용 — 향후 GAS 중력 조작 어빌리티가 로드-타임 클램프를 우회하지 않도록) | 낮추면 체공시간↑, 텐션 늘어짐 | 높이면 급전개, 콤보 여유 부족 |
| AirControl | 0.6 (기존 0.2에서 상향 — Core Rule 4 반응성 강화 결정) | 0.4–1.0 (**코드 하드 클램프 최소 0.2** — 0 설정 시 공중 조향 완전 상실이 Core Rule 4의 "항상 반응함" 보장과 정면 충돌하므로 Safe Range 하단보다 낮은 절대 하한을 코드로 강제) | 공중 조작 자유로워짐(체공전투 유리) 대신 회피 난이도↓ | 0.4 미만 지양 — 다시 무거워짐, Souls류 커밋 궤적으로 회귀 위험 |
| BrakingDecelerationFalling | **5000 uu/s²**(2026-07-16 리비전 — 기존 1200에서 상향, Formulas 섹션 근거 참조) | **3000–8000** | 공중 감속 강화 → 지상과 손맛 격차 축소(600uu/s 정지시간 120ms) | 너무 낮으면 관성 과다(에어브레이크 없음과 동일해짐); 3000 미만은 최초 리비전 전 문제(500ms 격차) 재발 위험 |
| Jump Input Buffer Window | 150 ms | **80–250 ms**(2026-07-16 리비전 — 기존 50ms 하단에서 상향, 아래 근거 참조) | 너무 길면 의도치않은 점프 오조작 | 너무 짧으면 입력 "씹힘" 재발 — **리비전 근거(design-review 2026-07-16)**: 기존 50ms 하단은 Frame Budget 표의 점프 50ms 레이턴시 예산(입력→점프 실행 반응 목표치)과 정확히 동일해 실질 여유가 마이너스였음(버퍼 판정 자체가 전체 예산을 소진). 80ms로 상향해 판정 윈도우가 반응성 예산을 잠식하지 않도록 함. 첫 플레이테스트 검증 필수(Open Questions 참조) |
| Coyote Time Window | **150 ms**(2026-07-16 신규 추가, design-review — Core Rule 10 참조) | **80–250 ms**(2026-07-16 리비전 — 착지 버퍼와 동일 근거로 하단 상향, 위 Jump Input Buffer Window 참조) | 너무 길면 낭떠러지 근처에서 "떠 있는" 느낌/판정 신뢰도 저하 | 너무 짧으면 낭떠러지 이탈 직후 점프가 씹혀 Weight and Responsiveness Profile의 "입력 누락 없음" 원칙 위반 재발 |
| AirTime Joint Bound | **0.5s ~ 3.0s**(2026-07-16 신규 추가, design-review — Formulas 섹션 Jump Air Time 참조) | 해당 없음(고정 계약값, 튜닝 대상 아님) | 해당 없음 — GravityScale/JumpZVelocity 조합이 이 범위를 벗어나면 로드 거부/이전 유효값 되돌림 | 해당 없음 |

> **버퍼 타이머 소유**: Jump Input Buffer Window/Coyote Time Window의 타이머는 모두 `Character` 클래스가 소유(착지 감지 `Landed()`와 동일 위치) — `PlayerController`나 `AnimInstance`가 아님. 향후 멀티플레이어 확장 시 `FSavedMove_Character` 저장/예측 파이프라인에 통합 필요(현재 스코프 밖, Open Questions에 등록).

## Visual/Audio Requirements

| Event | Visual Feedback | Audio Feedback | Priority |
|-------|----------------|---------------|----------|
| 지상 이동(발걸음) | 스텝마다 표면 먼지/이펙트 미세 트레일 | 표면별 발소리 SFX | High |
| 점프 이탈 | 스쿼시&스트레치 소폭 애니메이션, 발밑 소량 임팩트 파티클 | 점프 SFX | Medium |
| 착지 | 임팩트 파티클(속도 비례 강도) + 카메라 살짝 다운(세부는 Camera System GDD 소유) | 표면별 착지 SFX | High |

> Solo mode — `art-director` 미상담. Production 전 수동 확인 필요.

## Game Feel

### Feel Reference

*NieR:Automata*의 대쉬-회피 캔슬 반응성(입력 즉시 반영, 관성 없음) + *Dead Cells*의 감속 램프 없는 즉시정지. Anti-reference: Souls류의 무겁고 커밋된 회피(강제 recovery 프레임).

### Input Responsiveness

| Action | Max Input-to-Response Latency (ms) | Frame Budget (at 60fps) | Notes |
|--------|-----------------------------------|------------------------|-------|
| 이동 시작 | 33ms | 2 frames | 램프 없이 즉시 가속 |
| 이동 정지 | 33ms | 2 frames | 감속 램프 없이 즉시 정지 |
| 점프 | 50ms | 3 frames | 버퍼링 적용해도 체감 지연 없어야 함 |

### Animation Feel Targets

| Animation | Startup Frames | Active Frames | Recovery Frames | Feel Goal | Notes |
|-----------|---------------|--------------|----------------|-----------|-------|
| 점프 이탈 | 2 | 4 | 2 | 스냅감, 낮은 커밋 | 캐스팅 인터럽트 안 됨 |
| 착지 | 0 | 2 | 3 | 임팩트감 있지만 복귀 빠름 | Recovery 중에도 이동입력 즉시반영 |

> **프레임 수는 60fps 기준 참조치**이며, 실제 구현은 delta-time 기반으로 동일한 실제 지속시간을 유지해야 함(다른 프레임레이트에서 고정 틱 카운트로 구현하면 60fps 미달 시 체감 지속시간이 달라짐 — Core Rule 8과 함께 적용).

### Impact Moments

| Impact Type | Duration (ms) | Effect Description | Configurable? |
|-------------|--------------|-------------------|---------------|
| 착지 히트스탑 | 40ms | 짧은 프리즈(과하지 않게, 전투흐름 안 끊김) | Yes |
| 카메라 임팩트(착지) | 100ms | 미세 다운킥 | Yes |

### Weight and Responsiveness Profile

가볍고 반응적(무겁지 않음). 플레이어는 항상 높은 조작권을 가짐 — 언제든 코스보정 가능, 커밋 구간 최소화. 스냅 퀄리티는 크리스프하고 바이너리에 가까움(관성 최소). 가속/감속 모델은 즉시시작·즉시정지(램프 없는 아케이드 모델). 실패 텍스처: 회피/포지셔닝 실패 시 그 원인이 "입력이 늦었다"가 아니라 "판단이 늦었다"여야 함 — Player Movement 자체가 실패의 원인이 되면 안 됨(입력 누락 없음).

### Feel Acceptance Criteria

- [ ] GIVEN N≥8 플레이테스터가 15분 전투 세션 플레이, WHEN 세션 후 구조화 5점 Likert 설문("입력에 즉시 반응하며 드리프트/지연이 느껴지지 않았다") 응답, THEN 75% 이상이 4점 이상 응답 ("floaty"/"slippery" 자유서술 코멘트는 정성 보조자료로만 기록, pass/fail 기준 아님)

> **설문 기반 Feel-AC 공통 프로토콜(design-review 2026-07-16 추가 — 이 섹션의 모든 N≥8 플레이테스터 설문 AC에 공통 적용)**: 모집 대상은 사내 QA/개발팀 외부 인원 우선(내부 개발자는 이미 시스템에 익숙해 반응성 저하를 못 느낄 편향 위험 — 불가피하게 내부 인원 포함 시 최대 절반까지만 허용). 세션당 참가자는 위 4개 설문(반응성/공중이동/카메라스냅/미세드리프트) 전부를 **하나의 세션에서 순서를 무작위화**하여 응답(고정 순서로 인한 피로/순서효과 방지). 반올림 규칙: N=8에서 75% 기준은 6/8명 이상, 25% 기준은 2/8명 이하, 20% 기준은 1.6명 → **소수점 발생 시 보수적으로 내림 처리**(예: 20% 기준은 1명 이하만 PASS, 2명이면 FAIL) — pass 기준을 유리하게 반올림하지 않음.
- [ ] **[측정 도구 수정, 하네스 스펙 확정(design-review 2026-07-16 — 기존엔 QA가 실제로 구현 불가능할 만큼 미명시, blocking)]** GIVEN 60fps 유지 상태(프레임타임 ≤16.6ms), WHEN 이동 시작/정지 입력 발생, THEN 95th percentile 반응 지연(입력 이벤트 타임스탬프 → velocity 변화 반영 프레임까지)이 Input Responsiveness 표의 33ms(2프레임) 초과 안 함 — **`stat unit`은 프레임 타이밍만 측정하며 입력→반응 지연을 측정하지 않으므로 사용 불가**. **트레이스 하네스 확정 스펙**: 전용 `TRACE_CPUPROFILER_EVENT_SCOPE` 채널(예: `MovementInputTrace`) 2개 지점에 삽입 — (a) "입력 처리 시점"은 raw HID/디바이스 이벤트가 아니라 **Enhanced Input의 `Triggered` 델리게이트가 발화되는 프레임의 시작 시각**으로 정의(엔진 입력 파이프라인 내부 지터를 측정 범위에서 제외하기 위함), (b) "velocity 변화 반영 시점"은 `CharacterMovementComponent::TickComponent` 완료 후 `Velocity`가 목표 방향으로 실제 갱신된 첫 프레임. 두 trace event의 타임스탬프 차이를 Unreal Insights `.utrace` 파일로 캡처 후 CSV로 export(`UnrealInsights -export`), N≥50 샘플에서 95th percentile을 스크립트(pandas/Excel 등, 도구 불문)로 계산 — 원본 `.utrace`/CSV는 `production/qa/evidence/`에 증거로 보관. Feel-AC 중 "이동 시작/정지 램프 체감 없음"을 대체
- [ ] **[신규 — ramp-shape 검증]** GIVEN 위 자동화 하네스, WHEN 이동 시작 입력 발생, THEN velocity가 목표 EffectiveSpeed의 95%에 도달하는 시간(time-to-95%)이 50ms 초과 안 함 — onset latency(첫 반응) 측정만으로는 카메라 랙/블렌드스페이스 보간 등 하류 스무딩으로 인한 "floaty" 체감을 놓칠 수 있어 별도 지표로 검증 필요
- [ ] **[신규 — 저프레임 대응]** GIVEN 프레임타임 33.3ms(30fps, 의도적 부하 시뮬레이션) 유지 상태, WHEN 이동 시작/정지 입력 발생, THEN 95th percentile 반응 지연이 66ms(2프레임 @ 30fps) 초과 안 함 — 60fps 미달 상황에 대한 입력 반응성 목표 부재 갭 해소
- [ ] Core Rule 7/8의 "캐스팅·애니메이션이 이동을 막지 않음" 계약은 Acceptance Criteria의 AC7(정적 검사 + 스텁 테스트)로 검증 — 별도 주관적 Feel-AC 불필요
- [ ] **[측정 도구 수정]** GIVEN 60fps 유지 상태 **AND 점프 입력이 착지 전 버퍼링 윈도우(150ms) 밖에서 발생한 non-buffered 케이스**, WHEN 점프 입력 발생, THEN 95th percentile 반응 지연(위와 동일 trace 방식)이 50ms(3프레임) 초과 안 함 — 버퍼링된 점프(착지 시 소비)는 이 AC의 측정 대상이 아님(버퍼 자체가 최대 150ms 지연을 의도적으로 허용하므로 별도 AC로 검증, Acceptance Criteria 섹션 참조)
- [ ] GIVEN N≥8 플레이테스터가 공중 이동(점프/리포지셔닝)과 지상 이동을 비교, WHEN 세션 후 설문, THEN 공중 이동을 지상 대비 "무겁다"/"조작권이 없다"고 언급한 비율이 25% 미만 (Core Rule 4 반응성 강화 결정의 효과 검증)
- [ ] **[신규 — 카메라 스냅 가독성]** GIVEN N≥8 플레이테스터가 빠른 마우스룩/스틱 플릭 중 캐릭터 facing 스냅을 관찰, WHEN 세션 후 설문("빠른 시점 전환 시 캐릭터 방향전환이 부자연스럽거나 거슬렸다"), THEN "그렇다" 응답 20% 미만 — 즉시 스냅 회전이 입력 반응성과는 별개로 시각적 부자연스러움을 유발하는지 별도 검증 (33ms 반응성 AC와는 다른 축)
- [ ] **[신규 — 미세 드리프트/떨림 검증]** GIVEN 자동화 하네스가 컨트롤러 yaw에 저진폭 고빈도 오실레이션(±2도, 스틱 데드존 통과/마이크로 드리프트 시뮬레이션)을 주입, WHEN N≥100 프레임 관찰, THEN 캐릭터 메시 yaw가 매 프레임 즉시 스냅되어 육안 확인 시 "떨림(jitter)"으로 보이는지 N≥8 플레이테스터가 15초 클립 시청 후 평가("떨려 보였다" 응답 20% 미만) — 카메라 스냅 가독성 AC(단발성 빠른 회전)와 달리 **지속적 미세 오실레이션** 케이스를 별도로 검증(design-review 2026-07-16 지적 — 두 실패 모드가 다름)

## UI Requirements

N/A — Player Movement 자체는 UI 요소 없음. 이동 관련 표시(스태미나 등 있다면)는 Combat HUD GDD에서 처리.

| Information | Display Location | Update Frequency | Condition |
|-------------|-----------------|-----------------|-----------|

## Cross-References

| This Document References | Target GDD | Specific Element Referenced | Nature |
|--------------------------|-----------|----------------------------|--------|
| 이동이 스펠캐스팅을 막지 않음 | `design/gdd/spell-casting-base.md` | 캐스팅 상태와 이동 상태 독립성 계약 | Rule dependency |
| `MovementLocked` 플래그는 Status Effect만 설정 | `design/gdd/status-effect.md` | 이동 고정(root) 소유권 | Ownership handoff |
| 카메라-상대 이동 기준(컨트롤 요) | `design/gdd/camera-system-base.md` | 컨트롤 요(yaw) 값 | Data dependency |
| 대쉬 임펄스 레이어링 (합성 방식 — additive/override 미정, Open Questions 참조) + Z축 launch/impulse 인터페이스 | `design/gdd/dash-evasion.md` | `AddMovementInput`/velocity 오버라이드 API, `LaunchCharacter()` 또는 동등 API | Data dependency |
| AirControl 오버라이드(체공전투) | `design/gdd/arena-morphing.md` | AirControl 값 | Data dependency |
| 낙하 파편 엄폐는 순수 포지셔닝(신규 로코모션 불필요) | `design/gdd/arena-morphing.md` | 엄폐 판정 볼륨/배치 | Ownership handoff (design-review 2026-07-16 추가) |
| 처형 연출 중 이동은 히트스탑과 동일 철학(MovementLocked 미사용) | `design/gdd/core-extraction-execution.md` | 카메라 컷/애니메이션 연출 | Rule dependency (design-review 2026-07-16 추가) |

## Acceptance Criteria

- [ ] GIVEN 이동입력 크기 X (0.0–1.0), WHEN Grounded 상태, THEN EffectiveSpeed = X × MaxWalkSpeed로 이동
- [ ] GIVEN W+D 동시 입력(대각선), WHEN InputMagnitude 계산, THEN 결과 크기는 1.0으로 정규화되어 EffectiveSpeed가 MaxWalkSpeed를 초과하지 않음 (√2배 과속 방지)
- [ ] GIVEN 카메라가 특정 yaw를 향함, WHEN 이동입력 발생, THEN 이동방향은 카메라 forward/right 기준 계산됨
- [ ] GIVEN 캐릭터가 이동 중, WHEN facing 체크, THEN 캐릭터는 카메라 yaw를 향하고 이동방향과 무관 (`bUseControllerRotationYaw=true` 즉시 반영, 인터폴레이션 없음)
- [ ] GIVEN Grounded 상태에서 점프 입력(JumpZVelocity=600, GravityScale=1.0), WHEN 궤적 계산, THEN AirTime = 1.2245s **±3프레임(±0.05s, 60fps 기준)** 이내
- [ ] **[신규 — 비-기본값 GravityScale 회귀 테스트]** GIVEN Grounded 상태에서 점프 입력(JumpZVelocity=400, GravityScale=1.3, Safe Range 끝단), WHEN 궤적 계산, THEN AirTime = 0.6279s ±3프레임 이내 — GravityScale=1.0(기본값)에서만 검증하면 GravityScale이 제곱 적용되는 회귀(Formulas 섹션 WorldGravityZ 경고 참조)가 숨겨지므로 비-기본값에서 반드시 검증
- [ ] GIVEN GravityScale이 데이터애셋에 0 또는 음수로 설정됨(로드 시) **또는 런타임에 0/음수로 write 시도됨**, WHEN 로드 또는 write, THEN 즉시 0.1로 하드 클램프되고 경고 로그 출력 (division-by-zero/음수 AirTime 방지, 로드 시점과 런타임 write 경로 모두 커버)
- [ ] GIVEN MaxWalkSpeed 또는 JumpZVelocity가 데이터애셋에 0 또는 음수로 설정됨, WHEN 로드, THEN 각각 하드 클램프 최소값(100/100)으로 클램프되고 경고 로그 출력
- [ ] **[신규 — 클램프 대칭성, design-review 2026-07-16]** GIVEN MaxAcceleration, BrakingDecelerationWalking, 또는 GroundFriction이 데이터애셋에 0 또는 음수로 설정됨, WHEN 로드, THEN 각각 하드 클램프 최소값(1000/1000/1)으로 클램프되고 경고 로그 출력 — 이 세 값은 기존에 클램프가 없어 Core Rule 3/4를 동시에 무너뜨릴 수 있었던 갭
- [ ] **[신규 — AirTime Joint Bound, design-review 2026-07-16]** GIVEN GravityScale=0.1(하드 클램프 최소) + JumpZVelocity=800(Safe Range 최대), WHEN 계산된 AirTime(≈16.33s)이 Joint Bound(0.5s~3.0s)를 초과함이 확인됨, THEN 로드 거부(에디터) 또는 경고 로그 후 이전 유효 조합으로 되돌림(런타임). GIVEN JumpZVelocity=100(하드 클램프 최소) + GravityScale=1.3(Safe Range 최대), WHEN 계산된 AirTime(≈0.157s)이 Joint Bound 미만임이 확인됨, THEN 동일하게 거부/되돌림 — 개별 변수 클램프로는 막지 못하는 두 변수의 곱 관계 검증
- [ ] **[신규 — 코요테 타임, design-review 2026-07-16, 착지 버퍼 AC와 대칭, 경계연산자 명시(design-review 2026-07-16 추가)]** 판정식은 `ElapsedTimeSinceLeftGround <= 150.0f`(밀리초, 부동소수점) — 150ms는 **PASS**(윈도우 내 포함), 초과분은 **FAIL**. GIVEN 낭떠러지 이탈 149ms 이내 또는 정확히 150ms 이내 점프 입력, WHEN 낙하 시작, THEN Grounded 점프로 소비되어 실행됨 (**PASS 조건**: 두 케이스 모두 점프 실행). GIVEN 낭떠러지 이탈 151ms 이후 점프 입력(코요테 윈도우 밖), WHEN 낙하 시작, THEN 일반 공중 입력으로 처리되어 점프 미실행 (**FAIL 조건 정의**: 151ms 케이스에서 점프가 실행되면 윈도우 경계 오구현) — 149ms/150ms/151ms **및 150.5ms**(부동소수점 실측 경계 확인용) 네 지점 모두 별도 테스트 케이스로 검증. 테스트 하네스는 프레임 시뮬레이션으로 시간을 근사하지 말고 경과시간 변수에 값을 **직접 주입**(예: 유닛테스트에서 타이머 필드를 149.0f/150.0f/150.5f/151.0f로 직접 set)해 프레임 양자화 오차를 배제
- [ ] **[신규 — Falling→Ascending 재진입, design-review 2026-07-16]** GIVEN Airborne(Falling) 상태에서 외부 Z축 임펄스(대쉬/Launch Character 스텁)가 `Velocity.Z`를 양수로 전환, WHEN 다음 틱 상태 판정, THEN States and Transitions 표에 따라 Airborne(Ascending)로 재진입 — States and Transitions의 "Velocity.Z 부호만이 단일 진실 공급원" 규칙이 재진입 케이스에도 예외 없이 적용됨을 검증
- [ ] **[신규 — 데드존 연속성, design-review 2026-07-16]** GIVEN 공중에서 InputMagnitude=0.02(스틱 중심 통과 시뮬레이션), WHEN AirSteerAccel 및 BrakingDecelerationFalling 계산, THEN 둘 다 0이 아닌 값으로 동시 적용됨(Edge Cases 섹션 명시된 "데드존 없는 연속 전이" 검증) — 자동화 유닛테스트로 InputMagnitude를 0.0/0.02/1.0 세 지점에서 비교
- [ ] **[신규 — 공중 감속/조향 객관적 검증, design-review 2026-07-16]** GIVEN 60fps 유지 상태, WHEN 공중에서 입력이 0으로 전환됨, THEN 자동화 trace 하네스(Input Responsiveness AC와 동일 방식)로 측정한 수평 정지시간이 Formulas 섹션 산출값(600uu/s 기준 약 120ms) ±10% 이내 — 기존에는 이 수치가 주관적 Feel-AC(무겁다 응답 <25%)로만 검증되어 지상 이동의 ramp-shape AC 대비 객관적 검증이 없었음
- [ ] **[신규 — 히트스탑 포지티브 어설션, design-review 2026-07-16]** GIVEN 착지 히트스탑(40ms) 발생, WHEN 동일 프레임에 이동입력 발생, THEN 자동화 trace 하네스로 캡처한 velocity가 비-히트스탑 컨트롤 프레임과 동일한 델타로 갱신됨을 포지티브 어설션 (기존 AC는 Time Dilation 호출 부재만 그렙으로 확인하는 네거티브 검증이었음 — velocity가 실제로 갱신됐는지는 별도 확인 필요)
- [ ] **[신규 — 히트스탑 시각 아티팩트, design-review 2026-07-16, Core Rule 9 필수 요건과 연동, blocking]** GIVEN 착지 히트스탑(40ms) 중 대쉬캔슬 등으로 이동입력이 들어와 실제 액터 위치가 프리즈 중에도 이동하는 케이스, WHEN N≥8 플레이테스터가 해당 순간을 담은 15초 클립(느린 재생 포함)을 시청 후 "포즈가 실제 위치를 따라 미끄러지는 것처럼 보였다" 또는 "언프리즈 시 카메라/캐릭터가 순간 스냅되는 것처럼 보였다" 항목에 응답, THEN 두 항목 모두 "그렇다" 응답 20% 미만 — Core Rule 9의 델타 보정 오프셋 기법이 실제로 시각적 아티팩트를 제거했는지 검증하는 Feel AC(기존에는 velocity 연속성만 AC 대상이었고 시각적 슬라이딩 자체는 게이트 없이 gameplay-programmer 재량이었음)
- [ ] **[정적 검사]** GIVEN Movement 모듈(.h/.cpp) 소스, WHEN CI에서 그렙 기반 정적 검사 실행, THEN Spell Casting 관련 타입/상태에 대한 참조 0건 — Core Rule 7의 "이동이 캐스팅을 참조하지 않음" 아키텍처 불변식 검증 (Spell Casting 시스템 자체의 존재 여부와 무관하게 지금 실행 가능). **보강**: 그렙만으로는 래퍼 함수/인터페이스를 통한 간접 참조를 놓칠 수 있으므로, Movement 모듈의 `Build.cs` `PublicDependencyModuleNames`/`PrivateDependencyModuleNames`에 SpellCasting 모듈이 등재되지 않았음을 함께 검증(모듈 의존성 자체가 없으면 간접 참조도 컴파일 불가하므로 그렙보다 견고함)
- [ ] **[정적 검사 — Core Rule 8]** GIVEN Movement/애니메이션 관련 코드, WHEN CI 그렙 검사, THEN `bStopAllMontages` 또는 몬타주 기반 입력 차단 패턴에 대한 참조 0건. **AND** GIVEN 점프/착지 계열 AnimSequence 애셋, WHEN 애셋 임포트 설정 검사, THEN `bEnableRootMotion`(또는 동등 root motion 추출 설정)이 모두 false — 로코모션 애니메이션이 non-root-motion임을 애셋 레벨에서 검증 (Core Rule 8)
- [ ] **[신규 — 정적 검사, Core Rule 2 회귀 방지, design-review 2026-07-16]** GIVEN Character 클래스의 CMC 설정, WHEN 정적 검사(CDO 프로퍼티 확인 또는 CI 그렙), THEN `bUseControllerRotationPitch == false` **AND** `bUseControllerRotationRoll == false` — Core Rule 2가 명시한 회귀 위험(향후 다른 시스템이 실수로 켤 경우 카메라 피치를 캐릭터 메시가 상속)이 이전엔 서술만 되고 AC로 전환되지 않았던 갭
- [ ] **[신규 — 싱글 점프 강제, Core Rule 4, design-review 2026-07-16]** GIVEN Airborne 상태(코요테/착지버퍼 윈도우 밖), WHEN 추가 점프 입력 발생, THEN 점프가 실행되지 않고 점프 카운트가 증가하지 않음(Double Jump 미지원 회귀 테스트) — Landed → Grounded 전환 시에만 점프 카운트가 리셋됨을 함께 검증
- [ ] **[스텁 단위테스트, 보조]** GIVEN `IsCasting` 상태를 흉내낸 스텁을 유닛테스트 하네스가 true로 직접 주입, WHEN 100틱 동안 이동입력 발생, THEN `AddMovementInput` 결과(velocity 크기/방향)가 `IsCasting=false` 베이스라인과 100% 동일
- [ ] **[MovementLocked 소유권 계약]** GIVEN Status Effect 역할을 흉내낸 스텁이 `MovementLocked=true` 설정, WHEN 이동입력 발생, THEN `AddMovementInput` 무효화됨. GIVEN 동일 플래그를 Spell Casting 역할 스텁이 set 시도, WHEN 호출, THEN 쓰기 거부/무시됨 (유닛테스트로 검증, `tests/unit/movement/`)
- [ ] GIVEN 착지 히트스탑(40ms) 발생, WHEN 동일 프레임에 이동입력 또는 캐스팅 시도, THEN 100% 정상 처리됨 (Time Dilation 미사용 확인 — 정적 검사로 `SetGlobalTimeDilation`/액터 `CustomTimeDilation` 호출이 히트스탑 코드 경로에 없음을 그렙 검사)
- [ ] **[신규 — AnimNotify 슬립 방지, Core Rule 9 추가 주의 승격, design-review 2026-07-16, advisory]** GIVEN 착지 히트스탑(40ms) 윈도우와 겹치는 프레임에 착지 SFX/임팩트 `AnimNotify`가 스케줄됨, WHEN 히트스탑 프리즈 발생, THEN 해당 노티파이가 프리즈 시작 전 미리 트리거되거나 프리즈와 무관한 별도 타임라인으로 발화되어 소리 슬립/누락이 없음을 sound-designer/technical-artist가 재생 확인 — 기존엔 각주로만 남아있던 리스크를 검증 가능한 Edge Case 항목으로 승격(ADVISORY 등급, 이전 리비전에서 blocking 대상은 아님)
- [ ] **[경계연산자 명시(design-review 2026-07-16 추가)]** 판정식은 `TimeBeforeLanding <= 150.0f`(밀리초, 부동소수점) — 150ms는 **PASS**(버퍼 윈도우 내 포함), 초과분은 **FAIL**. GIVEN 착지 149ms 이전 또는 정확히 150ms 이전 점프 입력, WHEN 착지 발생, THEN 버퍼된 점프가 착지 즉시 실행됨 (**PASS 조건**: 두 케이스 모두 점프 실행). GIVEN 착지 151ms 이전(버퍼 윈도우 밖) 점프 입력, WHEN 착지 발생, THEN 점프가 버퍼되지 않아 착지 시 자동 실행되지 않음 (**FAIL 조건 정의**: 151ms 케이스에서 점프가 실행되면 버퍼 윈도우 경계 오구현으로 실패 처리) — 149ms/150ms/151ms **및 150.5ms**(부동소수점 실측 경계 확인용) 네 지점 모두 별도 테스트 케이스로 검증, 타이머 값은 코요테 타임 AC와 동일하게 직접 주입 방식 사용
- [ ] GIVEN 임의의 바닥 콜리전이 런타임에 제거됨(Destructible Geometry 구현 여부와 무관), WHEN 다음 틱 지면 트레이스 실패, THEN Airborne(Falling) 즉시 전환
- [ ] **Performance** (최종 기준 — Blocked): 이동 계산 오버헤드 <0.1ms/actor 목표는 타겟 최소 하드웨어가 `.claude/docs/technical-preferences.md`의 Memory Ceiling에 확정되기 전까지 **최종 pass/fail 게이트로는** 검증 보류. 확정 후: N≥100 동시 액터, Unreal Insights로 `CharacterMovementComponent::TickComponent` 평균 실행시간 캡처(Development 빌드, 300프레임 평균), <0.1ms/actor 기준 적용 (Open Questions 참조)
- [ ] **[신규 — Performance 잠정 프록시 AC(design-review 2026-07-16 추가 — 무기한 Blocked 방치 방지)]** 최종 AC가 하드웨어 미확정으로 보류 중인 동안에도, GIVEN 현재 개발 참조 스펙(개발팀 표준 개발 PC, 사양은 `producer`가 확정)에서 N≥100 동시 액터, WHEN 300프레임 평균 `CharacterMovementComponent::TickComponent` 실행시간 캡처, THEN 결과를 잠정 베이스라인으로 기록(pass/fail 기준 없음, 회귀 추적용). **데드라인**: 타겟 최소 하드웨어 확정과 무관하게 **Production 진입 전(Pre-Production → Production 게이트, `/gate-check` 기준)까지 반드시 1회 이상 측정 완료** — "확정 시"라는 무기한 조건 대신 고정 마일스톤을 데드라인으로 명시
- [ ] **No hardcoded values**: CI 그렙린트가 Movement 관련 .cpp/.h에서 `MaxWalkSpeed`/`JumpZVelocity`/`GravityScale`/`AirControl`/`BrakingDecelerationFalling`/`MaxAcceleration`/`BrakingDecelerationWalking`/`GroundFriction`/버퍼윈도우의 숫자 리터럴 하드코딩 패턴 0건을 검증하고, 해당 데이터애셋 클래스에 각 값이 `UPROPERTY(EditAnywhere)`로 존재함을 정적 어설션으로 확인 (코드 리뷰는 보조 체크일 뿐 게이트 아님)

## Open Questions

| Question | Owner | Deadline | Resolution |
|----------|-------|----------|-----------|
| Camera System(base) 설계 시 상호참조 방향 재확인 필요 | game-designer | Camera System GDD 작성 시 | 미정 |
| Arena Morphing의 AirControl 오버라이드 구체값 | level-designer/systems-designer | Arena Morphing GDD 작성 시 | 미정 |
| Jump Input Buffer Window / Coyote Time Window(둘 다 150ms, 하단 50ms=Frame Budget 예산과 동일해 여유 없음) 실플레이테스트 검증 필요 | qa-tester | 첫 플레이테스트 이후 | 미정 |
| Jump Input Buffer 타이머(Character 소유)를 멀티플레이어 확장 시 `FSavedMove_Character` 예측 파이프라인에 통합할 필요 | network-programmer | 멀티플레이어 스코프 확정 시 | 미정 — 현재 싱글플레이어 스코프에서는 보류 |
| 대쉬 임펄스가 공중에서 유지 중인 momentum에 **가산(additive)**되는지 **덮어쓰기(override)**하는지 — Dash/Evasion GDD 작성 전 확정 필요, 확정 안 하면 예측불가능한 합성 벡터로 "즉시 방향전환" 판타지 훼손 위험 | game-designer | Dash/Evasion GDD 작성 시 | 미정 |
| Double Jump는 이번 리비전에서 MVP 스코프 컷됨(이 문서에서 관련 Core Rule/State/Edge Case/AC 전부 제거) — Progression 시스템이 향후 스코프에 편입되면 네이티브 `JumpMaxCount`(커스텀 병렬 상태머신 대신)로 재도입 검토. **트레이드오프 명시(design-review 2026-07-16 추가)**: 이 컷은 Arena Morphing 보스전(Dash/Evasion의 Z축 launch API로 대체 가능)뿐 아니라 **일반 전투 전반의 공중 기동성**에도 적용됨 — Player Fantasy의 "즉각반응형/상시 조작권" 약속이 지상에 비해 공중에서는 상시 대쉬-재점프 콤보 없이 단일 점프+AirControl로 제한됨을 의미. 이번 리비전은 이 트레이드오프를 의도적으로 수용(스코프 컷 유지)하되, 향후 공중 기동성 부족이 플레이테스트에서 문제로 확인되면 Double Jump 재도입보다 먼저 AirControl 상향 튜닝(Tuning Knobs 참조)으로 대응하는 것을 권장 | producer/game-designer | Progression 시스템 스코프 확정 시 | 재도입 시 JumpMaxCount 기반으로 |
| Performance AC(<0.1ms/actor)는 타겟 최소 하드웨어 확정 전까지 검증 보류(technical-preferences.md Memory Ceiling 미확정) | producer | 타겟 하드웨어 확정 시 | 미정 |
| **[신규, design-review 2026-07-16]** `BrakingDecelerationFalling` 네이티브 매핑(FallingLateralFriction≈0 가정)이 실제 UE5.8 CMC에서 성립하는지 엔진 스파이크로 확인 필요 — 확인 전까지 120ms 손계산치는 잠정치 | gameplay-programmer | 구현 착수 전 | 미정 |
| **[신규, design-review 2026-07-16]** AirTime Joint Bound(0.5s~3.0s) 위반 시 "로드 거부"와 "이전 유효값 되돌림" 중 어느 쪽을 채택할지, 되돌림 시 구체 로직(직전 값 저장 위치 등) 확정 필요 | gameplay-programmer | 구현 착수 전 | 미정 |
| **[신규, design-review 2026-07-16]** `SetMovementModeWithCustomMode()` deprecation 주장은 UE5.8 공식 문서 섹션 단위 인용만 확인됨 — 코드 게이트 전 `CharacterMovementComponent.h` 실물 대조 필요(포스트-cutoff 버전 API 시그니처 주장은 환각 위험 최고) | gameplay-programmer | 코드 리뷰 게이트 전 | 미정 |
