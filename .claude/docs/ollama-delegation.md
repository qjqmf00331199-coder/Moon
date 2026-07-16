# Ollama Delegation & Auto-Commit Policy

Applies to **both** Claude Code and Antigravity CLI (mirrored in `AGENTS.md` — Antigravity has
no `@`-import mechanism, so its copy is written out in full rather than referenced).

## Delegate simple work to Ollama — don't do it yourself

Before doing any sub-task (not just the main task the user asked for — any incidental cleanup,
doc-writing, or boilerplate you notice along the way), check it against
`production/ollama-instructions.md`. If it matches the "맡기기 좋은 작업" list
(documentation, comments, boilerplate getters/setters, simple UPROPERTY/UFUNCTION declarations,
commit-message polish, TODO extraction, log/error summarization, small deterministic utility
scripts, folder/naming cleanup, text formatting/translation) — **do not do it yourself**, even
partially. Instead:

1. Add a new task entry to `production/overnight-queue.md`, following the existing task template
   (Why queued / Risk / Context to inject / Prompt / Output path / Review checklist).
2. Leave the actual work undone — Ollama does it overnight, a human or the next session reviews
   the draft in `production/overnight-output/` before it's promoted into any canonical file.
3. Commit and push (see below) so the queue entry is live on the remote for the overnight run.

Never route anything on the "맡기면 안 되는 작업" list to Ollama: cross-cutting design judgment
(Blueprint/C++ interaction, multi-file consistency), engine-API-accurate debugging, performance/
threading/replication work, or any final code that would be risky if committed unreviewed. That
work stays with Claude Code / Antigravity CLI as normal.

## Commit + push after every completed task

This project's default Collaboration Protocol (see `CLAUDE.md`) is "no commits without user
instruction." **That default is overridden for this specific case**: commit and push to GitHub
after finishing each discrete task or session, without waiting for the user to ask each time.
The multi-tool handoff model (Claude Code / Antigravity CLI / overnight Ollama) depends on
`production/overnight-queue.md`, `production/session-state/active.md`, and any newly-registered
tasks being live on the remote — not sitting uncommitted in one tool's local working tree.

- Still follow normal git hygiene: stage only the files that are actually part of the task you
  just completed (never blindly `git add -A` and never sweep in unrelated dirty-tree changes
  from other sessions without asking first), write a real Conventional Commits message, never
  force-push, never skip hooks, never commit anything that looks like a secret.
- This override applies to committing **your own completed work**. If the working tree already
  has unrelated uncommitted changes you did not make this session, leave them alone and ask the
  user before including them in the same commit — do not use this policy as blanket license to
  commit everything in sight.

## See also

- `production/ollama-instructions.md` — the actual selection criteria (Gemini reads this
  too, for when the overnight bot picks its own tasks)
- `production/overnight-queue.md` — the live task queue
- `production/overnight-protocol.md` — trust model + morning review routine
- `tools/overnight-bot/discord_ollama_bot.py` — the runner script
