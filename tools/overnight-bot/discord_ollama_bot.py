import discord
import requests
import google.generativeai as genai
import os
from datetime import datetime
from dotenv import load_dotenv

# ============ 설정 (실제 값은 .env 파일에 — 이 파일에는 절대 하드코딩하지 말 것) ============
load_dotenv()

DISCORD_TOKEN = os.environ["DISCORD_TOKEN"]
GEMINI_API_KEY = os.environ["GEMINI_API_KEY"]
OLLAMA_URL = os.environ.get("OLLAMA_URL", "http://localhost:11434/api/generate")
OLLAMA_MODEL = os.environ.get("OLLAMA_MODEL", "qwen2.5-coder:7b")

# 리포지토리 루트 — 이 맥에서 moon-fragment-hunt를 clone/sync 해둔 실제 경로로 바꿀 것
REPO_ROOT = os.environ.get("REPO_ROOT", "/Users/t2025-m0206/moon-fragment-hunt")
PRODUCTION_DIR = f"{REPO_ROOT}/production"
OUTPUT_DIR = f"{PRODUCTION_DIR}/overnight-output"
# ==============================================================================

genai.configure(api_key=GEMINI_API_KEY)
gemini_model = genai.GenerativeModel(
    model_name="gemini-1.5-flash-latest",
    generation_config={"max_output_tokens": 200, "temperature": 0.1},
    system_instruction="인사말 생략. 작업 내용 3줄 이내 단답 요약.",
)

STRICT_SYSTEM_PREFIX = (
    "너는 설정 교차 검증 및 문서 요약기야. 창의적 판단, 밸런스 조정, 설정 창작은 절대 금지. "
    "오직 주어진 팩트체크/요약/간극(gap) 분석만 수행해. 모르면 '소스에 명시 안 됨'이라고 써. "
    "모든 결과는 Draft(초안)로 간주함.\n\n"
)

# production/ollama-instructions.md의 태스크 정의를 그대로 옮긴 것.
# 새 태스크를 큐에 추가하면 여기도 같이 추가해야 실제로 돌아감 — queue.md는 사람이 읽는 스펙,
# 이 리스트가 봇이 실제로 실행하는 소스.
TASKS = [
    {
        "id": "task1-arena-morphing-concept",
        "context_files": {
            "{{CONTEXT}}": "prototypes/arena-morphing-spike-2026-07-16/SPIKE-NOTE.md",
        },
        "prompt": """다음 스파이크 노트를 읽고 아래 구조로 요약해. 노트에 없는 내용은 절대 지어내지 말고,
불명확하면 "소스에 명시 안 됨"이라고 써.

# Arena Morphing — Nanite/Geometry Collection Spike

## What was tested
## Result
## Method
## Gotchas found
## Still open / follow-up needed

---
SOURCE SPIKE NOTE:
{{CONTEXT}}""",
    },
    {
        "id": "task2-registry-check-player-movement",
        "context_files": {
            "{{CONTEXT_REGISTRY}}": "design/registry/entities.yaml",
            "{{CONTEXT_GDD}}": "design/gdd/player-movement.md",
        },
        "prompt": """두 문서에서 같은 이름의 수치/상수/공식이 서로 다른 값으로 적혀있는 곳만 찾아서
"- [이름]: registry는 [X], GDD는 [Y] (GDD 섹션: [섹션명])" 형식으로 나열해.
불일치가 없으면 없다고 명시해. 다른 건(스타일, 누락 등) 언급하지 말 것.

---
REGISTRY (entities.yaml):
{{CONTEXT_REGISTRY}}

---
GDD (player-movement.md):
{{CONTEXT_GDD}}""",
    },
    {
        "id": "task3-registry-check-health-damage-core",
        "context_files": {
            "{{CONTEXT_REGISTRY}}": "design/registry/entities.yaml",
            "{{CONTEXT_GDD}}": "design/gdd/health-damage-core.md",
        },
        "prompt": """두 문서에서 같은 이름의 수치/상수/공식이 서로 다른 값으로 적혀있는 곳만 찾아서
"- [이름]: registry는 [X], GDD는 [Y] (GDD 섹션: [섹션명])" 형식으로 나열해.
불일치가 없으면 없다고 명시해. 다른 건(스타일, 누락 등) 언급하지 말 것.

---
REGISTRY (entities.yaml):
{{CONTEXT_REGISTRY}}

---
GDD (health-damage-core.md):
{{CONTEXT_GDD}}""",
    },
    {
        "id": "task4-terminology-consistency",
        "context_files": {
            "{{CONTEXT_A}}": "design/gdd/player-movement.md",
            "{{CONTEXT_B}}": "design/gdd/health-damage-core.md",
        },
        "prompt": """두 한국어 게임 기획서에서 같은 개념을 서로 다른 용어로 부르는 경우만 찾아서
"- 개념: [무엇] — A는 \\"[용어]\\", B는 \\"[용어]\\"" 형식으로 나열해.
한쪽에만 등장하는 용어는 불일치가 아니니 무시. 문법/번역 품질은 언급하지 말 것.

---
DOCUMENT A (player-movement.md):
{{CONTEXT_A}}

---
DOCUMENT B (health-damage-core.md):
{{CONTEXT_B}}""",
    },
]


def build_prompt(task):
    prompt = task["prompt"]
    for placeholder, rel_path in task["context_files"].items():
        full_path = os.path.join(REPO_ROOT, rel_path)
        try:
            with open(full_path, "r", encoding="utf-8") as f:
                content = f.read()
        except FileNotFoundError:
            content = f"[파일 없음: {rel_path} — 이 태스크는 스킵됨]"
        prompt = prompt.replace(placeholder, content)
    return STRICT_SYSTEM_PREFIX + prompt


def run_ollama(prompt):
    res = requests.post(
        OLLAMA_URL,
        json={"model": OLLAMA_MODEL, "prompt": prompt, "stream": False},
        timeout=300,
    )
    res.raise_for_status()
    return res.json().get("response", "생성 실패")


def gemini_summary(text):
    try:
        return gemini_model.generate_content(f"다음 리포트 내용 3줄 요약.\n{text}").text
    except Exception as e:
        return f"Gemini 에러: {e}"


intents = discord.Intents.default()
intents.message_content = True
client = discord.Client(intents=intents)


@client.event
async def on_ready():
    print(f"봇 로그인 완료: {client.user}")
    print("'!야간시작' = 큐 4개 태스크 순차 실행 / '!작업 <내용>' = 자유 작업 1건 실행")


@client.event
async def on_message(message):
    if message.author == client.user:
        return

    if message.content.strip() == "!야간시작":
        os.makedirs(OUTPUT_DIR, exist_ok=True)
        await message.channel.send(f"⏳ **큐 태스크 {len(TASKS)}개 순차 실행 시작**")

        saved_files = []
        for task in TASKS:
            await message.channel.send(f"▶ `{task['id']}` 실행 중...")
            prompt = build_prompt(task)
            try:
                result = run_ollama(prompt)
            except Exception as e:
                await message.channel.send(f"❌ `{task['id']}` Ollama 에러: {e}")
                continue

            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            output_path = f"{OUTPUT_DIR}/{task['id']}_{timestamp}.md"
            with open(output_path, "w", encoding="utf-8") as f:
                f.write(result)
            saved_files.append(output_path)

            summary = gemini_summary(result)
            await message.channel.send(
                f"✅ `{task['id']}` 완료 → `{output_path}`\n**[요약]** {summary}"
            )

        await message.channel.send(
            f"🌅 **큐 실행 종료 ({len(saved_files)}/{len(TASKS)} 성공)**. "
            "아침에 Claude/Antigravity로 전체 승격 심사하세요."
        )

    elif message.content.startswith("!작업 "):
        free_text = message.content[len("!작업 "):].strip()
        if not free_text:
            await message.channel.send("사용법: `!작업 <내용>` (예: !작업 combat-hud.md 초안 검토)")
            return

        os.makedirs(OUTPUT_DIR, exist_ok=True)
        await message.channel.send("⏳ **자유 작업 실행 중...**")

        prompt = STRICT_SYSTEM_PREFIX + free_text
        try:
            result = run_ollama(prompt)
        except Exception as e:
            await message.channel.send(f"❌ Ollama 에러: {e}")
            return

        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        output_path = f"{OUTPUT_DIR}/adhoc_{timestamp}.md"
        with open(output_path, "w", encoding="utf-8") as f:
            f.write(result)

        summary = gemini_summary(result)
        await message.channel.send(
            f"✅ 완료 → `{output_path}`\n**[요약]** {summary}\n\n"
            "🌅 이것도 draft임 — Claude/Antigravity 승격 심사 필요."
        )


client.run(DISCORD_TOKEN)
