# Ollama 작업 위임 기준

이 문서는 Gemini가 "다음 작업"을 고를 때 반드시 따라야 하는 기준이다. `bot.py`가 이 파일을
그대로 읽어서 Gemini 프롬프트에 포함시킨다 — 여기 내용을 수정하면 코드를 안 건드리고도 판단
기준을 바꿀 수 있다.

Claude Code와 Antigravity CLI도 이 문서를 기준으로 삼는다: 작업 중 아래 "맡기기 좋은 작업"에
해당하는 하위 작업을 발견하면, 그 자리에서 직접 하지 말고 `production/overnight-queue.md`에
태스크로 등록한다 (`.claude/docs/ollama-delegation.md` 참조).

## 전체 구조 요약

- 디스코드 봇이 Github 레포를 읽음 (읽기 전용, git 쓰기 동작 없음)
- Gemini(무료 티어)가 그 내용을 보고 "로컬 LLM이 할 만한 간단한 작업" 하나를 골라 지시문 작성
- 로컬 Ollama(`safe_qwen`)가 그 지시문을 실행해서 텍스트 결과 생성
- 결과는 로컬 파일(.md) + 디스코드 첨부파일로 저장, 커밋/푸시 절대 안 함
- 실제 UE5.8 핵심 개발(로직, 아키텍처, 엔진 API, 디버깅)은 전부 Windows의 클로드 코드가 담당 —
  이 파이프라인은 클로드 코드의 토큰을 아끼기 위해 "간단하고 위험 없는 작업"만 대신 처리

## Ollama에게 맡기기 좋은 작업 (이런 것만 고를 것)

- 문서화: README, 주석, 함수/변수 설명 정리
- 반복적인 보일러플레이트: Getter/Setter, 단순 UPROPERTY/UFUNCTION 선언, 단순 데이터 구조체
- 커밋 메시지 다듬기, TODO/체크리스트 뽑아내기
- 로그나 에러 메시지를 사람이 읽기 쉽게 요약
- 이미 정해진 패턴을 그대로 따라 하는 짧은 유틸리티 스크립트(파이썬 등)
- 폴더/파일 정리 제안, 네이밍 컨벤션 통일 제안
- 텍스트 변환 (번역, 서식 변경, 표 정리)

## Ollama에게 맡기면 안 되는 작업 (이런 건 절대 고르지 말 것)

- 블루프린트/C++ 간 상호작용처럼 여러 요소가 얽힌 설계 판단
- 여러 파일에 걸쳐 일관성을 유지해야 하는 리팩터링
- 언리얼 엔진 API에 대한 정확한 지식이 필요한 디버깅
- 성능 최적화, 멀티스레딩, 네트워크 리플리케이션 등 실수하면 큰 문제가 되는 작업
- 검증 없이 그대로 커밋되면 위험한 최종 코드 작성

## Gemini(무료 티어) 사용 원칙

- 입력은 최소한으로 (코드 스니펫 5개, 파일당 500자 이내)
- 출력은 512 토큰 이내로 제한
- Gemini는 "무엇을 시킬지 고르고 짧은 지시문만 작성" — 실제 결과물 생성은 전부 로컬 Ollama가 담당
- 하루 여러 번 실행하면 무료 티어 한도(분당/일일 요청 수)에 걸릴 수 있으니 필요할 때만 호출

## 관련 문서

- `production/overnight-protocol.md` — 신뢰 모델(Ollama 결과는 항상 Draft), 아침 리뷰 루틴
- `production/overnight-queue.md` — 실제 태스크 큐 (Claude Code/Antigravity가 여기 등록)
- `tools/overnight-bot/discord_ollama_bot.py` — 실행 스크립트
- `.claude/docs/ollama-delegation.md` — Claude Code/Antigravity가 지켜야 하는 위임+커밋 정책

> **참고**: `discord_ollama_bot.py`는 2026-07-16 기준 이 문서가 설명하는 "Gemini가 레포를 보고
> 다음 작업을 직접 고르는" 구조가 아니라, 하드코딩된 태스크 4개를 순서대로 돌리고 Gemini는
> Ollama 결과를 3줄 요약하는 역할만 한다. 이 문서의 구조대로 봇을 다시 짜는 작업은 아직 별도
> 태스크로 남아있음 — 진행 시 `production/session-state/active.md`에 기록할 것.
