# Camera System (base)

> **Status**: Approved
> **Author**: user + game-designer
> **Last Updated**: 2026-07-16
> **Implements Pillar**: Dopamine Driven Design — physical foundation for responsive aiming, dynamic battlefield awareness, and cinematic executions
> **Creative Director Review (CD-GDD-ALIGN)**: APPROVED 2026-07-16 — Verdict: APPROVED (Gemini Antigravity design-review)

## Summary

Camera System (base)는 플레이어의 조작 및 전투 상황에 따라 캐릭터를 추적하고 화면을 제어하는 3인칭(Third-Person) 스프링 암 카메라 시스템이다. Player Movement와 상호참조 관계를 가지며, 스펠 에이밍의 정밀함과 공중 체공 전투(Arena Morphing) 시의 시야 확보를 책임진다.

> **Quick reference** — Layer: `Core` · Priority: `MVP` · Key deps: [Player Movement](file:///D:/moon-fragment-hunt/design/gdd/player-movement.md) (mutual)

---

Camera System (base)는 플레이어의 조작 및 전투 상황에 따라 캐릭터를 추적하고 화면을 제어하는 3인칭(Third-Person) 스프링 암 카메라 시스템이다. 이 시스템은 단순한 시야 확보를 넘어, 플레이어가 스펠을 정확히 조준하고, 공중 대쉬와 체공 전투(Arena Morphing) 상황에서 전장 인지능력을 유지할 수 있도록 하는 전투의 중추 역할을 수행한다.

본 시스템은 다음과 같은 핵심 역할을 담당한다:
1. **카메라 제어 및 플레이어 추적**: `USpringArmComponent`와 `UCameraComponent`를 사용하여 플레이어 캐릭터의 위치를 부드럽고 정확하게 추적한다.
2. **카메라-상대(Camera-Relative) 입력 처리**: 플레이어가 입력하는 W/A/S/D(또는 아날로그 스틱) 이동 벡터를 카메라의 현재 Yaw 회전값을 기준으로 변환하여, 플레이어가 바라보는 시점 기준으로 직관적인 이동이 가능하게 한다.
3. **Player Movement와의 상호참조 해결**: Player Movement 시스템은 이동 기준축 계산과 캐릭터 정면 방향(Facing) 결정을 위해 카메라의 컨트롤러 Yaw를 읽는다. 반대로 카메라 시스템은 캐릭터의 위치를 읽어 스프링 암의 TargetArmLength 및 소켓 오프셋을 업데이트한다. 두 시스템은 독립적으로 `PlayerController`의 Rotation을 참조하므로 실행 순서 의존성이 없으며, 순환 참조 오류가 발생하지 않도록 상호 데이터 의존성을 격리한다.
4. **환경 충돌(Collision) 처리**: 레벨 내 정적 구조물 및 파괴 가능한 지오메트리(Destructible Geometry)와의 충돌 시 카메라가 뚫고 들어가지 않도록 스프링 암의 프로브 충돌(`bDoCollisionTest`)을 설정하되, 파괴된 조각들로 인한 빈번한 시야 가림과 지터링(Jitter)을 방지하기 위해 특정 콜리전 채널은 예외 처리한다.

이 문서는 MVP 단계에서 필요한 기본적인 3인칭 카메라 시스템과 관련 조작감에 대해 명세하며, 향후 처형 컷신(Execution Camera/Cutscene) 이나 보스전 변형(Arena Morphing) 단계의 특수 카메라 거동으로 확장할 수 있는 인터페이스를 제공한다.

---

## 2. Player Fantasy

플레이어는 카메라가 자신의 조작 의도를 완벽히 이해하는 "보이지 않는 전술적 동반자"처럼 작동한다고 느껴야 한다. 급박한 공중 전투나 순간적인 회피 중에도 시야가 적을 놓치지 않아야 하며, 마법 스펠을 조준할 때 카메라가 미세하게 떨리거나 답답하게 따라오지 않고 즉각적이고 부드럽게 조준점을 동기화해야 한다.

* **시야의 명확함 (Clarity of Vision)**: 플레이어가 대쉬하거나 스킬을 난사할 때 카메라는 안정감을 유지해야 한다. 불필요하게 카메라 흔들림(Head Bobbing)이 과도하거나 급격하게 회전하여 멀미를 유발하지 않으며, 전장의 상황을 언제나 한눈에 파악할 수 있는 최적의 쿼터뷰/백뷰 구도를 유지한다.
* **속도감과 오버드라이브의 해방감**: 루나 오버드라이브 각성 시, 카메라의 시야각(FOV)이 일시적으로 넓어지며 플레이어에게 더 큰 공간적 여유와 초월적인 속도감을 제공한다. 
* **처형의 카타르시스**: 코어 적출 처형(F키) 발동 시, 카메라는 즉시 3인칭 추적 모드에서 극적인 클로즈업 컷신 구도로 부드럽게 전환(Blend)되어, 플레이어가 적의 가슴에서 코어를 뜯어내는 강렬한 시각적 충격을 스크린에 가득 담아 전달한다.

**Feel Reference**:
* *NieR:Automata*: 부드러운 카메라 래그(Lag) 속도와 플레이어 캐릭터를 중앙에서 살짝 비껴 프레이밍하여 전방 시야를 넓게 보여주는 3인칭 숄더뷰.
* *Devil May Cry 5*: 스타일리시하고 빠른 이동에 완벽히 호응하는 카메라 트래킹, 오버드라이브/특수 스킬 시의 역동적인 FOV 확대.

**Anti-Reference**:
* *Souls-like* 게임의 락온 카메라 오작동: 거대 몬스터와 벽 사이에 끼었을 때 카메라가 요동치며 벽 내부를 뚫고 보거나 캐릭터를 완전히 가려버리는 현상.
* *God of War (2018)* 스타일의 지나치게 밀착된 숄더뷰: 근접 타격감은 좋으나 360도 전방위에서 몰려드는 적을 상대해야 하는 스펠 위빙 전투 방식에는 부적합하여 지양한다.

---

## 3. Detailed Rules

### Rule 1: 카메라 컴포넌트 계층 구조 (Component Hierarchy)
* 카메라는 플레이어 캐릭터 클래스(`APlayerCharacter`) 내에 계층형 컴포넌트로 장착된다.
* **캡슐 컴포넌트(Root)** -> **스프링 암 컴포넌트(`USpringArmComponent`)** -> **카메라 컴포넌트(`UCameraComponent`)** 순으로 연결된다.
* 스프링 암의 Pivot 포인트(Relative Location)는 캐릭터의 발밑이 아닌 상반신(기본값 `Z = 60.0 uu`)에 고정되어 카메라 회전 시 캐릭터 중심의 자연스러운 시야를 보장한다.

### Rule 2: Enhanced Input 기반의 회전 입력 매핑
* 마우스 및 게임패드 아날로그 스틱의 2축 입력을 위해 `IA_Look` (Value Type: Axis2D) 액션을 사용한다.
* `IA_Look.X` (Yaw) 입력은 플레이어 컨트롤러의 Yaw 입력(`APlayerController::AddControllerYawInput`)으로 라우팅되어 월드 Z축 기준으로 카메라를 수평 회전시킨다.
* `IA_Look.Y` (Pitch) 입력은 플레이어 컨트롤러의 Pitch 입력(`APlayerController::AddControllerPitchInput`)으로 라우팅되어 카메라를 수직 회전시킨다.
* **수직 회전(Pitch) 제한**: 지면을 뚫고 들어가거나 카메라가 뒤집히는 것을 방지하기 위해 Pitch 값은 최소 `-60.0`도(아래에서 위를 올려다보는 한계), 최대 `30.0`도(위에서 아래를 굽어보는 한계)로 강제 클램프한다. 이 제한은 `APlayerCameraManager` 서브클래스에서 관리한다.

### Rule 3: 카메라-상대(Camera-Relative) 이동 입력 계산
* 플레이어의 8방향 이동 입력(W/A/S/D 또는 좌측 아날로그 스틱)은 캐릭터 자체의 forward/right 벡터가 아니라, **카메라 컨트롤러의 Yaw 회전각**을 기준으로 변환된다.
* 입력 처리 틱마다 컨트롤러의 Rotation 값을 획득한 후, Pitch와 Roll을 제거한 Yaw만을 남긴 회전 행렬(Rotator)을 구한다.
* 해당 Rotator로부터 Forward Vector와 Right Vector를 얻어, 캐릭터의 `AddMovementInput` API에 전달한다. 이로 인해 플레이어가 화면 위쪽 방향(W)을 누르면 카메라도 캐릭터의 뒤쪽에서 앞쪽으로 전진하는 힘을 즉시 가한다.

### Rule 4: 스프링 암 속성 및 트래킹 래그 (Camera Lag)
* 스프링 암 컴포넌트는 `bUsePawnControlRotation = true`로 설정되어 컨트롤러 회전을 상속받는다.
* 수평 수직 회전을 위해 `bInheritPitch = true`, `bInheritYaw = true`를 켜되, 화면의 지평선이 기우는 것을 막기 위해 **`bInheritRoll = false`**를 강제한다.
* **위치 및 회전 래그(Lag)**: 순간적인 대쉬나 공중 발사(Launch) 시 카메라가 캐릭터에 너무 딱딱하게 붙어 화면이 심하게 요동치는 것을 막기 위해 `bEnableCameraLag = true`와 `bEnableCameraRotationLag = true`를 활성화한다.
* **오프스크린 방지 (CameraLagMaxDistance)**: 고속 대쉬나 순간적인 날아가기(Launch) 시 캐릭터가 화면 밖으로 이탈하여 조작감을 잃는 현상을 차단하기 위해 `CameraLagMaxDistance = 200.0 uu`를 설정한다. 이 거리를 초과하면 카메라 lag 연산이 일시 중지되고 스프링 암이 캐릭터를 즉시 강제 추적한다.

### Rule 5: 환경 충돌 테스트 및 파괴물(Debris) 예외 처리
* 스프링 암의 `bDoCollisionTest = true`를 설정하여 맵 구조물 내부를 카메라가 뚫고 보지 못하게 한다. 프로브 반경(`ProbeSize`)은 `12.0 uu`로 설정한다.
* 충돌 쿼리 채널은 `ECC_Camera`를 사용한다.
* **파괴 지오메트리 예외 처리**: [Destructible Geometry](file:///D:/moon-fragment-hunt/design/gdd/systems-index.md) 기믹(다리 붕괴, 포자 폭발 등)으로 인해 발생하는 수많은 물리 파편들이 카메라의 충돌 테스트를 차단해 카메라가 캐릭터 쪽으로 갑자기 튀는 래피드 스냅(Rapid Snap) 아티팩트를 방지해야 한다. 따라서, 모든 파괴물 조각(Geometry Collection / Chaos Destructible)의 Collision Object Type은 `ECC_Destructible` 또는 커스텀 채널로 분류하고, **카메라 스프링 암의 충돌 응답 채널에서 이를 무시(Ignore)**하도록 구성한다.

### Rule 6: Player Movement 연동 캐릭터 회전 스냅 (Facing)
* [Player Movement GDD](file:///D:/moon-fragment-hunt/design/gdd/player-movement.md) 계약에 따라, 캐릭터의 회전 상태는 다음과 같이 제어된다:
  * `bOrientRotationToMovement = false`
  * `bUseControllerRotationYaw = true`
  * `bUseControllerRotationPitch = false`
  * `bUseControllerRotationRoll = false`
* 캐릭터는 매 틱 플레이어 컨트롤러의 Yaw 값을 즉시 100% 반영하여 회전 스냅(회전 속도 보간 없음)된다. 이를 통해 스트레이프 중에도 스펠 조준 방향이 카메라 중앙 조준점과 한치의 오차 없이 즉시 유지된다.

### Rule 7: 루나 오버드라이브 FOV 확장
* 루나 오버드라이브(Luna Overdrive) 각성 상태가 활성화되면, 플레이어의 속도감과 시야 확장을 극대화하기 위해 카메라의 FOV(Field Of View)를 기본 `90.0`도에서 `100.0`도로 부드럽게 넓힌다.
* 오버드라이브 진입/해제 시 시야각 전환은 `FMath::FInterpTo`를 이용해 각각 `0.5초`(진입) 및 `0.8초`(해제) 동안 보간 처리하여 시각적인 스무딩을 보장한다.

### Rule 8: 처형 컷신(Core Extraction Execution) 전환
* F키 처형이 발동되면 카메라는 3인칭 추적 모드에서 연출 카메라 시점으로 즉시 블렌딩된다.
* 이때 스프링 암의 `TargetArmLength`를 `150.0 uu`로 축소하고 캐릭터의 우측 숄더백 뷰(`SocketOffset = (X=0, Y=40, Z=20)`)로 카메라를 오프셋하며, 이 블렌딩은 `0.2초` 내에 빠르게 완료되어 처형 연출의 속도감을 저해하지 않는다.
* 처형 중에도 [Player Movement GDD](file:///D:/moon-fragment-hunt/design/gdd/player-movement.md)에 따라 캐릭터는 움직일 수 있으므로(히트스탑 방식), 연출 카메라는 플레이어의 실제 무브먼트 궤적을 왜곡하지 않고 단지 렌더링 오버레이 블렌딩만 제공하며, 처형 완료 시 원래 스프링 암 셋업으로 `0.3초`에 걸쳐 복구된다.

### Rule 9: 카메라 쉐이크(Camera Shake) 피드백
* 특정 위력이상의 이벤트 발생 시 `UCameraShakeBase`를 플레이하여 물리적 타격감을 화면에 전달한다.
  * **데미지 피격**: 받는 피해량에 비례하여 Pitch/Roll 방향으로 흔들림 유발 (최대 흔들림 진폭 강도 제약).
  * **초신성(Supernova) 폭발**: Radial Camera Shake를 폭발 원점 기준으로 재생하여 거리에 따라 쇠퇴(Attenuation) 적용.
  * **저스트 회피(Just-Dodge)**: 미세하고 빠른 횡방향 흔들림을 주어 순간적인 회피 성공 피드백 전달.

---

## 4. Formulas

### Formula 1: 카메라-상대(Camera-Relative) 입력 벡터 공식
플레이어가 입력한 이동 값 $\mathbf{I} = (I_x, I_y)$ (여기서 $I_x$는 전진/후진, $I_y$는 좌측/우측)과 카메라 컨트롤 회전값 $\mathbf{R} = (R_{pitch}, R_{yaw}, R_{roll})$이 주어졌을 때, 실제 월드 이동 입력 벡터 $\mathbf{V}_{final}$은 다음과 같이 계산된다.

1. Pitch와 Roll을 제거한 Yaw 전용 회전값 계산:
   $$\mathbf{R}_{yaw} = (0.0, R_{yaw}, 0.0)$$

2. 회전 행렬을 기준으로 한 Forward 및 Right 단위 벡터 추출:
   $$\mathbf{U}_{forward} = \text{FRotationMatrix}(\mathbf{R}_{yaw}).\text{GetUnitAxis}(\text{EAxis::X})$$
   $$\mathbf{U}_{right} = \text{FRotationMatrix}(\mathbf{R}_{yaw}).\text{GetUnitAxis}(\text{EAxis::Y})$$

3. 가중치 적용 임시 입력 벡터 합산:
   $$\mathbf{V}_{raw} = (\mathbf{U}_{forward} \times I_x) + (\mathbf{U}_{right} \times I_y)$$

4. 대각선 이동 시 속도 증가를 방지하기 위한 크기 클램핑 (1.0 기준):
   $$\mathbf{V}_{final} = \begin{cases} 
   \frac{\mathbf{V}_{raw}}{\|\mathbf{V}_{raw}\|} & \text{if } \|\mathbf{V}_{raw}\| > 1.0 \\
   \mathbf{V}_{raw} & \text{otherwise} 
   \end{cases}$$

### Formula 2: 시야각(FOV) 부드러운 보간 공식
게임 틱 마다 현재 시야각 $\text{FOV}_{current}$를 목표 시야각 $\text{FOV}_{target}$으로 업데이트하는 보간 공식이다.

$$\text{FOV}_{new} = \text{FMath::FInterpTo}(\text{FOV}_{current}, \text{FOV}_{target}, \Delta t, \text{InterpSpeed})$$

* **루나 오버드라이브 각성 시**:
  * $\text{FOV}_{target} = 100.0$, $\text{InterpSpeed} = 6.0$ (목표값 도달에 약 `0.5`초 소요)
* **루나 오버드라이브 종료 시**:
  * $\text{FOV}_{target} = 90.0$, $\text{InterpSpeed} = 4.0$ (목표값 복구에 약 `0.8`초 소요)

### Formula 3: 카메라 위치 래그 및 제한 공식
스프링 암 컴포넌트 내부에서 연산되는 카메라 위치 보간 및 최대 거리 클램프 공식이다.

1. 프레임당 기본 보간 연산 (언리얼 스프링 암 내부 동작):
   $$\mathbf{L}_{current}(t) = \mathbf{L}_{current}(t - \Delta t) + (\mathbf{L}_{target}(t) - \mathbf{L}_{current}(t - \Delta t)) \times (1 - e^{-\text{LagSpeed} \times \Delta t})$$

2. 최대 래그 거리($\text{MaxDistance} = 200.0 \text{ uu}$) 클램프 연산:
   $$\text{Distance} = \|\mathbf{L}_{current}(t) - \mathbf{L}_{target}(t)\|$$
   $$\mathbf{L}_{final}(t) = \begin{cases}
   \mathbf{L}_{target}(t) + \frac{\mathbf{L}_{current}(t) - \mathbf{L}_{target}(t)}{\text{Distance}} \times \text{MaxDistance} & \text{if } \text{Distance} > \text{MaxDistance} \\
   \mathbf{L}_{current}(t) & \text{otherwise}
   \end{cases}$$

### Formula 4: 수직 회전(Pitch) 클램프 공식
플레이어의 수직 회전 조작 각도 $\theta_{pitch}$를 강제 제약하는 공식이다.

$$\theta_{final\_pitch} = \text{FMath::Clamp}(\theta_{pitch}, \theta_{min}, \theta_{max})$$

* $\theta_{min} = -60.0$도 (최대 앙각, 올려다보기)
* $\theta_{max} = 30.0$도 (최대 부각, 내려다보기)

### Formula 5: 처형 컷신 렌즈 블렌딩 공식
처형 씬 진입 시 스프링 암 길이($D$)와 소켓 오프셋 벡터($\mathbf{S}$)의 보간 연산이다.

$$D_{new} = \text{FMath::FInterpTo}(D_{current}, D_{target}, \Delta t, 15.0)$$
$$\mathbf{S}_{new} = \text{FMath::VInterpTo}(\mathbf{S}_{current}, \mathbf{S}_{target}, \Delta t, 15.0)$$

* **진입 단계**:
  * $D_{target} = 150.0 \text{ uu}$, $\mathbf{S}_{target} = (0, 40, 20) \text{ uu}$
* **복구 단계**:
  * $D_{target} = 350.0 \text{ uu}$ (기본값), $\mathbf{S}_{target} = (0, 0, 0) \text{ uu}$ (기본값)
  * 복구 시에는 부드러움을 위해 $\text{InterpSpeed} = 10.0$을 사용한다.

---

## 5. Edge Cases

### Edge Case 1: 좁은 모퉁이/구석에 갇혔을 때 캐릭터 메시 가림 및 클리핑
* **상황**: 플레이어가 벽을 등지거나 좁은 모퉁이에 갇히면, 스프링 암 충돌 테스트로 인해 카메라가 캐릭터 메시 방향으로 매우 가깝게 스냅된다. 이로 인해 카메라가 캐릭터 메시 내부로 뚫고 들어가 내부 폴리곤이 보이거나(클리핑) 플레이어 시야가 완전히 가려져 전장 인지가 불가능해진다.
* **해결책**: 카메라와 캐릭터 사이의 실시간 거리(Spring Arm의 `TargetArmLength`가 아닌 실제 계산된 Spring Arm의 길이)가 `80.0 uu` 이하로 감소하면, 플레이어 캐릭터 메시의 Material에 디더링(Dither Temporal AA) 페이드 효과를 강제로 적용하여 캐릭터를 반투명(또는 완전 투명)하게 만들어 시야를 확보한다. 또한, 카메라 렌즈의 근접 클리핑 평면(Near Clipping Plane)은 `10.0 uu`로 설정하여 캐릭터의 극단적인 근접 상황에서도 클리핑 현상을 원천 차단한다.

### Edge Case 2: 순간이동(Teleport) 또는 체크포인트 리스폰 시 카메라 트래킹 지연
* **상황**: 플레이어가 사망하여 먼 거리의 체크포인트로 리스폰하거나, 향후 포탈 등을 통해 순간이동할 경우, 활성화되어 있는 카메라 위치 래그(Camera Position Lag)로 인해 카메라가 순간이동 전 위치에서 이동 후 위치까지 비정상적으로 느리게 스위핑(Sweeping)하며 월드를 관통해 지나가는 아티팩트가 발생한다.
* **해결책**: 캐릭터의 리스폰 또는 순간이동 이벤트 발생 시, PlayerController 또는 CharacterBlueprint에서 스프링 암 컴포넌트의 **`ResetCameraLag()`** 함수를 반드시 명시적으로 호출한다. 이를 통해 카메라 래그의 보간 캐시를 즉시 지우고 새 위치로 프레임 드랍 없이 동기화한다.

### Edge Case 3: 환경 파괴물(Debris) 잔해더미로 인한 카메라 지터링
* **상황**: 다리 붕괴(Chap 2 Bridge) 또는 수정 기둥 파괴 등 [Destructible Geometry](file:///D:/moon-fragment-hunt/design/gdd/systems-index.md) 기믹이 발생하면 공중에 수십 개의 물리 파편이 비산한다. 이 파편들이 카메라의 충돌 판정 채널(`ECC_Camera`)을 블로킹하도록 설정되어 있으면 파편이 카메라와 캐릭터 사이를 지나갈 때마다 카메라가 앞뒤로 격렬하게 튀는 지터링(Jitter) 현상이 일어난다.
* **해결책**: 모든 파괴 가능한 환경 파편(Chaos Geometry Collection)의 콜리전 채널은 `ECC_Destructible`로 지정하며, 캐릭터 스프링 암의 충돌 채널 반응에서 `ECC_Destructible`은 무조건 **Ignore**로 설정한다. 카메라는 오직 정적 콜리전(`WorldStatic`), 파괴되지 않는 지형지물(`WorldDynamic`)에만 반응하여 충돌을 회피해야 한다.

### Edge Case 4: 플레이어 캐릭터 초고속 날아가기(Launch) 시 오프스크린
* **상황**: 보스전 아레나 몰핑 페이즈 등에서 캐릭터가 천장으로 빠르게 발사(`LaunchCharacter`로 `Z축` 속도 `3000 uu/s` 이상 적용)되는 경우, 카메라 래그 속도가 이를 따라가지 못해 캐릭터가 카메라의 상단 화면 경계 밖(Off-screen)으로 일시적으로 사라지는 문제가 발생한다.
* **해결책**: 스프링 암의 `CameraLagMaxDistance`를 `200.0 uu`로 제한하여 아무리 속도가 빠르더라도 카메라와 캐릭터 간의 절대적 거리가 이 값을 초과할 수 없도록 강제한다. 이 한계 거리에 도달하면 보간 연산 대신 하드 플로우(Hard Follow)로 강제 고정되어 캐릭터가 항상 화면 내에 유지된다.

### Edge Case 5: 처형 컷신 도중 카메라 회전 조작 주입
* **상황**: F키 코어 적출 처형 연출(Camera Blend) 도중 플레이어가 마우스나 스틱을 격렬하게 휘두르면, 블렌딩 대상인 연출 카메라의 구도와 플레이어의 룩 입력이 충돌하여 화면이 떨리거나 원치 않는 회전이 적용될 위험이 있다.
* **해결책**: 처형 시퀀스가 시작(`State.Executable` 소비 및 처형 상태 진입)되는 즉시 플레이어 컨트롤러의 **룩 입력 수신을 일시적으로 차단(Ignore Look Input)**한다. 처형 애니메이션 몽타주가 완전히 종료되고 기본 카메라 뷰로 블렌딩이 끝나는 시점에 룩 입력을 다시 재개(Enable Look Input)한다.

### Edge Case 6: 카메라 쉐이크(Camera Shake) 중첩으로 인한 멀미/화면 왜곡
* **상황**: 여러 마리의 적이 동시에 폭발하여 초신성(Supernova) 카메라 쉐이크가 짧은 시간 내에 여러 번 중첩 호출되면, 카메라 진폭이 무제한으로 복리 연산되어 화면이 지나치게 왜곡되거나 멀미를 유발할 수 있다.
* **해결책**: 플레이어 카메라 매니저에서 활성화할 수 있는 동시 카메라 쉐이크 개수를 엄격하게 제한하고, 새로운 쉐이크가 주입될 때 기존 쉐이크의 최대 강도를 초과하지 않도록 **진폭 제한(Amplitude Limit)** 및 스케일링을 강제한다. 동일 클래스의 쉐이크는 기존 인스턴스를 초기화(Restart)하는 방식으로 처리하여 누적 중첩을 방지한다.

---

## 6. Dependencies

본 시스템은 게임의 핵심 이동 및 연출 시스템들과 깊은 상호 연동 관계를 지닌다. 양방향 종속성이 올바르게 해소되었는지 검증하기 위해 하위 테이블을 명세한다.

| 연관 시스템 | 의존 방향 | 의존성 성격 | 상호 언급 여부 및 링크 |
|---|---|---|---|
| **Player Movement** | 상호 의존 (양방향, 순환 참조 없음) | * **Movement**는 이동 벡터 및 Facing 계산을 위해 카메라의 컨트롤러 Yaw를 참조함.<br>* **Camera**는 추적을 위해 캐릭터의 액터 위치를 참조함.<br>* 두 시스템은 PlayerController의 ControlRotation을 독립적으로 조회하므로 컴파일/런타임 실행 순서의 교착 상태가 없음. | [Player Movement GDD Dependencies](file:///D:/moon-fragment-hunt/design/gdd/player-movement.md#L142-L152)에서 본 문서를 상호참조함. |
| **Dash/Evasion** (미설계) | Dash/Evasion이 본 시스템에 의존 | * 대쉬 기동 시 카메라-상대(Camera-relative) 벡터를 기반으로 대쉬 임펄스를 가함.<br>* 대쉬 성공(저스트 회피) 시 화면에 횡방향 카메라 쉐이크를 재생함. | 향후 Dash/Evasion GDD 설계 시 본 문서의 조작 기준 명세를 참조하도록 기술 예정. |
| **Core Extraction Execution** (미설계) | Execution이 본 시스템에 의존 | * 처형 프롬프트(F키)가 활성화되었을 때, 적의 위치를 스크린 좌표로 투영(Project)하여 UI에 프롬프트를 정확히 오버레이함.<br>* 처형 실행 시 연출 시점 블렌딩(TargetArmLength 150uu 축소, SocketOffset 우숄더 이동) 기능을 호출함. | 향후 Core Extraction Execution GDD 설계 시 본 문서의 컷신 카메라 블렌딩 API를 참조하도록 기술 예정. |
| **Arena Morphing** (미설계) | Arena Morphing이 본 시스템에 의존 | * 지반이 무너지고 솟아오르는 Z축 체공 전투 상황에서 급격히 캐릭터가 날아갈 때 오프스크린이 발생하지 않도록 `CameraLagMaxDistance = 200uu`에 의존함. | 향후 Arena Morphing GDD 설계 시 본 문서의 최대 래그 거리 제약 조건을 참조하도록 기술 예정. |
| **Destructible Geometry** (미설계) | 본 시스템이 Destructible Geometry에 의존 | * 파괴물 조각들이 카메라 시야 및 충돌 시스템을 차단해 떨림이 발생하는 것을 방지하기 위해, 파괴 채널(`ECC_Destructible`)의 Ignore 필터 설정에 의존함. | 향후 Destructible Geometry GDD 설계 시 카메라 채널 충답 설정 규칙을 포함시킬 예정. |

---

## 7. Tuning Knobs

카메라 시스템의 주요 파라미터는 하드코딩되지 않고 외부 데이터 에셋(예: `DA_CameraSettings`)을 통해 튜닝 가능해야 한다.

| Parameter | Current Value | Safe Range | Effect of Increase | Effect of Decrease |
|---|---|---|---|---|
| `TargetArmLength` | 350.0 uu | 250.0 ~ 500.0 uu | 캐릭터가 작게 보이고 시야가 넓어져 전황 파악이 쉽지만 피격 및 세부 모션의 실감도 하락. | 캐릭터가 크게 보이고 현장감이 상승하지만 적들의 배치 파악이 어려워짐. |
| `CameraSocketOffset` | (X=0.0, Y=0.0, Z=0.0) | Y: -50 ~ 50, Z: -30 ~ 50 | 카메라의 중심축이 오른쪽/위쪽으로 밀려 비대칭 숄더뷰 연출 가능. | 카메라의 중심축이 왼쪽/아래쪽으로 밀림. |
| `CameraPitchMin` | -60.0 deg | -80.0 ~ -45.0 deg | 플레이어가 캐릭터 발밑에서 하늘을 더 수직에 가깝게 올려다볼 수 있음. | 수직 앙각이 제한되어 하늘을 덜 쳐다보게 됨. |
| `CameraPitchMax` | 30.0 deg | 15.0 ~ 45.0 deg | 공중에서 아래를 바라보는 각도가 가팔라져 Z축 수직 낙하 중 정밀도가 올라감. | 지면을 바라보는 부각 한계가 좁아져 답답함을 유발할 수 있음. |
| `CameraLagSpeed` | 10.0 | 5.0 ~ 20.0 | 카메라 추적이 매우 신속해지고 지연이 최소화됨. 과도할 경우 즉각반응성 스냅처럼 움직여 떨림 발생 가능. | 카메라 추적이 부드러워져 모션 멀미가 감소하나 빠른 대쉬 시 캐릭터가 화면 구석으로 밀림. |
| `CameraRotationLagSpeed` | 15.0 | 8.0 ~ 25.0 | 마우스/스틱 회전에 따른 시점 이동 스무딩이 신속해짐. | 마우스 조작 시 화면이 한 템포 미끄러지듯(Slippery) 늦게 회전하는 불쾌감 유발. |
| `CameraLagMaxDistance` | 200.0 uu | 100.0 ~ 400.0 uu | 고속 급 가속 시 캐릭터가 카메라 중심에서 더 멀리 벗어나 날아가는 속도감 증폭. | 캐릭터가 항상 카메라 중앙 근처에 엄격하게 강제 고정되어 딱딱해짐. |
| `BaseFOV` | 90.0 deg | 80.0 ~ 100.0 deg | 주변 시야각이 넓어지고 주변부가 다소 왜곡되나 공간감 및 속도감 상승. | 시야가 중앙으로 집중되어 정밀 조준은 편해지나 전장 상황 인지도가 떨어짐. |
| `OverdriveFOV` | 100.0 deg | 95.0 ~ 110.0 deg | 오버드라이브 각성 시 스피드 포스 터널 효과(Warp-speed)가 강력하게 연출됨. | 각성 상태 시 시각적 폭발력이 밋밋해짐. |
| `ExecutionArmLength` | 150.0 uu | 100.0 ~ 250.0 uu | 처형 시 몬스터의 얼굴 및 타격 위치가 한 화면에 가득 찬 극적인 시네마틱 프레임 연출. | 처형 연출 시 카메라 줌인이 너무 약해 일반 전투 상태와 차별화되지 않음. |
| `CameraProbeSize` | 12.0 uu | 5.0 ~ 25.0 uu | 벽 충돌 회피가 더 넓은 반경에서 확실히 일어나 벽 투과 현상을 완전히 차단함. | 좁은 틈새에 카메라가 더 오래 머무르나, 벽 근접 시 모서리가 일부 투과될 수 있음. |

---

## 8. Acceptance Criteria

본 시스템의 작동 및 예외 처리를 검증하기 위한 QA 테스트용 구체적 수락 기준이다.

### 1. 카메라-상대(Camera-Relative) 입력 검증 (QA-TEST-01)
* **GIVEN**: 플레이어가 월드 내에 스폰되어 있고 카메라가 임의의 수평 회전각(Yaw = 0도, 45도, 90도, 180도)을 향하고 있다.
* **WHEN**: 플레이어가 전진 키(W) 또는 아날로그 스틱을 위로 100% 입력한다.
* **THEN**: 캐릭터는 카메라 렌즈가 비추는 Forward 단위 벡터 방향을 축으로 하여 이동해야 한다. (오차 범위 ±1.0도 이내)

### 2. 수평 대각선 이동 입력 벡터 제한 검증 (QA-TEST-02)
* **GIVEN**: 플레이어가 대각선 이동(W+D 키 동시 입력 또는 아날로그 스틱 대각선 100% 입력)을 한다.
* **WHEN**: 캐릭터 무브먼트에 주입되는 원본 입력 벡터의 크기를 실시간 검증한다.
* **THEN**: 입력 벡터의 크기는 대각선 입력 합산 시 발생하는 $\sqrt{2}$배 과속을 방지하기 위해 반드시 `1.0`으로 정규화/클램프되어야 한다.

### 3. 캐릭터 즉시 snap 회전 검증 (QA-TEST-03)
* **GIVEN**: 캐릭터가 멈춰 서 있거나 특정 방향으로 이동 중이다.
* **WHEN**: 플레이어가 마우스를 빠르게 스냅 회전하거나 아날로그 스틱을 튕겨 컨트롤러 Yaw 값을 순간적으로 변화시킨다.
* **THEN**: 캐릭터 메쉬의 전방 방향(Facing)은 보간 지연 시간(Smooth Lerp) 없이 해당 프레임(1틱 이내)에 컨트롤러 Yaw 회전값과 즉시 일치해야 한다. (지연 프레임수 = 0)

### 4. 수직 회전 한계(Pitch Limit) 클램프 검증 (QA-TEST-04)
* **GIVEN**: 플레이어가 마우스나 아날로그 스틱을 최대로 위 또는 아래로 쳐다보도록 조작한다.
* **WHEN**: 플레이어 컨트롤러의 Rotation Pitch 값을 확인한다.
* **THEN**: 카메라는 수직 각도가 아래를 내려다볼 때 `30.0`도, 위를 올려다볼 때 `-60.0`도를 절대 초과하지 않고 해당 경계값에서 정지해야 한다.

### 5. 위치 래그 한계 거리 제약 검증 (QA-TEST-05)
* **GIVEN**: 캐릭터의 위치 래그(`bEnableCameraLag = true`)가 활성화된 상태이다.
* **WHEN**: 캐릭터가 급격한 대쉬나 공중 발사(Launch)로 가속되어 카메라 중심으로부터 벌어진다.
* **THEN**: 카메라와 캐릭터 간의 월드 거리는 `CameraLagMaxDistance`인 `200.0 uu`를 절대로 초과할 수 없으며, 한계 도달 시 카메라는 지연 없이 캐릭터를 하드 트래킹해야 한다.

### 6. 순간이동(Teleport) 시 카메라 스위핑 방지 검증 (QA-TEST-06)
* **GIVEN**: 캐릭터가 임의의 위치 A에서 $1000.0 \text{ uu}$ 이상 떨어진 위치 B로 순간이동(또는 체크포인트 리스폰)한다.
* **WHEN**: 리스폰 로직에서 `ResetCameraLag()`를 호출한다.
* **THEN**: 리스폰 직후 첫 프레임 렌더링 시 카메라는 순간이동된 위치 B에 오차 없이 즉시 동기화되어 있어야 하며, A에서 B로 벽을 뚫고 미끄러지는 시각적 아티팩트가 없어야 한다.

### 7. 파괴물 잔해물 충돌 무시 검증 (QA-TEST-07)
* **GIVEN**: 환경 구조물이 무너져 캐릭터와 카메라 사이를 가로막거나 지나가는 물리 파편(Geometry Collection)이 발생한다.
* **WHEN**: 파편 조각이 카메라 충돌 판정 프로브 영역을 통과한다.
* **THEN**: 카메라의 스프링 암 길이(`TargetArmLength`)는 `350.0 uu` 상태를 정상 유지해야 하며, 파편으로 인해 카메라가 캐릭터 쪽으로 갑작스럽게 스냅되는 현상이 전혀 없어야 한다.

### 8. 루나 오버드라이브 FOV 확장 보간 검증 (QA-TEST-08)
* **GIVEN**: 플레이어가 루나 오버드라이브 각성 상태를 활성화한다.
* **WHEN**: 카메라의 실시간 FOV(Field Of View) 값을 틱 단위로 로깅한다.
* **THEN**: FOV가 기본 `90.0`도에서 `100.0`도로 부드럽게 증가해야 하며, 목표값에 도달하는 데 걸리는 총 시간은 `0.5`초(오차 범위 ±0.05초)여야 한다. 비활성화 시 `90.0`도로 부드럽게 돌아오는 시간은 `0.8`초(오차 범위 ±0.08초)여야 한다.

### 9. 처형 컷신 진입 및 조작 잠금 검증 (QA-TEST-09)
* **GIVEN**: 플레이어가 처형(F키)을 발동하여 처형 상태(`State.Invulnerable` 획득)에 진입한다.
* **WHEN**: 스프링 암 속성의 변화와 플레이어 조작 여부를 관찰한다.
* **THEN**:
  1. 스프링 암 길이는 `0.2`초 내에 `150.0 uu`로 줌인되고, 소켓 오프셋은 우측 숄더 구도인 `(X=0, Y=40, Z=20)`으로 동시 블렌딩되어야 한다.
  2. 처형 연출 프레임 동안 플레이어의 시선 회전 조작(Look Input)은 무시(Ignore)되어 화면 회전 왜곡이 발생하지 않아야 한다.
  3. 처형 종료 후 원래 3인칭 기본 시점(`TargetArmLength = 350.0 uu`, `SocketOffset = (0,0,0)`)으로 `0.3`초에 걸쳐 정상 복귀되며, 회전 조작 기능도 복구되어야 한다.

### 10. 좁은 벽면 근접 시 메시 디더링 검증 (QA-TEST-10)
* **GIVEN**: 플레이어가 뒷벽에 밀착하여 카메라 스프링 암의 실제 길이가 `80.0 uu` 이하로 수축한다.
* **WHEN**: 화면 내의 플레이어 캐릭터 메시의 렌더링 상태를 관찰한다.
* **THEN**: 플레이어 캐릭터의 메시는 디더링 페이드(Dither Temporal AA) 처리를 통해 투명화 또는 반투명화되어 전방 시야를 가리지 않아야 하며, 렌더링 화면에 캐릭터 내부 폴리곤(뼈대, 빈 껍데기 등)이 노출되지 않아야 한다.
