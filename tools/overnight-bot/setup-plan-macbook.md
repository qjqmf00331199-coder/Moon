# 맥북 실행 계획서 — Overnight Discord Bot

이 맥북에서 실제로 실행할 경로 기준 체크리스트. 코드/README는 저장소
`tools/overnight-bot/`에 있지만, 실행은 홈 디렉토리(`/Users/t2025-m0206/`)에 별도로 복사해
돌리는 방식.

- [ ] **1. `python-dotenv` 설치**
  ```bash
  pip3 install python-dotenv
  ```

- [ ] **2. `.env` 파일 생성**
  ```bash
  nano /Users/t2025-m0206/.env
  ```
  아래 내용 붙여넣고 실제 값으로 교체 (따옴표 유지):
  ```env
  DISCORD_TOKEN="본인_디스코드_토큰"
  GEMINI_API_KEY="본인_제미나이_키"
  REPO_ROOT="/Users/t2025-m0206/moon-fragment-hunt"
  ```
  저장: `Control+O` → `Enter` → `Control+X`

  ⚠️ 이 `.env`는 저장소 밖(`/Users/t2025-m0206/.env`)에 있으므로 git 추적 대상이 아님 —
  안전하지만, 저장소 안 `.env`와 헷갈리지 않게 이 파일 하나만 쓸 것.

- [ ] **3. 봇 스크립트 배치**
  ```bash
  nano /Users/t2025-m0206/night_bot.py
  ```
  내용 전체를 `tools/overnight-bot/discord_ollama_bot.py`와 동일하게 붙여넣기
  (저장소 파일 그대로 복사 — 새로 타이핑하지 말고 그 파일 내용 복사/붙여넣기 권장).
  저장: `Control+O` → `Enter` → `Control+X`

  - [ ] `REPO_ROOT` 값이 `.env`의 `REPO_ROOT="/Users/t2025-m0206/moon-fragment-hunt"`와
        일치하는지 확인 — 이 경로 안에 `design/gdd/`, `prototypes/` 등이 실제로 있어야
        태스크가 파일을 정상적으로 읽는다.

- [ ] **4. 실행**
  ```bash
  python3 /Users/t2025-m0206/night_bot.py
  ```
  터미널에 `봇 로그인 완료: ...`가 뜨면 성공. 이 터미널 창은 봇이 도는 동안 계속 열어둘 것
  (종료하면 봇도 꺼짐).

- [ ] **5. Discord에서 테스트**
  - 봇이 초대된 채널에 `!야간시작` 입력 → 태스크 4개 순차 실행 확인
  - `!작업 <내용>` 입력 → 자유 작업 1건 실행 확인
  - 에러 메시지(`❌ ...`)가 뜨면 터미널 로그도 함께 확인해 전체 에러 문구 캡처해둘 것

- [ ] **6. 다음 날 아침**
  `production/overnight-output/`의 결과 파일들을 `production/ollama-instructions.md`의 리뷰
  체크리스트로 확인 → Claude Code/Antigravity CLI 세션에서 승격 심사 → 처리 끝난 파일은 삭제.

## 문제 생기면

터미널에 뜬 에러 메시지 전체(스택트레이스 포함)를 그대로 복사해서 전달할 것 — 요약하지 말고
원문 그대로. `tools/overnight-bot/README.md`의 "자주 나는 문제" 표에서 먼저 대조해봐도 됨.
