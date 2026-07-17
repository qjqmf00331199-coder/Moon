# Fable 실행 프롬프트 — 남은 MVP 시스템 2개 (Luna Overdrive, Combat HUD)

> 이 파일은 Fable에게 그대로 붙여넣을 프롬프트다. 작업 디렉터리는 `D:\moon-fragment-hunt`
> (Claude Code Game Studios 프로젝트, 루트의 `CLAUDE.md` 및 `.claude/docs/*` 규칙 적용됨).
> 아래 지시문을 **한 번에, 사용자에게 아무 것도 묻지 않고** 끝까지 실행할 것.

---

## PROMPT (Fable에게 그대로 전달)

```
너는 D:\moon-fragment-hunt (Claude Code Game Studios, Unreal Engine 5.8 프로젝트)에서
작업한다. 먼저 CLAUDE.md와 .claude/docs/*.md를 읽고 프로젝트 규칙을 파악해라.

## 목표

design/gdd/systems-index.md 기준 남은 MVP 시스템 2개를 순서대로 완전히 설계한다:
1. Luna Overdrive (Blood Moon) — design/gdd/luna-overdrive.md (신규)
2. Combat HUD — design/gdd/combat-hud.md (신규)

두 GDD 모두 /design-system 스킬(.claude/skills/design-system/SKILL.md)의 8섹션 요구사항
(Overview, Player Fantasy, Detailed Design, Formulas, Edge Cases, Dependencies, Tuning Knobs,
Acceptance Criteria)을 전부 채워야 한다. Visual/Audio, UI Requirements, Open Questions도
작성한다(두 시스템 다 필수 대상 카테고리 — Luna Overdrive는 UI 반전 연출, Combat HUD는
UI 시스템 자체).

## 진행 전 반드시 읽을 파일

- design/gdd/game-concept.md (컨셉 전체, 특히 "2. 각성: 루나 오버드라이브" 섹션)
- design/gdd/systems-index.md (의존성, 우선순위, High-Risk 표)
- design/gdd/combo-tension-gauge.md (Luna Overdrive의 직접 상류 — OnOverdriveTriggered 발행자)
- design/gdd/spell-casting-base.md (특히 CostBypass.Active 태그 계약, Tuning Knobs)
- design/gdd/dash-evasion.md, design/gdd/health-damage-core.md (State.* 태그 시스템)
- design/registry/entities.yaml (기존 등록값 — 재선언 금지, 재사용만 할 것)
- production/review-mode.txt (현재 "solo" — 명시된 대로 특수 에이전트 스폰 전부 스킵)
- docs/consistency-failures.md (과거 충돌 패턴 참고)

## 절대 규칙 — 사용자에게 묻지 않고 한 번에 끝내기

/design-system 스킬은 원래 AskUserQuestion으로 매 섹션마다 사용자 확인을 받는다. 이번 실행은
**AskUserQuestion을 단 한 번도 호출하지 않고** 진행한다. 대신:

1. 스킬이 "(추천)/(Recommended)" 라벨을 붙이는 지점(Overview 프레이밍, Player Fantasy 직접/간접
   등)은 그 추천 옵션을 자동 선택한다 (CLAUDE.md의 기존 auto-recommended override와 동일 원칙).
2. 추천 라벨이 없는 결정 지점은: (a) 이미 승인된 상류 GDD의 기존 계약과 game-concept.md 서술에
   가장 부합하는 선택 → (b) 그래도 애매하면 가장 단순하고 보수적인 값 선택. 두 경우 다 결정
   이유를 해당 섹션 본문에 한 줄로 남기고 절대 멈추지 않는다.
3. 정말 확정할 수 없는 사안(예: 미설계 시스템과의 인터페이스)은 섹션 하단 Open Questions에
   "Owner: [담당], Target: [시점]"으로 기록하고 그대로 진행한다 — 절대 사용자에게 묻지 않는다.
4. review-mode는 "solo" 그대로 유지 — creative-director/systems-designer/art-director/qa-lead
   등 스페셜리스트 Task 스폰을 전부 스킵하고, 스킵한 섹션마다 "Note: `[agent]` 미컨설트 —
   Solo 모드. Production 전 수동 리뷰 필요." 한 줄을 남긴다.
5. /design-review는 이번 세션에서 실행하지 않는다 (독립 리뷰는 별도 세션 몫 — 이번 작업 범위
   밖). 각 GDD 완성 후 상태는 "Designed (pending review)"로 남긴다.

## Luna Overdrive 설계 시 반드시 지킬 계약 (이미 확정된 상류 값)

- Combo/Tension Gauge가 게이지 Max(100) 도달 시 `OnOverdriveTriggered`를 1회 발행하고 같은
  프레임에 게이지를 0으로 리셋한다 — Luna Overdrive는 이 이벤트의 **소비자**다. 페이로드가
  필요한지(트리거 위치 등)는 Combo/Tension Gauge의 Open Questions에 이미 "Luna Overdrive
  설계 시 확정 필요"로 넘겨져 있다 — 여기서 확정하고, 필요하면 그 결정을 두 문서 모두에
  일관되게 반영(단, combo-tension-gauge.md는 이미 Designed 상태이므로 실제 파일 수정이
  필요하면 반드시 실행하되 변경 이력을 그 파일 Open Questions에도 남길 것).
- Spell Casting (base)의 `CostBypass.Active` GameplayTag를 Luna Overdrive가 소유/구동한다 —
  이 태그 하나가 마나비용과 쿨다운을 **동시에** 우회한다(부분 우회 모드 없음, MVP 범위에서
  이미 확정됨 — spell-casting-base.md Edge Cases 참고). 이 태그의 On/Off 타이밍이 곧 오버드라이브
  지속시간의 실제 구현이다.
- game-concept.md 명시값: 지속시간 10초, 마나 무한 + 쿨다운 제로, 시각 반전(크림슨 레드).
  정확한 지속시간은 Tuning Knob으로 노출하되 컨셉의 10초를 기본값으로 삼는다.
- High-Risk 표(systems-index.md)의 "Combo/Tension Gauge → Luna Overdrive" 항목: 발동 난이도
  자체(임계값)는 Combo/Tension Gauge 소유이니 재설계하지 말 것 — Luna Overdrive는 "발동된
  이후 무엇이 일어나는가"만 소유한다.
- 종료 조건(시간 만료 vs 마나 소모 등)과 종료 시 CostBypass.Active 해제 타이밍(같은 프레임/
  다음 프레임)을 Edge Cases에서 명확히 못박을 것 — spell-casting-base.md가 이미 "bypass-release
  same-frame race: 태그 변경이 게이트 평가보다 먼저 해소된다"는 순서 계약을 Luna Overdrive가
  따라야 한다고 명시해뒀다(spell-casting-base.md Edge Cases 참고, 반드시 그 순서를 그대로 채택).

## Combat HUD 설계 시 반드시 지킬 계약

- 하류 소비 대상 3개: Combo/Tension Gauge(게이지 값+Building/Decaying 상태), Luna Overdrive
  (오버드라이브 진입/잔여시간/크림슨 레드 전환 트리거), Core Extraction Execution(F키 처형
  프롬프트 — **이 시스템은 아직 미설계**이므로 인터페이스를 가정해서 Open Questions에 명시
  "Core Extraction Execution 설계 시 실제 이벤트명/데이터 확정 필요"로 남기고 진행할 것.
  가정 인터페이스: `State.Executable` 태그가 대상에 부여되어 있고 근접 조건 충족 시 F키
  프롬프트 표시(health-damage-core.md / dash-evasion.md에 이미 있는 `State.Executable` 태그
  존재 여부만으로 최소 구현 가능하다고 가정 — 정밀한 트리거 신호는 Core Extraction Execution
  GDD가 나중에 확정).
- UI 카테고리 — 패드 내비게이션 지원 필요(technical-preferences.md 명시: "UI는 패드 네비게이션
  지원 필요"). 각 위젯(게이지 바, 오버드라이브 상태 표시, 처형 프롬프트)이 읽기 전용
  표시인지 입력 대상인지 구분해서 서술할 것 — 이번 3개는 전부 읽기전용 표시로 간주(입력은
  게임플레이 액션 키가 직접 처리, HUD는 상태만 반영).
- 60fps/16.6ms 프레임 예산(technical-preferences.md) 안에서 동작해야 함 — Tuning Knobs에
  갱신 주기(매 틱 vs 이벤트 기반) 관련 노브를 하나 넣을 것.

## Ollama 위임 규칙 (qwen3:8b가 처리 가능한 하위 작업은 네가 직접 하지 말고 큐에 등록)

`.claude/docs/ollama-delegation.md`와 `production/ollama-delegation-criteria.md`를 따른다.
GDD의 핵심 설계 판단(Formulas/Player Fantasy/Edge Cases/Tuning Knobs/Core Rules, 밸런스
수치)은 **절대 위임 금지** — 이건 네가 직접 다 쓴다. 다만 아래 3종 하위 작업은 Ollama 큐로
넘기고 너는 실행하지 않는다:

각 GDD(Luna Overdrive, Combat HUD)가 완성될 때마다, 그 GDD에 대해 아래 3개 태스크를
OLLAMA-INSTRUCTIONS.md(레포 루트)에 **기존 Task 9/10과 정확히 같은 포맷**으로 추가한다
(현재 마지막 태스크 번호를 확인하고 그 다음 번호부터 이어서 붙일 것 — 지금 기준 Task 11부터):

1. **Registry vs GDD fact-check** — Task 9와 동일한 프롬프트 패턴, Context files를
   `design/registry/entities.yaml` + 해당 신규 GDD로 교체.
2. **한/영 용어 일관성 체크** — Task 10과 동일한 패턴, Context files를 신규 GDD +
   그 시스템의 가장 밀접한 상류 의존 GDD(Luna Overdrive는 combo-tension-gauge.md,
   Combat HUD는 luna-overdrive.md)로 교체.
3. **GIVEN-WHEN-THEN Acceptance Criteria → 평문 QA 체크리스트 변환** — 신규 태스크 유형.
   아래 템플릿으로 추가:

   ```
   ## [ ] Task N — Acceptance Criteria를 QA 체크리스트로 변환 ([시스템명])

   **Why queued**: [시스템명].md GDD가 완성됨. GIVEN-WHEN-THEN 형식의 Acceptance Criteria를
   QA가 바로 쓸 수 있는 평문 체크리스트로 기계적으로 변환 — 판단 없는 순수 재서식.

   **Risk**: Low. Report-only, 원본 GDD는 수정하지 않음.

   **Context files**:
   - {{CONTEXT}}: design/gdd/[system-slug].md

   **Prompt**:
   \`\`\`
   아래는 게임 디자인 문서의 Acceptance Criteria 섹션이다(GIVEN-WHEN-THEN 형식). 이것을
   QA 테스터가 체크박스로 바로 쓸 수 있는 평문 체크리스트로 변환해라. 각 항목은:

   - [ ] [조건을 간결한 평서문으로 — GIVEN/WHEN/THEN 구조를 자연스러운 한 문장으로 합칠 것]

   형식만 바꿀 것 — 새 조건을 추가하거나 기존 조건을 생략/수정하지 마라. 원문에 없는 내용은
   절대 넣지 마라.

   ---
   ACCEPTANCE CRITERIA:
   {{CONTEXT}}
   \`\`\`

   **Output path**: `production/overnight-output/taskN-qa-checklist-[system-slug].md`

   **Review checklist** (under 2 min): 원본 AC 개수와 체크리스트 항목 개수 일치 확인, 조건
   누락/추가 없는지 대조.

   ---
   ```

## 완료 조건 및 커밋/푸시

각 GDD(Luna Overdrive 먼저, 그 다음 Combat HUD)를 완성할 때마다 아래를 그 GDD 완료 직후
순서대로 실행한다 (하나 끝날 때마다 커밋 — 두 번 커밋하게 됨):

1. 8개 필수 섹션 + Visual/Audio + UI Requirements + Open Questions 전부 채워졌는지
   `[To be designed]` 잔존 여부로 셀프체크.
2. design/registry/entities.yaml에 새 상수/포뮬러 등록 (기존 값 재선언 금지, 재사용만).
3. design/gdd/systems-index.md 해당 행 상태를 "Designed (pending review)"로, Design Doc
   링크 채우고, Progress Tracker 카운트 갱신.
4. 위 "Ollama 위임 규칙"에 따라 OLLAMA-INSTRUCTIONS.md에 태스크 3개 추가.
5. production/session-state/active.md에 이번 작업 요약 append (기존 세션 로그 포맷 그대로
   따를 것 — STATUS 블록 갱신 + "## What changed this session" 항목 추가).
6. Combat HUD까지 다 끝난 시점(즉 두 번째 GDD 완료 시)에는 추가로:
   - `/consistency-check` 절차를 수동으로 수행: design/registry/entities.yaml의 전체
     항목을 design/gdd/*.md(game-concept.md, systems-index.md 제외) 전체와 대조, 충돌
     발견 시 registry의 source 필드 기준으로 직접 해결(사용자에게 묻지 않고 스스로 결정),
     docs/consistency-failures.md에 발견된 충돌이 있었다면 로그 추가.
   - production/session-state/active.md에 `<!-- CONSISTENCY-CHECK: ... -->` 주석 추가.
7. git 커밋 + 푸시 (사용자가 이 작업 전체를 사전 승인함 — 추가 확인 불필요):
   - `git add`로 이번 단계에서 실제로 건드린 파일만 스테이징 (해당 GDD 파일, registry,
     systems-index.md, OLLAMA-INSTRUCTIONS.md, production/session-state/active.md,
     충돌 있었으면 docs/consistency-failures.md) — `git add -A` 금지, 무관한 변경 섞지 말 것.
   - Conventional Commits 형식으로 커밋 메시지 작성 (`docs:` 또는 `feat:` 접두사), 예:
     `docs: author Luna Overdrive GDD, queue Ollama fact-check/consistency tasks`
   - `git push` (force 금지, `--no-verify` 금지, hook 실패 시 원인 고치고 재커밋 — amend 금지).
   - 최종(Combat HUD까지 끝난 뒤) 커밋에는 consistency-check 결과와 "MVP 9/9 설계 완료"
     여부를 커밋 메시지 본문에 한 줄 남길 것.

작업 끝나면 별도 요약 보고 없이 그냥 커밋/푸시까지 끝내고 세션을 종료해도 된다 — 사용자는
다음에 돌아와서 production/session-state/active.md와 git log로 확인한다.
```

---

## 이 프롬프트를 만들며 참고한 것

- 남은 MVP: `design/gdd/systems-index.md` 기준 #11 Luna Overdrive, #20 Combat HUD (7/9 완료
  상태에서 이어짐 — Combo/Tension Gauge 방금 완료).
- Ollama 위임 3종(레지스트리 팩트체크·용어 일관성·AC→체크리스트)은 `OLLAMA-INSTRUCTIONS.md`의
  Task 9/10 기존 패턴 + "Adding new tasks" 섹션이 명시적으로 좋은 후보로 꼽은 것(AC 재서식)을
  그대로 재사용 — 새 포맷을 발명하지 않음.
- 핵심 설계 판단(포뮬러/밸런스/플레이어 판타지/엣지케이스)은 `ollama-delegation-criteria.md`의
  "맡기면 안 되는 작업" 목록에 명시적으로 걸려 Fable이 직접 하도록 프롬프트에 못박음.
- 커밋+푸시는 사용자가 이 메시지에서 명시적으로 사전승인 — 세션당 추가 확인 없이 진행하도록
  프롬프트에 반영(단, `git add -A` 금지 등 기존 안전수칙은 유지).
