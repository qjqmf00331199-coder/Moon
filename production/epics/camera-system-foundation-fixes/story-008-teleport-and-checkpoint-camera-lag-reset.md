# Story 008: Teleport and Checkpoint Camera Lag Reset

> **Epic**: Camera System Foundation Fixes
> **Status**: Ready
> **Layer**: Core
> **Type**: Integration
> **Estimate**: 2-4 hours
> **Manifest Version**: 2026-07-27
> **Last Updated**: Not started

## Context

**GDD**: `design/gdd/camera-system-base.md`
**Requirement**: `TR-cam-007` — 순간이동 및 체크포인트 리스폰 시 `ResetCameraLag()`로 보간 캐시를 비우는 경로

**ADR Governing Implementation**: ADR-0005: Camera System SpringArm (primary); ADR-0002: Runtime Checkpoint Persistence Strategy (secondary)
**ADR Decision Summary**: ADR-0005는 모든 텔레포트/리스폰 경계에서 SpringArm `ResetCameraLag()` 호출을 요구한다. ADR-0002의 `UMoonCheckpointSubsystem::RestoreCheckpoint()`가 플레이어 Transform을 복원하는 경계이므로 해당 복원 흐름이 카메라 래그 초기화 훅을 호출해야 한다.

**Engine**: Unreal Engine 5.8 | **Risk**: LOW (camera) / HIGH (checkpoint integration)
**Engine Notes**: `USpringArmComponent::ResetCameraLag()`는 ADR-0005가 채택한 경로다. ADR-0002의 체크포인트 서브시스템은 Accepted 설계이나 구현 시 UE5.8 GAS 복원 경계를 실제 빌드로 검증해야 하며, 이 스토리는 속성 복원 방식 자체를 변경하지 않는다.

**Control Manifest Rules (Core layer)**:
- Required: 순간이동 및 체크포인트 리스폰 경로에서 `ResetCameraLag()`를 호출한다.
- Forbidden: 체크포인트 복원 중 AttributeSet 원시 값을 직접 쓰지 않으며 카메라가 복원 게임플레이 상태를 소유하지 않는다.
- Guardrail: 래그 캐시 초기화는 위치 복원 경계의 프레젠테이션 후처리이며 이동·데미지 판정을 차단하지 않는다.

---

## Acceptance Criteria

*GDD Edge Case 2와 QA-TEST-06을 이 스토리 범위로 구체화한다.*

- [ ] 위치 A에서 `1000.0 uu` 이상 떨어진 위치 B로 텔레포트한 직후 `ResetCameraLag()`가 호출된다.
- [ ] ADR-0002의 `UMoonCheckpointSubsystem::RestoreCheckpoint()`가 플레이어 Transform을 복원하는 경계에도 같은 카메라 래그 초기화 훅이 연결된다.
- [ ] 위치 변경 후 첫 렌더 프레임에서 카메라는 위치 B의 새 추적 목표에 즉시 동기화되어 이전 위치 A의 보간 캐시를 사용하지 않는다.
- [ ] A에서 B로 카메라가 스위핑하며 벽이나 월드를 관통하는 중간 프레임이 관찰되지 않는다.
- [ ] 래그 초기화는 플레이어 Transform과 체크포인트 속성 복원 결과를 다시 쓰거나 변경하지 않는다.
- [ ] 일반 텔레포트와 체크포인트 리스폰 양쪽이 동일한 명시적 카메라 래그 초기화 계약을 사용한다.

---

## Implementation Notes

- 위치 변경을 완료한 직후, 첫 카메라 갱신 전에 `CameraBoom->ResetCameraLag()`를 호출하는 단일 캐릭터 측 훅을 제공한다.
- `UMoonCheckpointSubsystem::RestoreCheckpoint()`의 Transform 복원 경계는 해당 훅을 호출한다. ADR-0002가 요구하는 `ResetDeathState()`와 GameplayEffect 속성 복원 순서는 그대로 보존한다.
- 일반 텔레포트 호출자도 동일 훅을 재사용해야 하며, 카메라 컴포넌트 내부 상태를 외부 시스템이 직접 조작하게 하지 않는다.
- 호출 순서는 `Transform 적용 -> Camera lag reset -> 첫 카메라 프레임`이 되도록 한다.
- 이미 래그가 비활성화된 경우에도 호출은 안전하고 결과가 멱등적이어야 한다.

---

## Out of Scope

- ADR-0002 전체 체크포인트 캡처/속성 복원 시스템 구현 또는 재설계
- 디스크 SaveGame, 레벨 로드, 네트워크 리스폰
- 일반 이동과 대쉬 중의 카메라 래그 튜닝
- Story 007의 처형 카메라 블렌드

---

## QA Test Cases

- **AC-1/AC-3/AC-4: 일반 텔레포트 첫 프레임 동기화**
  - Given: 카메라 위치 래그가 활성화된 플레이어가 위치 A에 있다.
  - When: 플레이어를 A에서 `1000.0 uu` 이상 떨어진 B로 이동하고 래그 초기화 훅을 호출한다.
  - Then: 첫 렌더 프레임의 카메라가 B의 추적 목표에 동기화되고 A-B 사이를 스위핑하는 프레임이 없다.
  - Edge cases: 여러 축을 동시에 이동하는 텔레포트, 래그 비활성 상태, 연속 텔레포트에서도 이전 캐시가 남지 않는다.

- **AC-2/AC-5: 체크포인트 복원 연동**
  - Given: ADR-0002 스냅샷이 있고 현재 위치에서 `1000.0 uu` 이상 떨어진 체크포인트 Transform이 저장되어 있다.
  - When: `UMoonCheckpointSubsystem::RestoreCheckpoint()`로 플레이어를 복원한다.
  - Then: Transform 복원 후 첫 카메라 프레임 전에 래그가 초기화되며, 카메라 훅은 복원된 Transform·Health·Mana·Tension 값을 변경하지 않는다.
  - Edge cases: 활성 체크포인트가 없어 Restore가 거부되는 경로에서는 카메라 훅이 잘못 호출되지 않는다.

- **AC-6: 호출 경로 일원화**
  - Given: 일반 텔레포트와 체크포인트 리스폰 테스트 호출자
  - When: 두 경로를 각각 실행한다.
  - Then: 두 호출자가 동일한 명시적 래그 초기화 훅을 사용하며 SpringArm 상태를 제각각 직접 수정하지 않는다.
  - Edge cases: 동일 프레임 중복 호출은 안전하고 최종 카메라 상태를 바꾸지 않는다.

---

## Test Evidence

**Story Type**: Integration
**Required evidence**:
- `tests/integration/camera/teleport_checkpoint_camera_lag_reset_test.ps1` — `1000.0 uu` 이상 이동, 체크포인트 경계, 첫 프레임 동기화 검증
- `production/qa/evidence/teleport-and-checkpoint-camera-lag-reset-evidence.md` — QA-TEST-06 PIE 프레임 캡처 또는 영상

**Status**: [ ] Not yet created

---

## Dependencies

- Depends on: Story 004 (Camera Lag Hard Limit); ADR-0002 체크포인트 복원 경계
- Unlocks: 텔레포트 및 체크포인트 리스폰의 카메라 스위핑 없는 통합 검증
