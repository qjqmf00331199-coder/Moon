# Story 006: Overdrive FOV Integration

> **Epic**: Camera System Foundation Fixes
> **Status**: Ready
> **Layer**: Core
> **Type**: Integration
> **Estimate**: 2-4 hours
> **Manifest Version**: 2026-07-27
> **Last Updated**: Not started

## Context

**GDD**: `design/gdd/camera-system-base.md`
**Requirement**: `TR-cam-006` — 오버드라이브의 시작/종료 이벤트에 따라 카메라 FOV를 보간하는 동적 카메라 모드

**ADR Governing Implementation**: ADR-0005: Camera System SpringArm
**ADR Decision Summary**: `AMoonCharacterBase`가 `SetOverdriveFOVActive(bool)` 상태와 Tick 기반 `FMath::FInterpTo` 보간을 소유하고, Luna Overdrive의 `OnOverdriveStarted`/`OnOverdriveEnded` 이벤트가 이를 구동한다. 런타임 값은 `UMoonCameraSettings`에서 읽는다.

**Engine**: Unreal Engine 5.8 | **Risk**: LOW
**Engine Notes**: ADR-0005는 장기 안정 API인 `UCameraComponent`와 `FMath::FInterpTo`만 사용하며 별도 post-cutoff API 위험을 식별하지 않았다. 실제 빌드와 PIE에서 이벤트 바인딩 및 시간 오차를 검증한다.

**Control Manifest Rules (Core layer)**:
- Required: 카메라 런타임 설정은 `UMoonCameraSettings` 데이터 에셋에서 읽는다.
- Forbidden: 런타임 FOV와 보간 값을 C++ 하드코딩 값으로 진실의 원천으로 삼지 않는다.
- Guardrail: FOV 보간은 프레젠테이션 전용이며 이동, 데미지, 캐스트 판정을 차단하지 않는다.

---

## Acceptance Criteria

*GDD Rule 7, Formula 2, QA-TEST-08을 이 스토리 범위로 구체화한다.*

- [ ] Luna Overdrive의 `OnOverdriveStarted`가 발생하면 카메라가 `UMoonCameraSettings`의 기본 FOV `90.0`도에서 오버드라이브 FOV `100.0`도로 단조롭게 부드럽게 증가한다.
- [ ] 시작 이벤트부터 목표 FOV 도달까지의 시간은 `0.5초 ±0.05초`이다.
- [ ] `OnOverdriveEnded`가 발생하면 카메라가 `100.0`도에서 `90.0`도로 단조롭게 부드럽게 복귀한다.
- [ ] 종료 이벤트부터 기본 FOV 복귀까지의 시간은 `0.8초 ±0.08초`이다.
- [ ] 이벤트 연결은 `SetOverdriveFOVActive(true/false)`를 통해 이뤄지며 카메라 보간은 이동, 데미지, 캐스트 또는 오버드라이브 게임플레이 상태의 판정을 소유하거나 차단하지 않는다.
- [ ] PIE 틱 로그가 시작/종료 양방향의 FOV 값과 경과 시간을 재현 가능하게 기록한다.

---

## Implementation Notes

- `AMoonCharacterBase::SetOverdriveFOVActive(bool bActive)`가 목표 FOV 상태만 전환하게 하고, 실제 FOV 갱신은 캐릭터 Tick의 활성 보간 구간에서 수행한다.
- `OnOverdriveStarted`에는 `true`, `OnOverdriveEnded`에는 `false`를 연결한다. 이벤트 수명에 맞춰 바인딩을 해제하여 중복 구독을 만들지 않는다.
- 목표값과 보간 속도는 `UMoonCameraSettings`에서 읽는다. GDD Formula 2의 기준은 진입 `InterpSpeed=6.0`, 복귀 `InterpSpeed=4.0`이며, QA의 최종 판정은 실제 도달 시간 허용오차로 한다.
- 이미 목표값에 도달한 동안에는 불필요한 보간 작업을 중지한다.
- 카메라는 로컬 프레젠테이션 상태만 변경하고 Overdrive의 Active/Recovery 상태나 지속시간을 변경하지 않는다.

---

## Out of Scope

- Story 007: 처형 카메라 블렌드와 룩 입력 억제
- Story 008: 순간이동/체크포인트 복원 시 카메라 래그 캐시 초기화
- Story 009: 카메라 쉐이크 예산과 중복 제거
- Luna Overdrive의 10초 Active/1.5초 Recovery 게임플레이 규칙 변경

---

## QA Test Cases

- **AC-1/AC-2: 오버드라이브 진입 FOV 보간**
  - Given: 기본 FOV가 `90.0`도이고 Luna Overdrive가 Inactive인 PIE 플레이어
  - When: `OnOverdriveStarted`를 발생시키고 매 틱 FOV와 경과 시간을 기록한다.
  - Then: FOV가 역행 없이 `100.0`도에 도달하고 도달 시간이 `0.45~0.55초`이다.
  - Edge cases: 프레임률을 변화시켜도 시간 허용오차를 만족하며, 시작 이벤트 중복 수신이 보간 시간을 비정상적으로 누적하지 않는다.

- **AC-3/AC-4: 오버드라이브 종료 FOV 복귀**
  - Given: 카메라 FOV가 `100.0`도이고 Overdrive가 Active인 상태
  - When: `OnOverdriveEnded`를 발생시키고 매 틱 FOV와 경과 시간을 기록한다.
  - Then: FOV가 역행 없이 `90.0`도로 복귀하고 도달 시간이 `0.72~0.88초`이다.
  - Edge cases: 진입 보간 도중 종료 이벤트가 와도 현재 FOV에서 기본 FOV로 연속적으로 복귀하며 튀는 프레임이 없다.

- **AC-5: 프레젠테이션 경계 유지**
  - Given: 이동 입력과 스펠 입력이 가능한 상태에서 Overdrive 이벤트를 발생시킨다.
  - When: FOV 진입 및 복귀 보간 중 입력과 게임플레이 상태를 관찰한다.
  - Then: 이동, 데미지, 캐스트 판정은 카메라 보간 때문에 차단되거나 변경되지 않는다.
  - Edge cases: 보간 중 캐릭터가 대쉬하거나 피격되어도 FOV 상태 외 게임플레이 값은 카메라 코드가 쓰지 않는다.

---

## Test Evidence

**Story Type**: Integration
**Required evidence**:
- `tests/integration/camera/overdrive_fov_integration_test.ps1` — 이벤트 연결, 목표 FOV, 진입/복귀 시간 허용오차 검증
- `production/qa/evidence/overdrive-fov-integration-evidence.md` — PIE 틱 로그와 부드러운 전환 육안 확인

**Status**: [ ] Not yet created

---

## Dependencies

- Depends on: Story 001 (Camera Settings and Component Hierarchy)
- Unlocks: None
