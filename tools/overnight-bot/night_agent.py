"""저장소를 스스로 탐색하며 다단계로 작업하는 로컬 Ollama 에이전트 루프.

night_bot(디스코드 봇)의 `!개발` 명령이 이 모듈을 사용한다. 기존 큐(`!야간시작`)는 사람이
미리 붙여준 컨텍스트만 보고 1회 응답하는 구조라 모델이 스스로 뭔가를 확인할 수 없었다.
이 루프는 모델에게 LIST/READ 두 가지 **읽기 전용** 행동을 주고,
계획 → 파일 확인 → 초안 작성 → 자체 검토 순서로 스스로 생각하게 한다.
쓰기 도구는 일부러 주지 않는다 — 결과는 언제나 사람이 검토할 초안(draft) 텍스트다.
"""
import os
import re

import requests

# 파일 하나를 통째로 붙이면 컨텍스트가 금방 바닥나므로 (2026-07-16 잘림 사고 참고)
# READ 결과는 이 길이에서 자른다. 더 보고 싶으면 모델이 다른 파일/절을 골라 읽으면 된다.
MAX_FILE_CHARS = 24000

THINK_RE = re.compile(r"<think>.*?</think>", re.DOTALL)
ACTION_RE = re.compile(r"ACTION:\s*(READ|LIST)\s+(.+)")

AGENT_SYSTEM = (
    "너는 게임 개발 프로젝트 저장소를 직접 탐색하면서 스스로 계획을 세우고 문제를 해결하는 "
    "개발 보조 AI야. 모든 답변은 반드시 한국어로만 작성해.\n\n"
    "저장소를 확인해야 할 때는 아래 두 가지 행동만 쓸 수 있어 (한 번의 응답에 행동 하나만):\n"
    "ACTION: LIST <상대경로>   ← 폴더 내용 보기 (저장소 루트는 .)\n"
    "ACTION: READ <상대경로>   ← 파일 내용 읽기\n\n"
    "충분히 파악했으면 응답을 'FINAL:'로 시작하고 그 아래에 최종 결과물을 작성해.\n"
    "규칙:\n"
    "- 추측 금지: 파일 내용이 필요하면 반드시 READ로 직접 확인할 것\n"
    "- 근거 없는 내용은 지어내지 말고 '소스에 명시 안 됨'이라고 쓸 것\n"
    "- 최대 {max_steps}번의 ACTION 이후에는 반드시 FINAL을 낼 것\n"
    "- 이 결과는 사람이 검토하기 전의 초안(draft)이다"
)

REVIEW_SYSTEM = "너는 꼼꼼한 검토자야. 모든 답변은 반드시 한국어로만 작성해."

REVIEW_PROMPT = (
    "아래는 작업 요청과 그 초안이야. 초안을 비판적으로 검토해서 "
    "(1) 요청에서 빠뜨린 부분, (2) 근거 없이 지어낸 부분, (3) 앞뒤가 안 맞는 부분을 "
    "고친 **최종본만** 출력해. 검토 과정이나 설명은 쓰지 말고 고쳐진 결과물만 써.\n\n"
    "--- 작업 요청 ---\n{request}\n\n--- 초안 ---\n{draft}"
)


def strip_think(text):
    """qwen3 계열이 붙이는 <think>...</think> 사고 블록을 제거한 표시용 텍스트를 반환."""
    return THINK_RE.sub("", text).strip()


def _safe_path(repo_root, rel_path):
    """저장소 루트 밖으로 나가는 경로(../ 등)는 거부한다 — 에이전트는 저장소만 볼 수 있다."""
    rel_path = rel_path.strip().strip('`"\'')
    full = os.path.realpath(os.path.join(repo_root, rel_path))
    root = os.path.realpath(repo_root)
    if full != root and not full.startswith(root + os.sep):
        return None
    return full


def _tool_list(repo_root, rel_path):
    full = _safe_path(repo_root, rel_path)
    if not full or not os.path.isdir(full):
        return f"[디렉토리 없음: {rel_path}]"
    entries = []
    for name in sorted(os.listdir(full)):
        if name.startswith(".git"):
            continue
        marker = "/" if os.path.isdir(os.path.join(full, name)) else ""
        entries.append(name + marker)
    return "\n".join(entries) or "[빈 디렉토리]"


def _tool_read(repo_root, rel_path):
    full = _safe_path(repo_root, rel_path)
    if not full or not os.path.isfile(full):
        return f"[파일 없음: {rel_path}]"
    try:
        with open(full, encoding="utf-8", errors="ignore") as f:
            content = f.read()
    except Exception as e:
        return f"[읽기 실패: {e}]"
    if len(content) > MAX_FILE_CHARS:
        content = content[:MAX_FILE_CHARS] + (
            f"\n...[잘림: 총 {len(content)}자 중 앞 {MAX_FILE_CHARS}자만 표시. "
            "나머지가 필요하면 필요한 절이 있는 다른 파일을 찾아볼 것]"
        )
    return content


def _chat(messages, ollama_url, model, num_ctx, num_predict, timeout):
    """/api/generate 대신 /api/chat을 쓴다 — 멀티턴 탐색 루프라 대화 형식이 필요하다."""
    chat_url = ollama_url.replace("/api/generate", "/api/chat")
    res = requests.post(
        chat_url,
        json={
            "model": model,
            "messages": messages,
            "stream": False,
            "options": {
                "num_ctx": num_ctx,
                "num_predict": num_predict,
                "temperature": 0.2,
            },
        },
        timeout=timeout,
    )
    res.raise_for_status()
    msg = res.json().get("message", {})
    # 최신 Ollama는 사고 과정을 content 안 <think> 태그 또는 별도 thinking 필드로 준다 — 둘 다 수용.
    return msg.get("content", ""), msg.get("thinking", "")


def run_agent(request, repo_root, ollama_url, model, num_ctx, num_predict, timeout,
              max_steps=8, progress=None):
    """요청 하나를 계획→탐색→초안→자체검토로 수행하고 dict(final/draft/trail)를 반환한다."""

    def notify(text):
        if progress:
            try:
                progress(text)
            except Exception:
                pass  # 진행 알림 실패가 작업 자체를 죽여선 안 됨

    messages = [
        {"role": "system", "content": AGENT_SYSTEM.format(max_steps=max_steps)},
        {"role": "user", "content": (
            f"작업 요청: {request}\n\n"
            "저장소 루트(.)에서 시작해. 먼저 무엇을 확인해야 할지 계획을 세우고, "
            "필요한 파일만 골라 읽은 뒤 결과를 작성해."
        )},
    ]
    trail = []
    final = None

    for step in range(1, max_steps + 1):
        content, _ = _chat(messages, ollama_url, model, num_ctx, num_predict, timeout)
        visible = strip_think(content)
        messages.append({"role": "assistant", "content": visible})

        f_idx = visible.find("FINAL:")
        m = ACTION_RE.search(visible)

        if f_idx != -1 and (not m or f_idx < m.start()):
            final = visible[f_idx + len("FINAL:"):].strip()
            break

        if m:
            kind, arg = m.group(1), m.group(2).strip().strip('`"\'')
            result = _tool_list(repo_root, arg) if kind == "LIST" else _tool_read(repo_root, arg)
            trail.append(f"{kind} {arg}")
            notify(f"🔎 step {step}/{max_steps}: {kind} `{arg}`")
            messages.append({"role": "user", "content": (
                f"[{kind} {arg} 결과]\n{result}\n\n"
                f"계속해. 남은 ACTION 기회: {max_steps - step}번."
            )})
            continue

        final = visible  # 형식을 벗어난 응답 — 그대로 초안으로 취급
        break

    if final is None:
        # ACTION만 반복하다 스텝을 소진 — 지금까지 확인한 내용으로 최종 초안을 강제로 받는다
        messages.append({"role": "user", "content": (
            "ACTION 기회를 모두 썼어. 지금까지 확인한 내용만으로 'FINAL:' 결과를 작성해."
        )})
        content, _ = _chat(messages, ollama_url, model, num_ctx, num_predict, timeout)
        final = strip_think(content).replace("FINAL:", "", 1).strip()

    notify("🧐 초안 자체 검토 중...")
    reviewed, _ = _chat(
        [
            {"role": "system", "content": REVIEW_SYSTEM},
            {"role": "user", "content": REVIEW_PROMPT.format(request=request, draft=final)},
        ],
        ollama_url, model, num_ctx, num_predict, timeout,
    )
    reviewed = strip_think(reviewed)

    return {"final": reviewed or final, "draft": final, "trail": trail}
