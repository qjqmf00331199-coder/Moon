# Story 007: Execution Camera Blend and Look Suppression

> **Epic**: Camera System Foundation Fixes
> **Status**: Ready
> **Layer**: Core
> **Type**: Integration
> **Estimate**: 2-4 hours
> **Manifest Version**: 2026-07-27
> **Last Updated**: Not started

## Context

**GDD**: `design/gdd/camera-system-base.md`
**Requirements**: `TR-cam-006`, `TR-cam-008` — 처형 연출 카메라의 진입/복귀 블렌드와 처형 구간의 룩 입력 억제

**ADR Governing Implementation**: ADR-0005: Camera System SpringArm
**ADR Decision Summary**: `AMoonCharacterBase`가 Tick 기반 `FInterpTo`/`VInterpTo` 실행 카메라 상태를 소유하고 `BeginExecutionCameraBlend()`/`EndExecutionCameraBlend()` 전방 인터페이스를 제공한다. 룩 입력만 처형 경계에서 억제하며 실제 캐릭터 이동 궤적은 건드리지 않는다.

**Engine**: Unreal Engine 5.8 | **Risk**: LOW
**Engine Notes**: SpringArm 속성 보간과 컨트롤러 입력 억제는 장기 안정 API 범위다. Core Extraction Execution GDD와 실제 호출자는 아직 없으므로 이 스토리는 호출 가능한 전방 인터페이스와 테스트 하네스까지 제공한다.

**Control Manifest Rules (Core layer)**:
- Required: 처형 카메라 값은 `UMoonCameraSettings` 기반이며 카메라 전용 상태로 구현한다.
- Forbidden: 처형 프레젠테이션을 위해 이동 입력, 캡슐, CMC 또는 전역/액터 시간 배율을 차단하지 않는다.
- Guardrail: 카메라 블렌드는 이동, 데미지, 캐스트 판단을 소유하지 않는 렌더링 오버레이여야 한다.

---

## Acceptance Criteria

*GDD Rule 8, Formula 5, Edge Case 5, QA-TEST-09를 이 스토리 범위로 구체화한다.*

- [ ] `BeginExecutionCameraBlend()` 호출 후 `0.2초` 내에 SpringArm `TargetArmLength`가 `150.0 uu`, `SocketOffset`이 `(X=0, Y=40, Z=20)`으로 함께 블렌딩된다.
- [ ] 처형 진입과 동시에 룩 입력만 무시되어 마우스/스틱 회전이 연출 구도를 흔들지 않는다.
- [ ] 처형 블렌드 중 이동 입력, 캐릭터 캡슐, CharacterMovementComponent 틱과 실제 이동 궤적은 차단·정지·왜곡되지 않는다.
- [ ] `EndExecutionCameraBlend()` 호출 후 `0.3초`에 걸쳐 데이터 에셋의 기본 시점인 `TargetArmLength=450.0 uu`, `SocketOffset=(0,45,20)`으로 복귀한다.
- [ ] 기본 시점 복귀가 완료되면 룩 입력이 반드시 재개되며, 반복 진입/종료에도 Ignore Look Input 상태가 남지 않는다.
- [ ] Core Extraction Execution은 공개된 Begin/End 인터페이스만 호출하면 되며 카메라 내부 보간 상태를 직접 수정하지 않는다.

---

## Implementation Notes

- `AMoonCharacterBase::BeginExecutionCameraBlend()`와 `EndExecutionCameraBlend()`를 외부 시스템용 전방 인터페이스로 제공한다.
- 진입 목표는 `ExecutionArmLength=150.0 uu`와 `(0,40,20)`, 복귀 목표는 설정 에셋의 기본 `450.0 uu`와 `(0,45,20)`이다. C++ 리터럴은 런타임 진실의 원천이 아니다.
- GDD Formula 5에 따라 arm length는 `FMath::FInterpTo`, offset은 `FMath::VInterpTo` 계열의 Tick 상태로 함께 갱신한다.
- Begin에서 룩 입력 억제를 시작하고 End의 기본 시점 복귀 완료 경계에서 해제한다. 이동 입력 억제 API는 호출하지 않는다.
- 처형 호출자가 아직 설계되지 않았으므로 생산 호출자를 추측해 만들지 않는다. 자동화/PIE 테스트 하네스에서 Begin/End 계약을 직접 호출할 수 있게 한다.
- 취소·중단 경로도 End와 같은 정리 경계를 사용하여 룩 입력 잠금이 누수되지 않게 한다.

---

## Out of Scope

- Core Extraction Execution의 처형 판정, 대상 선정, 몽타주 및 최종 생산 호출자
- 처형 중 게임플레이 이동 잠금 또는 히트스탑 규칙 변경
- Story 006의 Overdrive FOV 보간
- Story 009의 카메라 쉐이크 정책

---

## QA Test Cases

- **AC-1/AC-2: 처형 카메라 진입과 룩 억제**
  - Given: 기본 시점 `450.0 uu`, `(0,45,20)`에서 룩 입력이 활성화된 PIE 플레이어
  - When: `BeginExecutionCameraBlend()`를 호출하고 강한 마우스/스틱 룩 입력을 주입한다.
  - Then: `0.2초` 내에 `150.0 uu`, `(0,40,20)`에 도달하고 룩 입력이 구도를 회전시키지 않는다.
  - Edge cases: 진입 중 Begin이 다시 호출되어도 별도 블렌드가 중첩되거나 룩 억제 카운트가 누수되지 않는다.

- **AC-3: 실제 이동 비차단**
  - Given: 처형 카메라 진입 전부터 일정한 이동 입력을 유지한다.
  - When: Begin부터 End 및 복귀 완료까지 캐릭터 위치, 속도, CMC 틱 상태를 기록한다.
  - Then: 카메라 처리가 이동 입력을 무시하거나 캡슐/CMC를 정지하지 않고 실제 이동 궤적이 계속된다.
  - Edge cases: 방향 전환 입력도 룩 입력 억제와 독립적으로 처리된다.

- **AC-4/AC-5: 복귀와 입력 복원**
  - Given: 처형 시점에 진입해 룩 입력이 억제된 상태
  - When: `EndExecutionCameraBlend()`를 호출한다.
  - Then: `0.3초`에 걸쳐 `450.0 uu`, `(0,45,20)`으로 복귀한 뒤 룩 입력이 정상 작동한다.
  - Edge cases: 중단/취소 경로와 연속 두 번의 Begin/End 사이클 후에도 룩 입력이 잠긴 채 남지 않는다.

---

## Test Evidence

**Story Type**: Integration
**Required evidence**:
- `tests/integration/camera/execution_camera_blend_test.ps1` — Begin/End 인터페이스, 목표값, 시간, 이동 비차단, 룩 복원 검증
- `production/qa/evidence/execution-camera-blend-and-look-suppression-evidence.md` — QA-TEST-09 PIE 영상/프레임 로그

**Status**: [ ] Not yet created

---

## Dependencies

- Depends on: Story 001 (Camera Settings and Component Hierarchy)
- Unlocks: 향후 Core Extraction Execution 생산 호출자 연동
