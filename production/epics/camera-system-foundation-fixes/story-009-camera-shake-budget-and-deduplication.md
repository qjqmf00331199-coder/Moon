# Story 009: Camera Shake Budget and Deduplication

> **Epic**: Camera System Foundation Fixes
> **Status**: Ready
> **Layer**: Core
> **Type**: Logic
> **Estimate**: 2-4 hours
> **Manifest Version**: 2026-07-27
> **Last Updated**: Not started

## Context

**GDD**: `design/gdd/camera-system-base.md`
**Requirement**: `TR-cam-008` — 데미지, Supernova, Just-Dodge 카메라 쉐이크와 동시 개수·진폭 제한

**ADR Governing Implementation**: ADR-0005: Camera System SpringArm
**ADR Decision Summary**: `AMoonPlayerCameraManager`가 트리거별 `UCameraShakeBase` 디스패치와 예산을 소유한다. 동시 쉐이크 수와 최종 진폭을 제한하고 동일 클래스 재요청은 누적 스택 대신 기존 인스턴스를 재시작한다.

**Engine**: Unreal Engine 5.8 | **Risk**: LOW
**Engine Notes**: ADR-0005는 표준 `APlayerCameraManager`/`UCameraShakeBase` 경로를 사용하며 post-cutoff API 의존성을 식별하지 않았다. 실제 UE5.8 빌드에서 선택한 shake 시작·중지 API 시그니처를 확인한다.

**Control Manifest Rules (Core layer)**:
- Required: 카메라 쉐이크는 `AMoonPlayerCameraManager`가 소유하고 런타임 파라미터는 데이터 기반으로 제한한다.
- Forbidden: 쉐이크 진폭·동시 개수 제한을 우회하거나 카메라 효과를 게임플레이 판정에 사용하지 않는다.
- Guardrail: 쉐이크 처리는 프레젠테이션 전용이며 이동, 데미지, 캐스트 판단을 차단하지 않는다.

---

## Acceptance Criteria

*GDD Rule 9와 Edge Case 6을 이 스토리 범위로 구체화한다.*

- [ ] 데미지 피격은 피해량에 비례하는 Pitch/Roll 쉐이크를 요청하되 설정된 최대 진폭을 초과하지 않는다.
- [ ] Supernova는 폭발 원점 기준 Radial Camera Shake를 요청하고 거리에 따른 감쇠를 적용한다.
- [ ] Just-Dodge는 짧고 미세한 횡방향 쉐이크를 요청한다.
- [ ] `AMoonPlayerCameraManager`는 데이터로 정의된 동시 활성 쉐이크 상한을 엄격히 지키며 초과 요청이 무제한 중첩되지 않게 한다.
- [ ] 모든 쉐이크의 합성 결과는 데이터로 정의된 진폭 상한을 초과하지 않는다.
- [ ] 활성 상태인 동일 `UCameraShakeBase` 클래스가 다시 요청되면 새 인스턴스를 쌓지 않고 기존 인스턴스를 재시작한다.
- [ ] 쉐이크 디스패치와 예산 로직은 로컬 카메라 프레젠테이션만 변경하며 원본 데미지, Supernova, Just-Dodge 게임플레이 결과를 변경하지 않는다.

---

## Implementation Notes

- 데미지, Supernova, Just-Dodge용 `UCameraShakeBase` 클래스를 구분하고 `AMoonPlayerCameraManager`의 단일 디스패치 경계로 라우팅한다.
- 데미지 스케일은 피해량 비례 계산 후 최대 진폭으로 clamp한다. Supernova는 폭발 원점과 플레이어 카메라 거리를 사용해 radial attenuation을 적용한다.
- 활성 쉐이크 인스턴스와 클래스별 상태를 추적해 동시 상한을 적용한다. 상한 도달 시 정책은 결정적이어야 하며 무제한 추가를 허용하지 않는다.
- 동일 클래스 요청은 기존 인스턴스를 restart하여 수와 진폭을 복리로 누적하지 않는다.
- 동시 상한과 진폭 상한은 런타임 하드코딩이 아니라 카메라 설정 데이터에서 읽을 수 있어야 한다.
- 이벤트 구독은 PlayerCameraManager 수명에 맞춰 정리하고 서버 권한 게임플레이 상태를 카메라 코드에서 수정하지 않는다.

---

## Out of Scope

- 데미지 계산, Supernova 폭발 판정, Just-Dodge 성공 판정 자체
- 최종 카메라 쉐이크 에셋의 미학적 튜닝과 플랫폼별 접근성 프리셋
- Story 006의 Overdrive FOV 및 Story 007의 처형 블렌드
- 카메라 외 VFX, 오디오, 컨트롤러 진동

---

## QA Test Cases

- **AC-1/AC-2/AC-3: 트리거별 쉐이크 라우팅**
  - Given: 데미지, Supernova, Just-Dodge 이벤트를 독립적으로 발생시킬 수 있는 테스트 하네스
  - When: 각 이벤트를 한 번씩 발생시킨다.
  - Then: 데미지는 피해 비례 Pitch/Roll, Supernova는 거리 감쇠 radial, Just-Dodge는 짧은 횡방향 클래스를 각각 요청한다.
  - Edge cases: 피해량 0/최대 초과, Supernova 원점과 동일 위치/감쇠 범위 밖, 연속 Just-Dodge 요청을 검증한다.

- **AC-4/AC-5: 동시 개수와 진폭 상한**
  - Given: 설정 데이터에 유한한 동시 쉐이크 상한과 진폭 상한이 정의되어 있다.
  - When: 서로 다른 쉐이크 요청을 같은 프레임과 짧은 구간에 상한보다 많이 발생시킨다.
  - Then: 활성 인스턴스 수와 합성 진폭이 각각의 상한을 한 번도 초과하지 않는다.
  - Edge cases: 상한 정확히 도달, 상한+1 요청, 매우 큰 피해 스케일, 여러 Supernova 동시 폭발을 검증한다.

- **AC-6: 동일 클래스 재시작**
  - Given: 한 쉐이크 클래스의 인스턴스가 이미 활성 상태다.
  - When: 같은 클래스를 다시 요청한다.
  - Then: 활성 인스턴스 수는 증가하지 않고 기존 인스턴스의 재생 상태만 처음부터 재시작된다.
  - Edge cases: 같은 프레임 중복 요청과 종료 직전 재요청에서도 중복 인스턴스가 생기지 않는다.

- **AC-7: 게임플레이 경계 유지**
  - Given: 각 원본 이벤트의 게임플레이 결과를 기록하는 테스트 더블
  - When: 쉐이크 예산이 요청을 제한하거나 동일 클래스를 재시작한다.
  - Then: 데미지 수치, Supernova 판정, Just-Dodge 성공 결과는 카메라 정책 때문에 변경되지 않는다.
  - Edge cases: 카메라 매니저나 쉐이크 에셋이 없는 환경에서도 게임플레이 이벤트는 정상 완료된다.

---

## Test Evidence

**Story Type**: Logic
**Required evidence**:
- `tests/unit/camera/camera_shake_budget_and_deduplication_test.ps1` — 트리거 라우팅, concurrent cap, amplitude cap, same-class restart 검증
- `production/qa/evidence/camera-shake-budget-and-deduplication-evidence.md` — 세 트리거의 PIE 표시 및 멀미 유발 중첩 부재 확인

**Status**: [ ] Not yet created

---

## Dependencies

- Depends on: Story 002 (Look Input and Pitch Clamp)
- Unlocks: 데미지, Supernova, Just-Dodge 생산 이벤트의 카메라 피드백 연동
