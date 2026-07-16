# Overnight Worker Protocol (Ollama)

Third worker in rotation, alongside **Claude Code** and **Antigravity CLI**. Runs unattended at
night. Ollama here is a **plain chat model — no file access, no tool calling**. Whatever script
or process feeds it prompts is responsible for:

1. Reading the referenced source file(s) for each queued task
2. Substituting their content into the task's `{{CONTEXT}}` placeholder
3. Sending the assembled prompt to Ollama
4. Saving the raw response, verbatim, to the task's output path under `production/overnight-output/`
   — no automatic editing, no automatic merging into canonical files

## Trust model

Ollama's output is a **draft, never a commit**. It never touches `design/gdd/`, `docs/`,
`design/registry/entities.yaml`, or any source file directly — only
`production/overnight-output/`. A human (or Claude Code / Antigravity CLI, next session) reviews
every file in `overnight-output/` before anything gets promoted into a canonical file. This is
the same Question → Options → Decision → **Draft** → Approval pipeline this project already
uses everywhere else — Ollama's output just sits at the "Draft" step a little longer, until a
human/stronger-model review happens.

## Why these tasks and not others

Tasks in `overnight-queue.md` are chosen to be:
- **Self-contained**: all context needed is inlined into the prompt — no multi-file reasoning,
  no judgment calls about which file to read next.
- **Low-risk if wrong**: worst case is a bad draft that gets discarded, never a corrupted
  canonical file or an unnoticed subtle error in an approved design.
- **Fast to verify**: the review checklist under each task should take under 2 minutes — either
  the draft matches its source facts, or it doesn't. No judgment call about design quality
  should be required to catch a bad Ollama output.

Explicitly **not** given to Ollama: anything requiring creative/balance judgment (formulas, tuning
values, Player Fantasy framing), anything that edits a canonical file directly, anything where a
wrong answer is hard to spot without re-deriving the right one.

## Morning review routine (Claude Code or Antigravity CLI, start of session)

1. `ls production/overnight-output/` — see what Ollama produced overnight.
2. For each file, run its task's "Review checklist" (see `overnight-queue.md`) — most take under
   2 minutes since they're fact-checks against a known source, not open-ended review.
3. Good output → promote: copy/adapt the relevant content into the real canonical file yourself
   (Ollama drafts are a starting point, not final copy — still reword/verify before it lands in
   `design/gdd/` etc.), following the normal Question→Options→Decision→Draft→Approval flow with
   the user.
4. Bad/hallucinated output → discard, note in `production/session-state/active.md` if the task
   seems mis-scoped for Ollama (too hard, ambiguous, etc.) so `overnight-queue.md` can be trimmed.
5. Clear or archive `production/overnight-output/*` once reviewed, so tomorrow's run starts clean.
6. Update `overnight-queue.md`: mark completed tasks, add new ones as the project's design/doc
   backlog changes (see "Restocking the queue" below).

## Restocking the queue

When a session (Claude Code or Antigravity CLI) finishes real work and wants to leave Ollama
something to do overnight, add a new entry to `overnight-queue.md` **only if it fits the shape**
above (self-contained, low-risk, fast-to-verify) — the authoritative do/don't list is
`production/ollama-instructions.md` (also the file the Discord bot feeds to Gemini). If
nothing in the current project state fits that shape, leave the queue short rather than force a
bad-fit task in — an empty queue is better than a queue Ollama will confidently get wrong in ways
that take longer to review than to just do yourself.

**Standing rule (see `.claude/docs/ollama-delegation.md` / `AGENTS.md`)**: whenever a Claude Code
or Antigravity CLI session notices a sub-task that fits Ollama's criteria, it should route it
here instead of doing it inline — then commit + push so the queue entry is actually live for the
overnight run, not stuck in a local working tree.
