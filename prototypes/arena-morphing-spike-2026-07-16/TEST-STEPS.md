// PROTOTYPE - NOT FOR PRODUCTION
// Question: Nanite 메시가 Chaos Geometry Collection으로 파괴 가능하고, 붕괴+발판 다수 동시 처리 시 60fps 유지되는가?
// Date: 2026-07-16

# 테스트 절차 (Unreal Editor 5.8, 수동 진행)

MCP 서버(localhost:8000) 미연결 상태 — 에디터 켜져 있지 않음. 아래 절차 직접 진행 후 결과를 알려주면 SPIKE-NOTE.md 갱신함.

## 0. 준비

- `Moon.uproject` 열기 (C++ Source 모듈 없음 확인됨 — Blueprint로 진행)
- 임시 테스트 레벨 생성: `L_Spike_ArenaMorph` (프로덕션 맵과 분리, 나중에 통째로 삭제 가능)

## 1. Nanite 바닥 배치

- 큰 평면 static mesh 배치 (예: 2000×2000×100 큐브) — Static Mesh 에디터에서 **Nanite Enabled** 체크 확인
- 이게 "평탄했던 투기장" 역할

## 2. Geometry Collection 변환 — 핵심 검증 지점

- 메시 선택 → **Fracture Mode** 진입 (없으면 Edit > Plugins에서 `Fracture` 플러그인 활성화, 에디터 재시작)
- Uniform Voronoi 패턴으로 피스 40개 내외 프랙처
- **여기서 확인할 것 (제일 중요):**
  - Geometry Collection Component 디테일 패널에 Nanite 관련 옵션이 있는가, 있으면 켜지는가?
  - Nanite 켠 상태로 프랙처 후 뷰포트에서 조각들이 그대로 Nanite로 렌더되는가, 아니면 자동으로 non-Nanite 폴백되는가?
  - 에디터가 경고/에러를 띄우면 **원문 그대로 복사해서 보고할 것** (내가 5.8 문서로 대조 필요)

## 3. 붕괴 트리거 (보스 슬램 시뮬레이션)

- Geometry Collection 근처에 **Field System Actor** 배치, preset: `Radial Falloff + Uniform Vector` (Chaos Field 템플릿)
- 레벨 블루프린트 또는 트리거 볼륨에서: 키 입력(예: 스페이스바) 시 Field 활성화 → GC에 방사형 임펄스 적용
- Geometry Collection의 Anchoring은 비워둠 (트리거 전까지 정적, 트리거 후 전체 시뮬레이션)

## 4. 떠오르는 발판 5개

- 단순 StaticMeshActor(박스) 5개, 바닥 아래 배치
- 각각에 미니 Blueprint: 트리거 이벤트 시 **Timeline**으로 Z축 위치 lerp (바닥 아래 → 바닥 위, 약 1.5초)
- 신규 액터 클래스 하나로 5개 인스턴스 재사용 (복붙 금지, 이 정도는 재사용해도 스파이크 범위 내)

## 5. 플레이어 무한 대쉬

- ThirdPerson 템플릿 기본 캐릭터 사용
- 캐릭터 BP에서 대쉬 바인딩: **Launch Character** 노드를 전방 임펄스로, 쿨다운/충전 제한 없이 매 입력마다 실행되게 하드코딩
- 목적: 체공 상태에서 무너지는 지형 위를 대쉬로 이동 가능한지 "느낌" 확인 (feel 검증은 이번 스파이크 핵심 아님, 사이드 체크)

## 6. 측정

콘솔에 순서대로 입력:
```
stat unit
stat fps
stat Chaos
```

다음 상황에서 각각 프레임타임 기록:
- (a) 대기 상태 (베이스라인)
- (b) 40피스 붕괴 트리거 직후 (가장 무거운 순간 예상)
- (c) 붕괴 + 발판 5개 상승 동시 진행 중
- (d) 플레이어가 파편 사이로 공중 대쉬 중 (c와 겹칠 때)

## 7. 보고할 내용 (이거만 답해주면 됨)

1. Nanite 조각이 그대로 유지됐는가, non-Nanite로 강제 전환됐는가, 아니면 옵션 자체가 없었는가?
2. 가장 무거운 순간의 프레임타임 (ms) — 16.6ms 넘었는가?
3. 에디터 경고/에러 원문 (있으면)
4. YES / NO / PARTIAL 한 줄 판정 + 이유
