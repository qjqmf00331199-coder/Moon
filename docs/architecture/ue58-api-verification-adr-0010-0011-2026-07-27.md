# UE5.8 API Verification: ADR-0010 / ADR-0011

Date: 2026-07-27
Engine checked: local install at `C:\Program Files\Epic Games\UE_5.8`
Scope: implementation-time blockers from ADR-0010 and ADR-0011 only.

## Verdict

PASS for story-readiness gating.

The ADR-0010 CommonUI glyph/input-method assumptions and ADR-0011 GAS/TickGroup assumptions were checked against the installed UE5.8 headers. The APIs required by the ADRs exist with compatible signatures. Dependent implementation stories may be marked Ready with the implementation notes below.

This does not implement the HUD or Combo/Tension code. It only verifies that the planned UE5.8 API surface is real enough to implement against.

## ADR-0010: CommonUI Glyph / Input Method

Result: VERIFIED.

Findings:

- `UCommonInputSubsystem` is still a `ULocalPlayerSubsystem`.
- Native input-method changes use `FInputMethodChangedEvent OnInputMethodChangedNative`.
- The native callback payload is `ECommonInputType`.
- `GetCurrentInputType()` and `GetCurrentGamepadName()` are public Blueprint-callable accessors.
- `BroadcastInputMethodChanged()` broadcasts `OnInputMethodChangedNative.Broadcast(CurrentInputType)`.
- Glyph data is still available through CommonUI controller-data brush maps:
  - `UCommonInputBaseControllerData::TryGetInputBrush(FSlateBrush&, FKey)`
  - `UCommonInputBaseControllerData::TryGetInputBrush(FSlateBrush&, const TArray<FKey>&)`
  - `UCommonInputPlatformSettings::TryGetInputBrush(...)`
- Enhanced Input is directly supported by `UCommonActionWidget::SetEnhancedInputAction(UInputAction*)` and `CommonUI::GetIconForEnhancedInputAction(...)`.

Implementation notes:

- For a custom execution-prompt glyph, subscribe with:
  `InputSubsystem->OnInputMethodChangedNative.AddUObject(this, &ThisClass::HandleInputMethodChanged)`.
- Handler signature should be:
  `void HandleInputMethodChanged(ECommonInputType NewInputType)`.
- Initial state should read `InputSubsystem->GetCurrentInputType()` during bind/construct.
- Preferred glyph source for this project is `UCommonActionWidget` with `SetEnhancedInputAction(ExecuteAction)` if the prompt can be represented as a widget. If the existing HUD keeps a plain `Image`, resolve a brush through CommonUI's platform/controller-data path instead of hardcoding keyboard/gamepad textures.

Header evidence:

- `CommonInputSubsystem.h`: declares `FInputMethodChangedEvent OnInputMethodChangedNative`, `GetCurrentInputType()`, and `GetCurrentGamepadName()`.
- `CommonInputSubsystem.cpp`: `BroadcastInputMethodChanged()` broadcasts `OnInputMethodChangedNative.Broadcast(CurrentInputType)`.
- `CommonInputBaseTypes.h/.cpp`: controller data and platform settings expose `TryGetInputBrush(...)`.
- `CommonActionWidget.h`: exposes `SetEnhancedInputAction(UInputAction*)`, `GetIcon()`, and input-icon update delegates.
- `CommonUITypes.cpp`: `CommonUI::GetIconForEnhancedInputAction(...)` resolves the first key for the current input type, then asks `UCommonInputPlatformSettings::TryGetInputBrush(...)`.

## ADR-0010: GAS Cooldown Query Surface

Result: VERIFIED.

Findings:

- `UAbilitySystemComponent::RegisterGameplayTagEvent(FGameplayTag, EGameplayTagEventType::Type)` exists and returns `FOnGameplayEffectTagCountChanged&`.
- `UAbilitySystemComponent::GetActiveEffectsTimeRemaining(const FGameplayEffectQuery&)` exists.
- `UAbilitySystemComponent::GetActiveEffectsTimeRemainingAndDuration(const FGameplayEffectQuery&)` exists and is preferable for HUD fraction math when duration and remaining are both needed.
- `FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(const FGameplayTagContainer&)` exists.
- UE's own `UGameplayAbility::GetCooldownTimeRemainingAndDuration()` uses the same owning-tag query plus `GetActiveEffectsTimeRemainingAndDuration(...)` pattern.

Implementation notes:

- ADR-0010's accessor shape is valid.
- Prefer backing `GetElementCooldownRemaining/Duration` with `GetActiveEffectsTimeRemainingAndDuration(...)` to avoid maintaining a separate duration constant for the active effect path.
- Keep the ADR's Spell Casting ownership boundary: HUD should call `UMoonAbilitySystemComponent` accessors, not hardcode cooldown tags.

Header evidence:

- `AbilitySystemComponent.h`: `RegisterGameplayTagEvent`, `GetActiveEffectsTimeRemaining`, and `GetActiveEffectsTimeRemainingAndDuration`.
- `GameplayEffect.h`: `FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags`.
- `GameplayAbility.cpp`: cooldown remaining/duration query uses `MakeQuery_MatchAnyOwningTags(*CooldownTags)` and `GetActiveEffectsTimeRemainingAndDuration(Query)`.

## ADR-0011: `FOnAttributeChangeData::GEModData`

Result: VERIFIED for the ADR's filter.

Findings:

- `FOnAttributeChangeData` has `NewValue`, `OldValue`, and `const FGameplayEffectModCallbackData* GEModData`.
- Default construction initializes `GEModData` to `nullptr`.
- Replication/direct dirty-broadcast path constructs callback data with `GEModData = nullptr`.
- `InternalUpdateNumericalAttribute(...)` sets `GEModData` to either the explicit modifier data or cached current modifier callback data, then broadcasts the new attribute-value delegate.
- Existing GAS ability tasks already inspect `CallbackData.GEModData`.

Implementation notes:

- ADR-0011's filter `Data.GEModData != nullptr && Data.NewValue < Data.OldValue` is valid for distinguishing GameplayEffect-originated Health decreases from direct/non-GE attribute changes.
- Keep the `NewValue < OldValue` half of the predicate; `GEModData != nullptr` alone means GE-originated change, not necessarily damage.

Header/source evidence:

- `GameplayEffectTypes.h`: `FOnAttributeChangeData` layout and default `GEModData(nullptr)`.
- `GameplayEffect.cpp`: direct/replication broadcast path sets `CallbackData.GEModData = nullptr`.
- `GameplayEffect.cpp`: `InternalUpdateNumericalAttribute(...)` assigns `CallbackData.GEModData = DataToShare`.
- `AbilityTask_WaitAttributeChange.cpp`: reads `CallbackData.GEModData`.

## ADR-0011: `TG_PostUpdateWork`

Result: VERIFIED for current intended call sites.

Findings:

- `ETickingGroup` in UE5.8 includes `TG_PostUpdateWork`.
- Its engine comment says it is for work after all normal gameplay tasks.
- `FTickFunction` exposes configurable `TickGroup` and `EndTickGroup`.
- `LevelTick.cpp` runs groups in order: `TG_PrePhysics`, `TG_StartPhysics`, `TG_DuringPhysics`, `TG_EndPhysics`, `TG_PostPhysics`, `TG_PostUpdateWork`, then `TG_LastDemotable`.
- Default `AActor`/`APawn`/`APlayerController` ticks are `TG_PrePhysics`.
- `UCharacterMovementComponent` uses `TG_PrePhysics` plus a `TG_PostPhysics` tick function.
- `USpringArmComponent` ticks in `TG_PostPhysics`.
- GAS itself uses `TG_PostUpdateWork` for trace target actors, which confirms the group is a valid gameplay-facing scheduling point.

Implementation notes:

- ADR-0011's `FTensionResolveTickFunction.TickGroup = TG_PostUpdateWork` plan is valid.
- For current planned gain call sites, the ordering precondition is satisfied:
  - spell hit and Just-Dodge success are synchronous gameplay/ability calls;
  - current character, pawn, controller, CMC, and spring arm ticks all occur no later than `TG_PostPhysics`;
  - the tension resolve tick then runs in `TG_PostUpdateWork`.
- Future latent `AbilityTask` completions or custom Blueprint tick functions explicitly scheduled at `TG_PostUpdateWork` or `TG_LastDemotable` must not call `AddTension*` unless they also declare a prerequisite ordering before `TensionResolveTickFunction`.

Header/source evidence:

- `EngineBaseTypes.h`: `ETickingGroup` enum includes `TG_PostUpdateWork`; `FTickFunction` owns `TickGroup`/`EndTickGroup`.
- `LevelTick.cpp`: declares and runs `TG_PostUpdateWork` before `TG_LastDemotable`.
- `Actor.cpp`, `Pawn.cpp`, `PlayerController.cpp`: default actor/controller tick group is `TG_PrePhysics`.
- `CharacterMovementComponent.cpp`: CMC ticks in `TG_PrePhysics`/`TG_PostPhysics`.
- `SpringArmComponent.cpp`: spring arm tick group is `TG_PostPhysics`.
- `GameplayAbilityTargetActor_Trace.cpp`: GAS trace target actor tick group is `TG_PostUpdateWork`.

## Remaining Non-Blockers

- ADR-0011 still has a design/tuning open question: whether multiple same-frame damage penalty triggers collapse to one proportional penalty or stack. This is not an engine API blocker.
- Broader GAS items outside ADR-0010/0011 remain separate: legacy attribute initialization, `PostGameplayEffectExecute` / `PreAttributeChange` signatures from ADR-0008, and loose tag full-clear semantics from ADR-0004/0008.
