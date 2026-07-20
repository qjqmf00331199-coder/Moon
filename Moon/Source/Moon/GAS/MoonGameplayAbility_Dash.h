#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "MoonGameplayAbility_Dash.generated.h"

/**
 * Dash & Evasion Ability
 * Implements Dash charges, impulse override, and i-frame application.
 */
UCLASS()
class MOON_API UMoonGameplayAbility_Dash : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UMoonGameplayAbility_Dash();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// Tuning parameters from GDD
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	float DashSpeedMultiplier = 6.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	float DashDuration = 0.08f;

	// Dash motion, played on the character mesh via AMoonCharacterBase::PlayOneShotAnim.
	// Defaults to the Aurora "Jog_Fwd_Start" animation (project has no dedicated Dash anim yet).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash|Animation")
	TObjectPtr<class UAnimSequence> DashAnim;

	// Playback speed multiplier for DashAnim. Jog_Fwd_Start is 0.3s at 1x; 3.75x brings it to
	// 0.08s, matching DashDuration exactly.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash|Animation")
	float DashAnimPlayRate = 3.75f;

	// Dash VFX — Aurora's own (unused) dash particle set. A borrowed body-pose animation can't
	// carry a dash's "impact" on its own; most action games (Genshin, Warframe, etc.) lean on
	// burst/trail VFX for that read regardless of the underlying animation.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash|VFX")
	TObjectPtr<class UParticleSystem> DashWarmUpFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash|VFX")
	TObjectPtr<class UParticleSystem> DashTrailFX;

	// The trail FX component, kept alive for DashDuration then deactivated in OnDashFinished.
	TWeakObjectPtr<class UParticleSystemComponent> ActiveDashTrailComponent;

	// Timer handle for ending the dash
	FTimerHandle DashTimerHandle;

	// Ends the dash after duration
	void OnDashFinished();

	// Instantly repositions the character by the configured dash distance, using a swept
	// collision move. Returns the world-space dash direction used, so callers (VFX) can
	// orient to the actual dash direction rather than the mesh's facing.
	FVector ApplyDashImpulse(class ACharacter* Character);

	// Movement is disabled only while the dash animation/i-frame window completes, preventing
	// held movement input from turning an instant step back into a forward slide.
	bool bMovementLockedByDash = false;
	bool bRestoreFallingMovement = false;
	void RestoreCharacterMovement(class ACharacter* Character);

	// Core logic for checking Just-Dodge window
	// Will be fully implemented when Enemy AI is ready
	void CheckJustDodge(class ACharacter* PlayerCharacter);
};
