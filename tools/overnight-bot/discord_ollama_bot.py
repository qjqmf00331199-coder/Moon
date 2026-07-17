import discord
import requests
import google.generativeai as genai
import os
import re
from datetime import datetime
from dotenv import load_dotenv

# ============ 설정 (실제 값은 .env 파일에 — 이 파일에는 절대 하드코딩하지 말 것) ============
load_dotenv()

DISCORD_TOKEN = os.environ["DISCORD_TOKEN"]
GEMINI_API_KEY = os.environ["GEMINI_API_KEY"]
OLLAMA_URL = os.environ.get("OLLAMA_URL", "http://localhost:11434/api/generate")
OLLAMA_MODEL = os.environ.get("OLLAMA_MODEL", "qwen2.5-coder:7b")
# num_ctx 미지정 시 Ollama가 모델 기본값(보통 2048~4096)을 씀 — 이 큐의 태스크는 GDD/registry
# 파일 전체를 프롬프트에 파스팅하므로 (player-movement.md 하나만 7만자) 기본값으로는 앞부분
# 지시문이 잘려나가 모델이 "IDK"나 프롬프트 구조 자체를 그대로 반복하는 증상이 나온다
# (2026-07-16 밤 실행에서 Task 1~6 전부 이 증상으로 무효 처리됨). qwen2.5-coder:7b는 32k 네이티브
# 컨텍스트를 지원하니 기본을 32768로 올려둔다 — VRAM 부족하면 OLLAMA_NUM_CTX 환경변수로 낮출 것.
OLLAMA_NUM_CTX = int(os.environ.get("OLLAMA_NUM_CTX", "32768"))
OLLAMA_NUM_PREDICT = int(os.environ.get("OLLAMA_NUM_PREDICT", "2048"))

# 리포지토리 루트 — 이 맥에서 moon-fragment-hunt를 clone/sync 해둔 실제 경로로 바꿀 것
REPO_ROOT = os.environ.get("REPO_ROOT", "/Users/t2025-m0206/moon-fragment-hunt")
PRODUCTION_DIR = f"{REPO_ROOT}/production"
OUTPUT_DIR = f"{PRODUCTION_DIR}/overnight-output"
QUEUE_PATH = f"{REPO_ROOT}/OLLAMA-INSTRUCTIONS.md"
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

# 태스크는 하드코딩하지 않는다 — repo 루트의 OLLAMA-INSTRUCTIONS.md 큐 파일을 직접 파싱해서 얻는다.
# 큐 파일 하나가 사람이 읽는 스펙이자 봇이 실행하는 소스, 두 곳을 따로 관리할 필요 없음.
# 큐 파일의 "Format contract" 절이 이 파서가 기대하는 정확한 구조를 정의한다.
TASK_BLOCK_RE = re.compile(r"^## \[( |x|~)\] Task \d+ — (.+)$", re.MULTILINE)
CONTEXT_FILE_RE = re.compile(r"^- (\{\{[A-Z_]+\}\}): (.+)$", re.MULTILINE)
PROMPT_RE = re.compile(r"\*\*Prompt\*\*:\s*```\n(.*?)\n```", re.DOTALL)
OUTPUT_PATH_RE = re.compile(r"\*\*Output path\*\*:\s*`([^`]+)`")


def parse_task_queue(queue_path):
    """OLLAMA-INSTRUCTIONS.md를 읽어 `[ ]`(아직 안 돌린) 태스크를 (tasks, skipped_titles)로 반환.
    형식이 안 맞는 태스크 블록은 죽지 않고 건너뛰지만, 제목은 skipped_titles에 담아 호출자가
    경고할 수 있게 한다 — 사람이 큐에 뭔가 넣었다고 믿는데 조용히 안 돌아가는 상황 방지."""
    with open(queue_path, "r", encoding="utf-8") as f:
        text = f.read()

    headings = list(TASK_BLOCK_RE.finditer(text))
    tasks = []
    skipped_titles = []
    for i, m in enumerate(headings):
        if m.group(1) != " ":
            continue  # [x] 완료됐거나 [~] 폐기된 태스크는 스킵

        block_start = m.end()
        block_end = headings[i + 1].start() if i + 1 < len(headings) else len(text)
        block = text[block_start:block_end]

        context_files = dict(CONTEXT_FILE_RE.findall(block))
        prompt_match = PROMPT_RE.search(block)
        output_match = OUTPUT_PATH_RE.search(block)

        if not context_files or not prompt_match or not output_match:
            skipped_titles.append(m.group(2).strip())  # 형식이 깨진 블록 — 실행하지 않고 건너뜀
            continue

        output_path = output_match.group(1)
        task_id = os.path.splitext(os.path.basename(output_path))[0]

        tasks.append({
            "id": task_id,
            "title": m.group(2).strip(),
            "context_files": context_files,
            "prompt": prompt_match.group(1),
        })
    return tasks, skipped_titles


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


OLLAMA_TIMEOUT = int(os.environ.get("OLLAMA_TIMEOUT", "7200"))


def run_ollama(prompt):
    # 원래 timeout=300(5분)이었으나 Task 2/4처럼 player-movement.md(7만자) 전체를 붙이는
    # 태스크는 num_ctx를 32768로 늘려도 5분을 넘겨 응답 전에 끊긴다(2026-07-17, 실측 381초).
    # 이 봇은 밤새 무인으로 돌아 아무도 결과를 기다리지 않으므로, 로컬 Ollama를 쓰는 취지대로
    # 오래 걸리더라도 끝까지 생각하게 두는 쪽이 맞다 — 잘라서 빨리 받는 것보다 우선.
    res = requests.post(
        OLLAMA_URL,
        json={
            "model": OLLAMA_MODEL,
            "prompt": prompt,
            "stream": False,
            "options": {
                "num_ctx": OLLAMA_NUM_CTX,
                "num_predict": OLLAMA_NUM_PREDICT,
                "temperature": 0.1,
            },
        },
        timeout=OLLAMA_TIMEOUT,
    )
    res.raise_for_status()
    return res.json().get("response", "생성 실패")


# 컨텍스트 잘림/모델 혼란으로 나오는 전형적인 증상: 지시문 대신 프롬프트 구조를 그대로 반복하거나
# ("**Prompt**:", "**Context files**:", "**Output path**:" 같은 큐 파일 자체의 서식 마커가 응답에
# 나타남) 답 대신 "IDK" 류의 무의미한 단답만 나오는 경우. 둘 다 2026-07-16 밤 실행에서 실제로 발생.
DEGENERATE_MARKERS = ("**context files**", "**prompt**:", "**output path**:", "**why queued**")


def find_degenerate_reason(result):
    """결과가 유효한 답변이 아니라 잘림/혼란 증상으로 보이면 이유 문자열을, 정상이면 None을 반환."""
    stripped = result.strip()
    if len(stripped) < 30:
        return f"응답이 지나치게 짧음 ({len(stripped)}자) — 컨텍스트 잘림 의심"
    lowered = stripped.lower()
    for marker in DEGENERATE_MARKERS:
        if marker in lowered:
            return f"큐 파일 서식 마커('{marker}')를 그대로 반복함 — 지시문이 안 보이고 있었을 가능성"
    return None


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
    print("'!야간시작' = OLLAMA-INSTRUCTIONS.md의 큐 태스크 순차 실행 / '!작업 <내용>' = 자유 작업 1건 실행")


@client.event
async def on_message(message):
    if message.author == client.user:
        return

    if message.content.strip() == "!야간시작":
        os.makedirs(OUTPUT_DIR, exist_ok=True)

        try:
            tasks, skipped_titles = parse_task_queue(QUEUE_PATH)
        except FileNotFoundError:
            await message.channel.send(f"❌ 큐 파일 없음: `{QUEUE_PATH}`")
            return

        if skipped_titles:
            await message.channel.send(
                "⚠️ 형식이 안 맞아 스킵된 태스크: " + ", ".join(f"`{t}`" for t in skipped_titles)
            )

        if not tasks:
            await message.channel.send("ℹ️ 큐에 실행할 `[ ]` 태스크가 없음 — 전부 완료/폐기됐거나 큐가 비어있음.")
            return

        await message.channel.send(f"⏳ **큐 태스크 {len(tasks)}개 순차 실행 시작**")

        saved_files = []
        for task in tasks:
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

            degenerate_reason = find_degenerate_reason(result)
            if degenerate_reason:
                await message.channel.send(
                    f"⚠️ `{task['id']}` 응답이 의심스러움 ({degenerate_reason}) → `{output_path}`\n"
                    "Gemini 요약 생략 — 아침 리뷰에서 최우선으로 열어볼 것. 원인이 반복되면 "
                    "OLLAMA_NUM_CTX를 올리거나 해당 태스크의 컨텍스트 파일 크기를 줄일 것."
                )
                continue

            summary = gemini_summary(result)
            await message.channel.send(
                f"✅ `{task['id']}` 완료 → `{output_path}`\n**[요약]** {summary}"
            )

        await message.channel.send(
            f"🌅 **큐 실행 종료 ({len(saved_files)}/{len(tasks)} 성공)**. "
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

        degenerate_reason = find_degenerate_reason(result)
        if degenerate_reason:
            await message.channel.send(
                f"⚠️ 응답이 의심스러움 ({degenerate_reason}) → `{output_path}`\n"
                "Gemini 요약 생략 — 그대로 버리거나 프롬프트를 줄여서 다시 시도할 것."
            )
            return

        summary = gemini_summary(result)
        await message.channel.send(
            f"✅ 완료 → `{output_path}`\n**[요약]** {summary}\n\n"
            "🌅 이것도 draft임 — Claude/Antigravity 승격 심사 필요."
        )


client.run(DISCORD_TOKEN)
