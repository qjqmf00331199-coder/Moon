# Overnight Discord Bot (Ollama runner) — 처음부터 설정하는 법

이 봇은 Discord 채팅으로 명령을 보내면, 로컬에서 돌아가는 Ollama(qwen2.5-coder:7b)에게
`production/ollama-instructions.md`의 태스크를 시키고, 결과를 Gemini로 3줄 요약해서 다시 Discord로
알려주는 스크립트다. 결과물은 전부 draft — `production/overnight-output/`에 저장될 뿐, 이 봇은
`design/gdd/` 등 실제 프로젝트 파일을 절대 직접 건드리지 않는다.

아래는 아무것도 설치 안 된 맥북 기준, 맨 처음부터 순서대로 하는 설정법이다.

---

## 0. 필요한 것 목록

- 이 저장소(moon-fragment-hunt)가 clone/sync되어 있는 맥
- Python 3.10 이상
- Discord 계정 + 봇을 등록할 서버(길드)에 대한 관리 권한
- Google 계정 (Gemini API 키 발급용)
- Ollama (로컬 LLM 실행기)

하나씩 확인한다.

---

## 1. 저장소 준비

이미 이 맥에 저장소가 있다면 최신 상태로 받아둔다:

```bash
cd ~/이 저장소를-clone한-경로/moon-fragment-hunt
git pull
```

아직 없다면 clone한다 (팀 저장소 주소로 바꿀 것):

```bash
git clone <저장소_URL> ~/moon-fragment-hunt
cd ~/moon-fragment-hunt
```

이 전체 경로(`~/moon-fragment-hunt`)를 나중에 `.env`의 `REPO_ROOT`에 넣는다. **매번 `!야간시작`
돌리기 전에 `git pull`부터 하는 습관을 들일 것** — Windows 쪽 세션(Claude Code/Antigravity CLI)이
만든 최신 GDD가 이 맥에 없으면 옛날 내용으로 팩트체크를 하게 된다.

---

## 2. Python 설치 확인

터미널에서:

```bash
python3 --version
```

`Python 3.10.x` 이상이 나오면 OK. 없으면:

```bash
brew install python3
```

(Homebrew가 없으면 먼저 `https://brew.sh` 안내대로 설치)

---

## 3. Discord 봇 만들기

이미 봇이 있고 토큰만 재발급하면 되는 경우 3-1로 바로 이동.

### 3-1. 애플리케이션/봇 생성 (처음 만드는 경우)

1. https://discord.com/developers/applications 접속, 로그인
2. **New Application** 클릭 → 이름 입력(예: `MoonNightWorker`) → Create
3. 왼쪽 메뉴 **Bot** 클릭
4. **Reset Token** (또는 최초 생성 시 **Token 보기**) 클릭 → 나오는 토큰을 복사해둠
   - ⚠️ 이 토큰은 한 번만 보여준다. 잃어버리면 다시 Reset Token 눌러 재발급.
   - ⚠️ 이 토큰을 절대 채팅/이슈/커밋에 붙여넣지 말 것. 코드에도 하드코딩 금지 — `.env`에만 넣음.
5. 같은 **Bot** 탭에서 **Message Content Intent** 토글을 켠다(ON). 이게 꺼져있으면 봇이 메시지
   내용을 못 읽어서 `!야간시작` 같은 명령어에 반응하지 않는다.

### 3-2. 서버에 봇 초대

1. 왼쪽 메뉴 **OAuth2 → URL Generator**
2. **SCOPES**에서 `bot` 체크
3. 아래 **BOT PERMISSIONS**에서 `Send Messages`, `Read Message History` 체크
4. 맨 아래 생성된 URL 복사 → 브라우저 새 탭에 붙여넣기 → 봇을 추가할 서버 선택 → 승인
5. 봇이 등록된 서버 채널 목록에 (오프라인 상태로) 나타나면 성공

### 3-3. 기존 토큰이 이미 노출된 경우

봇 토큰을 채팅/코드에 평문으로 올린 적 있다면, 3-1의 4번(Reset Token)을 지금 바로 실행해서
재발급할 것 — 옛 토큰은 그 순간 무효화된다.

---

## 4. Gemini API 키 발급

1. https://aistudio.google.com/apikey 접속, 구글 계정 로그인
2. **Create API key** 클릭 → 새 프로젝트 선택 또는 기존 프로젝트 선택
3. 생성된 키(보통 `AIza...`로 시작) 복사해둠
   - 예전에 다른 키를 채팅에 올린 적 있다면 그 키는 여기서 찾아 **삭제(Delete)** 하고 새로 발급

---

## 5. Ollama 설치 + 모델 준비

### 5-1. 설치

```bash
brew install ollama
```

또는 https://ollama.com/download 에서 맥용 설치 파일 받아 설치.

### 5-2. 서버 실행

```bash
ollama serve
```

터미널 하나를 이 명령 실행용으로 계속 띄워둔다(백그라운드로 켜두고 다른 터미널 탭 써도 됨).
brew로 설치한 경우 백그라운드 서비스로 이미 떠 있을 수도 있음 — 아래 6번으로 바로 확인해봐도 됨.

### 5-3. 모델 다운로드

새 터미널 탭에서:

```bash
ollama pull qwen2.5-coder:7b
```

다운로드 끝나면 확인:

```bash
ollama list
```

`qwen2.5-coder:7b`가 목록에 보이면 완료.

### 5-4. 동작 확인

```bash
curl http://localhost:11434/api/generate -d '{
  "model": "qwen2.5-coder:7b",
  "prompt": "안녕",
  "stream": false
}'
```

JSON 응답이 오면 Ollama 쪽은 준비 완료.

---

## 6. 이 봇 폴더 설정

```bash
cd ~/moon-fragment-hunt/tools/overnight-bot

# (권장) 가상환경 생성
python3 -m venv venv
source venv/bin/activate

# 패키지 설치
pip install -r requirements.txt
```

### 6-1. .env 파일 만들기

```bash
cp .env.example .env
```

`.env`를 열어서 (VS Code면 `code .env`, 기본 편집기면 `open -e .env`) 아래 값을 채운다:

```env
DISCORD_TOKEN=3-1에서_복사한_토큰
GEMINI_API_KEY=4번에서_복사한_키
OLLAMA_URL=http://localhost:11434/api/generate
OLLAMA_MODEL=qwen2.5-coder:7b
REPO_ROOT=/Users/본인계정/moon-fragment-hunt
```

`REPO_ROOT`는 1번에서 clone한 실제 절대경로로 정확히 맞출 것 (`pwd`로 확인 가능).

`.env`는 `.gitignore`에 이미 등록되어 있어 `git status`에 안 잡힌다 — 혹시라도 잡히면 절대
커밋하지 말고 먼저 문의할 것.

---

## 7. 실행

```bash
# tools/overnight-bot 폴더 안, 가상환경 켠 상태에서
python discord_ollama_bot.py
```

터미널에 아래처럼 뜨면 성공:

```
봇 로그인 완료: MoonNightWorker#1234
'!야간시작' = 큐 4개 태스크 순차 실행 / '!작업 <내용>' = 자유 작업 1건 실행
```

Discord에서 봇이 초대된 채널이 "온라인"으로 바뀐 걸 확인한다.

---

## 8. 사용법

Discord 채널에 메시지로:

- **`!야간시작`**
  `production/ollama-instructions.md`에 정의된 태스크 4개(스파이크 리버스도큐먼트, registry 팩트체크
  2건, 용어 통일성 리포트)를 순서대로 실행. 태스크마다 완료 메시지 + Gemini 3줄 요약을 채팅에
  보여주고, 전체 결과 파일은 `production/overnight-output/`에 저장됨.

- **`!작업 <내용>`**
  즉석에서 자유 텍스트로 작업 하나를 시킴. 예: `!작업 combat-hud.md 초안에 오타 있는지 확인해줘`
  이 경로는 큐에 미리 정의된 4개보다 검증이 느슨하니(사람이 그때그때 지시), 아침에 결과 리뷰를
  더 꼼꼼히 할 것.

봇은 창의적 판단(밸런스 수치, 공식, 설정 창작)을 하지 말라는 시스템 프롬프트가 항상 걸려있지만,
그래도 결과는 **전부 draft**로 취급한다 — 아래 9번 없이는 아무것도 실제 파일에 반영되지 않는다.

---

## 9. 다음 날 아침 리뷰 (필수)

1. `production/overnight-output/` 폴더 확인
2. 각 파일을 `production/ollama-instructions.md`에 적힌 "Review checklist" 기준으로 확인
   (대부분 원본 파일과 대조하는 것 — 2분 내로 끝남)
3. 쓸만한 내용은 Claude Code나 Antigravity CLI 세션에서 직접 다듬어서 실제 파일
   (`design/gdd/*`, `entities.yaml` 등)에 반영
4. 검토 끝난 파일은 `overnight-output/`에서 지워서 다음날 실행이 깨끗하게 시작되게 함

전체 신뢰 모델/원칙은 `production/overnight-protocol.md` 참고.

---

## 10. 자주 나는 문제

| 증상 | 원인 | 해결 |
|------|------|------|
| 봇이 로그인은 되는데 `!야간시작`에 반응 없음 | Message Content Intent가 꺼져있음 | 3-1의 5번 다시 확인 (Discord 개발자 포탈 Bot 탭) |
| `discord.errors.LoginFailure` | 토큰이 틀렸거나 재발급으로 무효화됨 | `.env`의 `DISCORD_TOKEN`을 최신 토큰으로 교체 |
| `ConnectionRefusedError` (Ollama 관련) | `ollama serve`가 안 떠 있음 | 터미널에서 `ollama serve` 실행 확인, 5-4번 curl로 재확인 |
| 결과 파일에 `[파일 없음: ...]`이 섞여 나옴 | `REPO_ROOT` 경로가 틀렸거나 해당 파일이 이 맥에 없음 | `REPO_ROOT` 재확인 + `git pull` |
| Gemini 요약이 "Gemini 에러: ..."로 나옴 | API 키 오류 또는 그날 무료 할당량 초과 | 키 재확인, https://aistudio.google.com 에서 사용량 확인 |
| `ModuleNotFoundError` | 가상환경 안 켜져 있거나 `pip install` 안 함 | `source venv/bin/activate` 후 `pip install -r requirements.txt` |

---

## 11. 태스크 추가하기

`production/ollama-instructions.md`에 새 태스크를 문서로 먼저 추가하고(사람이 읽는 스펙),
`discord_ollama_bot.py`의 `TASKS` 리스트에도 똑같이 추가해야 실제로 실행된다 — queue.md와
`TASKS`는 반드시 같이 관리할 것. `context_files`에는 `REPO_ROOT` 기준 상대경로를 적으면 실행 시
자동으로 읽어서 프롬프트의 해당 placeholder(`{{CONTEXT}}` 등)에 주입된다.

새 태스크가 `overnight-protocol.md`의 3원칙(자기완결형 / 잘못돼도 위험 낮음 / 2분 내 검증 가능)에
맞는지 먼저 확인 — 안 맞으면 큐에 넣지 말고 사람이 직접 하거나 Claude/Antigravity에 맡길 것.
