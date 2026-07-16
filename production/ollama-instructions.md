# Ollama 지시사항 (Overnight Task Queue)

See `production/overnight-protocol.md` for the trust model and review routine before running
any of these. Each task below is self-contained: whatever runs Ollama must read the referenced
file(s) and paste their full content in place of `{{CONTEXT}}` before sending the prompt.
Save Ollama's raw reply, unedited, to the listed output path.

Status legend: `[ ]` queued · `[x]` done, reviewed · `[~]` done, discarded (bad output)

---

## [ ] Task 1 — Reverse-document the Arena Morphing spike

**Why queued**: flagged at session start as an undocumented prototype gap
(`prototypes/arena-morphing-spike-2026-07-16/` has no README/CONCEPT doc). Pure summarization
of an already-completed, already-verified spike — nothing to invent, nothing to decide.

**Risk**: Low. Source of truth (`SPIKE-NOTE.md`) is fixed and already verified — a wrong summary
is trivially caught by comparing the two side by side.

**Context to inject**: full contents of
`prototypes/arena-morphing-spike-2026-07-16/SPIKE-NOTE.md`

**Prompt**:
```
You are drafting a short CONCEPT.md for a completed game-dev spike/prototype. Below is the
full spike note. Summarize it into this exact structure, using ONLY facts stated in the note —
do not invent capabilities, numbers, or conclusions not present in the source text. If something
is unclear or not stated, write "Not stated in source" rather than guessing.

# Arena Morphing — Nanite/Geometry Collection Spike

## What was tested
[1-2 sentences]

## Result
[APPROVED/REJECTED/INCONCLUSIVE — state the verdict exactly as the source states it]

## Method
[how it was verified — bullet list]

## Gotchas found
[bullet list, only if the source mentions any]

## Still open / follow-up needed
[bullet list, only if the source mentions any]

---
SOURCE SPIKE NOTE:
{{CONTEXT}}
```

**Output path**: `production/overnight-output/task1-arena-morphing-concept.md`

**Review checklist** (under 2 min): Open both files side by side.
- [ ] Every claim in the draft traces to a sentence in SPIKE-NOTE.md — no invented numbers/facts
- [ ] The verdict (APPROVED/etc.) matches the source exactly
- [ ] Nothing from the source that materially matters (e.g. the headless-bake gotcha) got dropped
- If it passes: promote into `prototypes/arena-morphing-spike-2026-07-16/CONCEPT.md` (light edit
  pass first, don't paste verbatim without reading it).

---

## [ ] Task 2 — Registry vs. GDD fact-check (Player Movement)

**Why queued**: this exact category of bug already happened once this project (the `air_control`
constant drifted to a stale value after a GDD revision and nobody caught it until it was
noticed by chance). A mechanical diff between the registry and the GDD is exactly the kind of
check a weak model can do reliably — it's pattern matching, not judgment.

**Risk**: Low. This produces a report only — no file is edited. Worst case, a false-positive
"mismatch" that a human dismisses in 10 seconds by checking the actual number.

**Context to inject**: full contents of `design/registry/entities.yaml` AND full contents of
`design/gdd/player-movement.md`

**Prompt**:
```
You are cross-checking two documents for a game design registry. The first is a YAML registry
of named constants/formulas; the second is a design document that should agree with it wherever
they both mention the same named value.

List every case where a numeric value, formula, or named constant appears in BOTH documents
but with DIFFERENT values. Format each finding as:

- [name]: registry says [X], GDD says [Y] (GDD section: [section name])

If you find no mismatches, say so explicitly — do not invent a mismatch to have something to
report. Do not comment on anything else (style, completeness, missing sections) — ONLY numeric/
value disagreements between the two documents.

---
REGISTRY (entities.yaml):
{{CONTEXT_REGISTRY}}

---
GDD (player-movement.md):
{{CONTEXT_GDD}}
```

**Output path**: `production/overnight-output/task2-registry-check-player-movement.md`

**Review checklist** (under 2 min): For each flagged mismatch, open both source files and check
the actual values at the cited locations.
- [ ] Confirmed real mismatches → fix registry or GDD (whichever is stale) directly, following
      the same pattern used to fix the `air_control` drift (registry `revised:` date + inline
      comment noting the correction)
- [ ] False positives → discard, no action

---

## [ ] Task 3 — Registry vs. GDD fact-check (Health/Damage Core)

Same task as Task 2, same prompt template, different GDD target — run once Health/Damage Core
has values worth registry-checking (it does: `max_health_player`, `effective_damage_applied`,
`new_health` were just added to the registry this session).

**Context to inject**: full contents of `design/registry/entities.yaml` AND full contents of
`design/gdd/health-damage-core.md`

**Prompt**: identical template to Task 2, swap `{{CONTEXT_GDD}}` for health-damage-core.md's content.

**Output path**: `production/overnight-output/task3-registry-check-health-damage-core.md`

**Review checklist**: same as Task 2.

---

## [ ] Task 4 — Korean/English terminology consistency report

**Why queued**: this project's GDDs mix Korean prose with English technical terms
(`bUseControllerRotationYaw`, "저스트회피", "코어 적출" vs "Core Extraction" etc.) — a report-only
pass flagging where the SAME concept is referred to with different terms/spelling across
documents is mechanical text comparison, not design judgment.

**Risk**: Low. Report only, no file touched. A missed inconsistency just means the queue runs
this task again later; a false positive costs a human 10 seconds to dismiss.

**Context to inject**: full contents of `design/gdd/player-movement.md` AND full contents of
`design/gdd/health-damage-core.md`

**Prompt**:
```
Below are two Korean-language game design documents that share some concepts (they're
Foundation-layer systems in the same project). List every case where the SAME concept appears
to be referred to with inconsistent terminology between the two documents — different Korean
phrasing, inconsistent English term choice, or Korean vs. English used inconsistently for the
same thing. Format each finding as:

- Concept: [what it refers to] — Doc A says "[term]", Doc B says "[term]"

Only flag genuine same-concept naming inconsistencies. Do not flag terms that only appear in one
document (that's not an inconsistency, just scope). Do not comment on grammar, style, or
translation quality — ONLY cross-document naming consistency.

---
DOCUMENT A (player-movement.md):
{{CONTEXT_A}}

---
DOCUMENT B (health-damage-core.md):
{{CONTEXT_B}}
```

**Output path**: `production/overnight-output/task4-terminology-consistency.md`

**Review checklist** (under 2 min): skim the list — for each flagged pair, decide if it's a real
inconsistency worth standardizing or an intentional distinction. No file gets edited from this
task directly; if something real turns up, fix it by hand in the relevant GDD(s).

---

## [ ] Task 5 — Registry vs. GDD fact-check (Enemy AI)

**Why queued**: We just approved `enemy-ai-base.md` this session. A mechanical check to ensure that the registry constants (`max_health_grunt`, `max_health_ranged`, `min_engage_range`, `max_engage_range`, `min_telegraph_window`, etc.) and formulas in `entities.yaml` match the values and descriptions in the approved GDD.

**Risk**: Low. Report-only task that edits no files. Mismatches are easy to verify.

**Context to inject**: full contents of `design/registry/entities.yaml` AND full contents of `design/gdd/enemy-ai-base.md`

**Prompt**: identical template to Task 2, swap `{{CONTEXT_GDD}}` for `enemy-ai-base.md`'s content.

**Output path**: `production/overnight-output/task5-registry-check-enemy-ai.md`

**Review checklist** (under 2 min): Same as Task 2.

---

## [ ] Task 6 — Korean/English terminology consistency report (Enemy AI vs. Health/Damage Core)

**Why queued**: Both are core systems that interact with each other (e.g., `OnDeath` trigger, `MaxHealth` attributes, and tag checking). This report flags naming and casing inconsistencies for concepts shared between the two files.

**Risk**: Low. Report-only task.

**Context to inject**: full contents of `design/gdd/enemy-ai-base.md` AND full contents of `design/gdd/health-damage-core.md`

**Prompt**: identical template to Task 4.

**Output path**: `production/overnight-output/task6-terminology-consistency-enemy-ai.md`

**Review checklist** (under 2 min): Same as Task 4.

---

## Adding new tasks

Only add a task here if it fits `overnight-protocol.md`'s three criteria: self-contained,
low-risk-if-wrong, fast-to-verify. Good candidate shapes going forward as the project grows:
- More registry-vs-GDD fact-checks, one per newly-approved GDD
- More terminology-consistency passes between newly-approved GDD pairs
- Reverse-documenting any other prototype/spike that lands without a doc
- Mechanical reformatting passes (e.g. GIVEN-WHEN-THEN Acceptance Criteria → a flat QA checklist)
  once a GDD has enough Acceptance Criteria to be worth transcribing

Do NOT add: formula/balance design, Player Fantasy writing, core rule design, anything that
edits `design/gdd/*`, `docs/*`, or `entities.yaml` directly, anything needing multi-file judgment
Ollama can't verify from the pasted context alone.
