# Review Log — spell-casting-base.md

## Review — 2026-07-17 — Verdict: APPROVED (after in-session revision; initial verdict NEEDS REVISION)
Scope signal: M
Specialists: game-designer, systems-designer, qa-lead, ue-gas-specialist, creative-director (senior synthesis)
Blocking items: 5 | Recommended: 6
Summary: creative-director 종합 — 파이프라인 뼈대(단일 데미지 진입점, 속성별 쿨다운, 태그 우회)는 견고하나
필라 두 축이 흔들림: (1) Blackhole 코스트 100=MaxMana가 캐스트 직후 전 스펠 3.1초 락아웃을 만들어
초신성 위빙 진입을 봉인(→ 70으로 하향, 위빙 1회 보장 교차 제약 신설), (2) Rule 4 캔슬과 Rule 10 우회의
GAS 구현 메커니즘 백지(→ 동일 프레임 라이프사이클/CommitAbility 스킵 분기 계약 명문화).
그 외: Formulas 117행 vs Tuning Knobs 172행 자기모순 해소, (구)AC8 초신성 검증을 Spell Weaving GDD로 이관,
AC5 스텁화/AC7 관측가능화, Edge Case 3건 추가(Mana=0, 전속성 쿨다운, 우회 해제 프레임 레이스),
무한 스팸 방지 교차 제약(ManaCost > Regen×CD) 명시, NoiseLoudness 서수 불변식 추가.
Prior verdict resolved: First review

### Blocking items resolved in-session
1. [game-designer + creative-director] Blackhole ManaCost 100 → 70 (registry `mana_cost_blackhole` 갱신)
2. [ue-gas-specialist] Rule 4 캔슬 메커니즘 확정 — 동일 프레임 활성화→End, 프레젠테이션만 캔슬, CancelAbility 불사용
3. [ue-gas-specialist] Rule 10 구현 경로 확정 — CostBypass.Active 시 CommitAbility 스킵 분기(문서화된 의도적 예외) + 마나 스냅 GE 순서 무해성(benign race) 명문화
4. [game-designer/systems-designer] Formulas-Tuning Knobs 자기모순 해소 + Safe Range/하드클램프 관계 명시 + 교차 제약 3건 신설
5. [qa-lead] (구)AC8 제거·이관, AC5/AC7 재작성

### Advisory (남은 항목 — Open Questions 등재)
- 밸런스 수치 플레이테스트 검증 (특히 Blackhole 70의 체감 무게)
- InsufficientMana 실패 피드백 톤 (거부 UX 우려)
- 캐스트 조준 방식 / 게임패드 매핑 (기존 Open Questions 유지)
- UE5.8 GAS 초기화 헤더 대조 (구현 착수 전 필수)

## Review — 2026-07-23 — Verdict: APPROVED
Scope signal: M
Specialists: (lean review — solo pass, project default)
Blocking items: 0 | Recommended: 1 (fixed this session)
Summary: 2026-07-21 sync (Rule 7 mana-regen pause during Overdrive Active, Rule 10 lazy
CurrentTime-vs-OverdriveEndTime check) cross-checked against luna-overdrive.md's independent Rule
9/4 — both sides agree on the same behavior and hand-off contract (Luna Overdrive delivers the
time predicate into this doc's two gate points). Full registry sweep of every Tuning Knob
(max_mana, mana_cost_fire/lightning, cooldown_blackhole/fire/lightning, mana_regen_rate,
max_casts_per_second, spell_noise_loudness) found zero drift — all match entities.yaml exactly.

Recommended item fixed in-session: Combat HUD was consuming 3 declared UI interfaces (Mana/MaxMana,
per-element cooldown state, CostBypass.Active) but was never listed in the Dependencies table —
added as a formal row + one Interactions-section line.
Prior verdict resolved: Yes — status updated from "Needs Revision Review" to "Approved"
