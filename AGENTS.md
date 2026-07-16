# AGENTS.md — Moon Fragment Hunt

Shared context file for **Antigravity CLI** (Gemini). Claude Code reads `CLAUDE.md` +
`.claude/` instead — this file is the Antigravity-readable mirror of the same project
rules, kept so work can hand off between the two tools without re-explaining context.

## Single source of truth for "what's next"

**Always read `production/session-state/active.md` first**, in either tool. It holds:
- Current task + step-by-step resume instructions
- Decisions/flags carried forward from prior sessions
- Open questions / unresolved risks

Update that file at the end of every session (either tool), not just this one.

## Engine & Stack

- Engine: **Unreal Engine 5.8** (project pinned 2026-07-16 — see `docs/engine-reference/unreal/VERSION.md`.
  Engine version is past both models' training cutoff — cross-check `breaking-changes.md` and
  `deprecated-apis.md` in that folder before trusting either model's native UE knowledge).
- Language: C++ (primary), Blueprint (gameplay prototyping)
- Rendering: Lumen + Nanite. Physics: Chaos.
- Naming: `A`/`U`/`F` prefixed PascalCase classes, PascalCase vars/events, `b` prefix for bools.

## MCP

- `unreal-mcp` (local HTTP, `http://127.0.0.1:8000/mcp`) — direct Unreal Editor scripting.
  Registered for Claude Code in `.mcp.json` / `Moon/.mcp.json`; registered for Antigravity CLI
  in `.agents/mcp_config.json` (same server, different config schema — Antigravity uses
  `serverUrl` instead of `type`+`url`).
- Editor must have the `AllToolsets` plugin enabled (already set in `Moon.uproject`) for the
  full toolset list (~20 toolsets) to appear — restart the editor if only a trivial
  `AgentSkillToolset` shows up.
- Batch `unreal-mcp` calls where possible; don't repeat `describe_toolset` calls needlessly —
  context is shared budget across both tools' sessions.

## Directory Structure

```
design/gdd/          — one GDD per system, 8 required sections (see below)
design/registry/      — entity/stat registry (entities.yaml) — cross-check GDDs against this
docs/engine-reference/unreal/ — version-pinned UE5.8 API notes; consult before any engine API call
production/session-state/active.md — living handoff file, READ FIRST
production/session-state/plan.md   — a SEPARATE, unrelated in-progress checklist (Enemy BP tree
                                       work) — do not overwrite or mark items done unless you are
                                       actually doing that specific work
prototypes/           — throwaway spikes, isolated from production content
```

## Design Document Standard (GDD)

Every file under `design/gdd/` must have these 8 sections, in order:
1. Overview 2. Player Fantasy 3. Detailed Rules 4. Formulas 5. Edge Cases
6. Dependencies (bidirectional — if A depends on B, B's doc must mention A back)
7. Tuning Knobs (with safe ranges) 8. Acceptance Criteria (QA-testable, not "should feel good")

Author incrementally: skeleton with empty headers first, fill one section at a time, write each
approved section to disk immediately. Update `design/gdd/systems-index.md` status/progress table
whenever a GDD's status changes (Not Started → Draft → Approved).

Claude Code has a guided `/design-system` skill for this; Antigravity has no equivalent slash
command — just follow the section list and rules above directly, or ask the user to run
`/design-system` in a Claude Code session if a guided pass is wanted.

## Review

Claude Code's `/design-review` skill produces the design-review verdict format currently used in
this project (see `design_review_player_movement.md` referenced from `active.md` for the format
this project has been using). If Antigravity is asked to review a GDD, produce a verdict in the
same shape: PASS/CONCERNS/FAIL-style verdict + specific section-by-section gaps, not vague praise.

## Coding Standards (once implementation starts)

- Gameplay values are data-driven (external config), never hardcoded
- Every system needs an ADR in `docs/architecture/` before implementation
- Commits: Conventional Commits (`feat:`, `fix:`, `chore:`, ...), reference story/task ID
- No merge if tests fail; never disable/skip a failing test to unblock CI

## Overnight worker (Ollama)

A third worker, Ollama, runs unattended overnight on a small queue of low-risk, self-contained,
fast-to-verify tasks (see `production/overnight-queue.md` and `production/overnight-protocol.md`).
It has no file/tool access and never touches canonical files — it only writes drafts to
`production/overnight-output/`. **Check that directory at the start of any session** — review and
promote/discard its drafts before starting new work, then clear the directory.

Selection criteria for what Ollama is and isn't allowed to do: `production/ollama-delegation-criteria.md`.
This is the same file the Discord bot (`tools/overnight-bot/discord_ollama_bot.py`) feeds to
Gemini, so it's the single source of truth for both the bot and Antigravity/Claude Code.

### Delegate simple work to Ollama instead of doing it yourself

Before doing any sub-task you notice along the way (not just the user's main ask — incidental
doc-writing, boilerplate, cleanup), check it against `production/ollama-delegation-criteria.md`.
If it matches the "맡기기 좋은 작업" list (documentation, comments, boilerplate getters/setters,
simple UPROPERTY/UFUNCTION declarations, commit-message polish, TODO extraction, log/error
summarization, small deterministic utility scripts, folder/naming cleanup, text formatting/
translation) — **do not do it yourself**. Instead, add a new task entry to
`production/overnight-queue.md` (follow the existing task template: Why queued / Risk / Context
to inject / Prompt / Output path / Review checklist), leave the actual work undone, and commit +
push so the entry is live for the overnight run. Never route cross-cutting design judgment,
engine-API-accurate debugging, or perf/threading/replication work to Ollama — that stays with you.

### Commit + push after every completed task

The Collaboration norm below still holds ("don't commit unless asked") **except** for this one
standing override: commit and push to GitHub after finishing each discrete task or session,
without waiting to be asked each time. The multi-tool handoff model depends on
`production/overnight-queue.md`, `production/session-state/active.md`, and any newly-registered
tasks being live on the remote, not sitting uncommitted in one tool's local working tree. Stage
only the files that are actually part of the task you just completed — don't sweep in unrelated
pre-existing dirty-tree changes from other sessions without asking first — and never commit
anything that looks like a secret.

## Collaboration norm

This project's Claude Code setup asks "may I write this to [filepath]?" before writing/editing and
shows drafts before requesting approval, with no commits without explicit instruction. Follow the
same norm in Antigravity: confirm before writing new design/code files, and don't commit or push
unless the user explicitly asks.
