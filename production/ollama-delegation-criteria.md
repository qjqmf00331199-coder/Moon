# Ollama 작업 위임 기준

이 문서는 Claude Code/Antigravity CLI가 어떤 하위 작업을 Ollama 큐(`OLLAMA-INSTRUCTIONS.md`,
레포 루트)에 넣어도 되는지 판단하는 기준이다. Gemini는 이 문서도, 레포의 다른 어떤 파일도
읽지 않는다 — 아래 "전체 구조 요약" 참고.

Claude Code와 Antigravity CLI도 이 문서를 기준으로 삼는다: 작업 중 아래 "맡기기 좋은 작업"에
해당하는 하위 작업을 발견하면, 그 자리에서 직접 하지 말고 `OLLAMA-INSTRUCTIONS.md`에
태스크로 등록한다 (`.claude/docs/ollama-delegation.md` 참조).

## 전체 구조 요약

- Claude Code/Antigravity CLI가 작업 중 발견한 저위험 하위 작업을 `OLLAMA-INSTRUCTIONS.md`
  (레포 루트)에 태스크로 등록
- 디스코드 봇(`discord_ollama_bot.py`)이 그 큐 파일을 직접 파싱해서 `[ ]`(아직 안 돌린) 태스크를
  순서대로 로컬 Ollama(`qwen2.5-coder:7b`)에게 실행시킴 — Gemini는 레포를 읽지도, 태스크를 고르지도
  않음
- Gemini(무료 티어)는 Ollama가 만든 결과를 3줄 요약하는 용도로만 쓰임 — 원본 지시문이 아니라
  결과 텍스트만 보므로 토큰 사용이 태스크 개수·크기와 무관하게 작게 유지됨
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

## 컨텍스트 크기 예산 (2026-07-16 밤 사고 이후 추가)

`qwen2.5-coder:7b`는 로컬 7B 모델이라 컨텍스트 윈도우가 넉넉하지 않다. 2026-07-16 밤 실행에서
Task 1~6이 전부 무효 처리됨 — 원인은 `discord_ollama_bot.py`가 Ollama 호출 시 `num_ctx`를
지정하지 않아 기본값(수천 토큰)으로 돌아갔고, `player-movement.md`(7만자)처럼 큰 파일을
paste한 태스크는 앞부분 지시문이 잘려나가 모델이 "IDK" 같은 무의미한 답이나 큐 파일 서식을
그대로 반복하는 증상을 보였다. `num_ctx`/`num_predict`/`temperature` 옵션을 명시하도록
`discord_ollama_bot.py`를 고쳤지만(`OLLAMA_NUM_CTX` 환경변수, 기본 32768), 태스크를 등록할 때도
아래를 지킬 것:

- **Context files 총 글자 수를 확인**: 한 태스크에 paste되는 모든 컨텍스트 파일 합계가
  대략 15,000~20,000자를 넘으면(한국어는 토큰 밀도가 영어보다 높음) `OLLAMA_NUM_CTX` 기본값
  32768로도 못 담을 수 있다. 그보다 큰 문서는 전체를 붙이지 말고, 태스크 목적에 실제로 필요한
  절(section)만 발췌해서 `{{PLACEHOLDER}}`에 넣을 것 (발췌 자체는 태스크를 등록하는 Claude
  Code/Antigravity가 판단 — Ollama에게 "어디를 발췌할지" 판단을 맡기지 않는다).
- **결과가 이상하면 즉시 알 수 있어야 함**: 봇이 응답을 저장하기 전 `find_degenerate_reason()`으로
  걸러서 의심스러우면 Discord에 `⚠️`로 표시하고 Gemini 요약을 생략하도록 되어 있다. 아침 리뷰
  시 이 표시가 붙은 태스크는 review checklist를 돌릴 필요 없이 바로 폐기하고 재큐잉만 하면 된다.
- 태스크 재실행 전에 `tools/overnight-bot/discord_ollama_bot.py`의 `OLLAMA_NUM_CTX` 값과 실제
  Ollama 서버가 그 값을 받아들이는지(모델 로드 시 OOM 안 나는지) 한 번은 확인할 것.

## Gemini(무료 티어) 사용 원칙

- 입력은 최소한으로 (코드 스니펫 5개, 파일당 500자 이내)
- 출력은 512 토큰 이내로 제한
- Gemini는 "무엇을 시킬지 고르고 짧은 지시문만 작성" — 실제 결과물 생성은 전부 로컬 Ollama가 담당
- 하루 여러 번 실행하면 무료 티어 한도(분당/일일 요청 수)에 걸릴 수 있으니 필요할 때만 호출

## 관련 문서

- `production/overnight-protocol.md` — 신뢰 모델(Ollama 결과는 항상 Draft), 아침 리뷰 루틴
- `OLLAMA-INSTRUCTIONS.md` (레포 루트) — 실제 태스크 큐 (Claude Code/Antigravity가 여기 등록,
  봇이 직접 파싱)
- `tools/overnight-bot/discord_ollama_bot.py` — 실행 스크립트
- `.claude/docs/ollama-delegation.md` — Claude Code/Antigravity가 지켜야 하는 위임+커밋 정책
