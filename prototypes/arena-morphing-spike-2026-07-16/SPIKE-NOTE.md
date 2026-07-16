# Spike: Arena Morphing — Nanite + Chaos Destruction

**Date:** 2026-07-16
**Type:** Mid-production performance/technical spike (not a concept prototype — no GDD/gate implications)
**Related concept:** Moon Fragment Hunt — 최종장 아레나 몰핑 (arena-morphing-concept, see game-concept.md)

## Question

Nanite 메시를 Chaos Geometry Collection으로 변환해 파괴 가능한 바닥으로 쓸 수 있는가?
붕괴 + 다수의 떠오르는 발판이 동시에 존재할 때 60fps(16.6ms) 예산을 지킬 수 있는가?

Two sub-questions:
1. **Compat**: UE5.8에서 Nanite 메시 → Geometry Collection 변환 시 Nanite 렌더링이 유지되는가, 아니면 조각(fragment)들이 non-Nanite로 강제 전환되는가?
2. **Perf**: 40+ 파편 동시 파괴 + 5개 발판 상승 + 플레이어 공중 대쉬가 겹칠 때 프레임타임이 16.6ms 이내인가?

## Why this first

앞선 기술 검토(unreal-specialist)에서 6개 시스템 중 유일하게 "확인 안 된 항목"으로 플래그됨.
나머지 5개는 GAS/Chaos/Niagara/Enhanced Input 표준 패턴 — 이미 검증된 조합. 이것만 미지수.
프로덕션 순서를 정하기 전에 가장 리스크 큰 가정부터 깨야 함.

## Scope (cut everything else)

- [ ] Nanite 활성화 대형 static mesh 1개 → Fracture Mode로 Geometry Collection 변환 (~40 피스)
- [ ] Field System 트리거 1개 (보스 슬램 시뮬레이션용 Radial Force)
- [ ] 떠오르는 발판 5개 (단순 BP 타임라인 Z축 lerp)
- [ ] 플레이어 무한 대쉬 (쿨다운 제거한 Launch Character 바인딩)
- [ ] `stat unit` / `stat fps` / `stat Chaos` 프레임타임 측정

**Cut:** 실제 보스, 애니메이션, 사운드, UI, 탄막, 조명 반전, 메뉴 — 전부 다음 단계.

## Project note

Moon 프로젝트에 C++ Source 모듈 없음 (Blueprint-only 상태). 계획은 Blueprint 수동 진행이었으나,
실제로는 `unreal-mcp` (Moon.uproject에 `AllToolsets` 플러그인 추가 후 재시작) 통해 에디터를 직접
스크립팅으로 조작함. 절차 원안은 `TEST-STEPS.md` 참고 (수동 백업 경로로 유효).

## Progress so far (2026-07-16, unreal-mcp 스크립팅)

1. `/Engine/BasicShapes/Cube2` → `/Game/_Spikes/SM_SpikeCube` 로 프로젝트 로컬 복제, Nanite 활성화 확인
   (`is_nanite_enabled: true`)
2. `DataflowAgent` 툴셋으로 `/Game/_Spikes/GC_SpikeFloor` GeometryCollection 생성 —
   템플릿 `DF_GC_Template_StaticMesh` 사용 (Epic 제공 템플릿, 이미 `StaticMeshToCollection →
   UniformFracture → GeometryCollectionTerminal` 그래프가 구성돼 있음)
3. **핵심 발견 #1**: GeometryCollection 애셋 자체에 `enableNanite` / `bEnableNaniteFallback`
   불리언 프로퍼티가 네이티브로 존재함 (reflection으로 확인). 5.8에서 Nanite 옵션 자체가
   사라지지 않았음 — 이전 unreal-specialist 리뷰가 "미확인"으로 플래그했던 부분에 대한 확답.
4. `sourceStaticMesh` 그래프 변수를 `SM_SpikeCube`로 오버라이드, `UniformFracture` 노드의
   `MinVoronoiSites=35` / `MaxVoronoiSites=45`로 설정 (반영 확인됨, `GetNodeInfo`로 재조회)
5. `enableNanite=true` 설정, 저장
6. 레벨에 액터로 스폰, 뷰포트 스크린샷 확인 — 균열/조각 안 보이고 원본 통짜 메시처럼 렌더됨
7. **핵심 발견 #2 (막힌 지점)**: 애셋 레지스트리 태그 확인 결과 `NumTransforms: 0` —
   그래프가 설정만 됐을 뿐 한 번도 평가(bake)된 적 없음. 화면의 "멀쩡한 바닥"은 실제 조각이 아니라
   `RootProxyMesh` 폴백(원본 미파괴 메시를 미리보기로 띄우는 것)이었음.
   에디터 재열기(`OpenEditorForAsset`), 재저장(`save_assets`) 모두 시도했으나 `NumTransforms`
   그대로 0 — 자동으로 안 구워짐.
8. `unreal-mcp`에 등록된 20여 개 툴셋 어디에도 Dataflow 그래프를 강제로 평가/빌드하는 함수 없음
   (`ProgrammaticToolset`도 등록된 툴 오케스트레이션만 허용, raw 엔진 API 접근 불가).
   Fracture/Dataflow 에디터 UI의 "Generate"/"Build" 버튼 클릭이 필요한 것으로 보임 —
   이건 수동 클릭 한 번이 남은 상태.

## Progress — bake 확인 (2026-07-16, 계속)

`GC_SpikeFloor` 에디터 탭에서 "Evaluate Dataflow Graph" 버튼(툴바 ▶) 클릭 직후엔 `NumTransforms: 0`
그대로였음 (에셋 태그 기준). 저장을 여러 번 다시 해봐도 변화 없어서 한동안 막힘.

**돌파구**: 해당 에디터 탭을 닫는 시점에 실제 bake가 트리거됨. Output Log 확인 결과:
```
[02.43.56] LogStaticMesh: NaniteBuild [0.05s]
[02.43.56] LogChaos: NewSimplicial: InitialSize: 2537, ImplicitExterior: 0, FullCopy: 1, FinalSize: 60
```
탭을 닫는 순간(`LogWorld: UWorld::CleanupWorld`, `LogSpawn: Warning: Destroying
/Engine/Transient.TempDataflowCacheCa...`) 임시 Dataflow 캐시가 실제 애셋으로 커밋되며 Nanite
빌드가 함께 수행된 것으로 보임. 레벨 뷰포트에서도 균열선이 선명하게 보이는 실제 Voronoi 파편
형태로 렌더 확인됨 (사용자 스크린샷).

재확인 결과:
- `NumTransforms: 38` (요청한 35~45 범위 내 — Voronoi 파괴 실제로 적용됨)
- `enableNanite: true`, `bEnableNaniteFallback: false` — Nanite로 강제 폴백 없이 유지

## Result

**Status: CONFIRMED — 핵심 질문(Nanite+GC 파괴 호환성) YES로 답 나옴**

- Nanite 유지 여부: **YES** — Nanite 메시 → Geometry Collection 변환 → Voronoi 파괴(38피스) →
  Nanite 강제 폴백 없이 유지됨 (`NaniteBuild` 로그로 실제 빌드 확인, `bEnableNaniteFallback: false`)
- 프레임타임 (ms, 최악의 순간): **미측정** — 이번 스파이크는 호환성 질문에 집중, 60fps 예산
  검증은 별도 후속 작업 (프로덕션 규모 피스 수 + 다수 발판 + PIE 물리 충격 테스트 필요)
- 에디터 경고/에러: 없음 (`r.MotionVectorSimulation`, 5.8 UI 학습데이터 갭 관련 시행착오는 진행상
  발생한 잡음이지 실패 원인 아님)
- 판정: **YES (호환성 확인됨) / 성능은 별도 검증 필요**
  - Dataflow 그래프의 자동 bake는 MCP 스크립팅만으로는 트리거 안 됨 — 에디터 탭을 닫아야
    커밋됨. 실제 프로덕션에서는 Fracture Mode UI로 직접 작업할 것이므로 이 우회 이슈는 무관.

## Next step depending on result

- **YES**: 프로덕션 순서 확정 진행 (`/map-systems`) — 아레나 몰핑도 표준 리스크로 하향
- **PARTIAL** (Nanite 깨지지만 non-Nanite GC로 대체 시 시각적으로 허용 가능): 아레나 바닥을 non-Nanite 전용 애셋으로 설계 — art-director/technical-artist와 조율
- **NO** (60fps 못 지킴 or Nanite 조합 자체가 깨짐): 피스 수 축소, 프리페처/클러스터링, 또는 "떠오르는 발판"을 별도 non-destructible 오브젝트로 분리하는 대안 설계 필요
