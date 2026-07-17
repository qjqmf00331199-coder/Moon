# Claude Code Game Studios -- Game Studio Agent Architecture

Indie game development managed through 38 coordinated Claude Code subagents.
Each agent owns a specific domain, enforcing separation of concerns and quality.

## Technology Stack

- **Engine**: Unreal Engine 5.8
- **Language**: C++ (primary), Blueprint (gameplay prototyping)
- **Version Control**: Git with trunk-based development
- **Build System**: Unreal Build Tool (UBT)
- **Asset Pipeline**: Unreal Content Pipeline

> **Note**: Engine-specialist agents: `ue-blueprint-specialist`, `ue-gas-specialist`,
> `ue-replication-specialist`, `ue-umg-specialist`.

## Project Structure

@.claude/docs/directory-structure.md

## Engine Version Reference

@docs/engine-reference/unreal/VERSION.md

## Technical Preferences

@.claude/docs/technical-preferences.md

## Coordination Rules

@.claude/docs/coordination-rules.md

## Collaboration Protocol

**User-driven collaboration, not autonomous execution.**
Every task follows: **Question -> Options -> Decision -> Draft -> Approval**

- Agents MUST ask "May I write this to [filepath]?" before using Write/Edit tools
- Agents MUST show drafts or summaries before requesting approval
- Multi-file changes require explicit approval for the full changeset
- No commits without user instruction, **except** the standing override in
  `.claude/docs/ollama-delegation.md` (commit + push after every completed task)
- **Auto-recommended override** (user standing instruction, 2026-07-17): whenever a skill's
  `AskUserQuestion` widget marks an option `(추천)`/`(Recommended)`, auto-select it and proceed
  without waiting for the user — do not stop to ask. Applies to every skill (`/design-review`,
  `/gate-check`, etc.), not just the one it was first requested for. Still stop and ask normally
  when no option is marked recommended, or when the action falls under the global safety rules'
  "explicit permission required" / "prohibited" categories (destructive/irreversible actions) —
  this override only removes the extra confirmation step for already-labeled-safe recommended
  choices, it does not bypass safety-critical confirmations.

See `docs/COLLABORATIVE-DESIGN-PRINCIPLE.md` for full protocol and examples.

> **First session?** If the project has no engine configured and no game concept,
> run `/start` to begin the guided onboarding flow.

## Ollama Delegation & Auto-Commit Policy

@.claude/docs/ollama-delegation.md

## Coding Standards

@.claude/docs/coding-standards.md

## Context Management

@.claude/docs/context-management.md
