# Plan: Enemy BP 트리 구축

## 목표
BP_Enemy_Base 기반으로 하위 적 BP + 플레이어 확장 + 맵 배치까지 완성.

## 체크리스트

- [x] BP_Enemy_Base 생성
- [ ] BP_Spider (BP_Enemy_Base 상속) 생성
- [ ] BP_SpiderKing (BP_Enemy_Base 또는 BP_Spider 상속) 생성
- [ ] 플레이어 BP 확장 (적 상호작용 대응)
- [ ] 맵에 배치 및 PIE 테스트

## 세션 분리 규칙
- 항목 1개 = 세션 1개. 새 세션 시작 시 이 파일부터 읽고 이어감.
- 완료 항목은 `[x]`로 체크 + 완료 시각/커밋 해시 한 줄 메모.
- unreal-mcp 호출은 batch 가능하면 batch로, describe_toolset 반복 조회 금지.
- compile/PIE 로그는 요약만 여기 남기고 원문은 별도 파일.

## 진행 로그
(세션마다 한 줄씩 추가)
- 
