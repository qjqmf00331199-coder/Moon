# 시그니처 전투 체인 — 셀프 플레이테스트 (Provisional)

> **Status**: Scenario locked, not yet executed
> **Date**: 2026-07-23
> **Related**: `prototypes/signature-combat-chain-spike-2026-07-21/SPIKE-NOTE.md` — "Next validation" 항목의 1차 약식 확인

## 목적

SPIKE-NOTE.md의 Next validation 항목(60–90초 무안내 완주 + Overdrive 종료 인지)을 1차로 확인한다.
**이 테스트는 최종 판정이 아니다** — 실행자가 로직을 직접 만든 본인이라 "무안내"가 온전히 성립하지
않는다. 결과는 Provisional PASS/FAIL로만 기록하며, 버티컬 슬라이스 승격 근거로 사용하지 않는다.

## 편향 완화책

- 이 로직을 마지막으로 만진 날로부터 최소 3일 경과 후 실행
- 실행 직전 코드/SPIKE-NOTE/관련 GDD 재열람 금지 (기억 리허설 방지)
- 컨트롤 키 목록(키만, 로직/인과관계 설명 없이)은 확인 가능 — 본인이 이미 알고 있어 통제 효과가
  없으므로 이 항목만 예외로 허용

## 빌드/맵

`/Game/Moon/Maps/L_CombatTest` (스파이크 빌드 그대로, 신규 구현 없음)

## 진행 절차

1. 3분 타이머 시작과 동시에 플레이 시작
2. 다음 이벤트의 타임스탬프 기록:
   - Blackhole 캐스트
   - Fire 캐스트
   - 기둥 파괴 (`MoonFracturePillar.bFractured`)
   - F 추출 (`SignatureChainState = Extracted`)
   - Overdrive 진입 시각 / 종료 시각 (발생한 경우)
3. 3분 내 미완주 시 FAIL 기록, 어디서 막혔는지 원인 메모

## Pass 조건 (모두 충족해야 Provisional PASS)

1. **완주 시간**: 60–90초 내 체인 1회 완주 (`Idle → Gathered → Executable → Extracted`)
2. **처형 근거 가시성**: 완주 직후 "타겟이 왜 추출 가능해졌는가"를 낯선 플레이어 시점으로 설명 시도.
   현재 HUD/이펙트만으로 그 이유가 화면에 드러나는지 자가 판정 (드러남 / 안 드러남 이분 기록)
3. **Overdrive 종료 인지**: Overdrive를 겪었다면 종료 후 "왜 자원 압박이 다시 평소처럼 돌아왔는가"를
   같은 방식으로 자가 판정. Overdrive가 발생하지 않았다면 이 항목은 N/A 처리하고 나머지 2개로만 판정.

## 결과 기록

(테스트 실행 후 아래 채움)

| 항목 | 값 |
|---|---|
| 실행일 | — |
| 마지막 로직 수정일과의 경과일 | — |
| 완주 시간 | — |
| 타임스탬프 로그 | — |
| Pass 조건 1 | — |
| Pass 조건 2 (자가 판정 근거) | — |
| Pass 조건 3 (자가 판정 근거, 또는 N/A) | — |
| **Provisional 판정** | — |

## 제외 범위 (검증 안 함)

재미/몰입도, 적 압박 하 가독성, 밸런스, 최종 파괴 연출 품질 — SPIKE-NOTE.md에 이미 비검증 항목으로 명시됨.

## 후속 필수 항목

이 셀프테스트가 Provisional PASS를 받아도 최종 게이트 통과로 취급하지 않는다. 외부 콜드 플레이어
3~5명 대상 정식 테스트(원래 시나리오: 무안내, 침묵 관찰, 고정 사후질문 2개)를 별도 task로 대기열에
남기고, 참가자 확보 즉시 실행한다.
