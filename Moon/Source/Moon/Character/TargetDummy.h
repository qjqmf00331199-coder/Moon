#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "TargetDummy.generated.h"

class UAbilitySystemComponent;
class UMoonAttributeSet;
class UGameplayEffect;
class UCapsuleComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class AMoonCharacterBase;
class AMoonFracturePillar;

UENUM(BlueprintType)
enum class EMoonSignatureChainState : uint8
{
	Idle,
	Gathered,
	Executable,
	Extracted
};

/**
 * Minimal target actor for GAS smoke-testing (e.g. PIE-testing the Blackhole spell end-to-end
 * via unreal-mcp). Not a real enemy: no mesh, animation, or AI — just a capsule + Ability System
 * Component + AttributeSet so it can receive damage through the same Gameplay Effect pipeline as
 * any other combatant (Health/Damage Core Rule 1: single GE entry point).
 *
 * This is a throwaway test actor. It deliberately duplicates AMoonCharacterBase's
 * ASC/AttributeSet/DefaultAttributesEffect setup pattern rather than sharing code with it, since
 * refactoring MoonCharacterBase to share this is out of scope for this task.
 */
UCLASS()
class MOON_API ATargetDummy : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ATargetDummy();

	// IAbilitySystemInterface implementation
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** Engine-spike hook: Blackhole establishes the causal setup for the signature chain. */
	void ApplyBlackholeSetup();

	/** Returns true only when Fire consumes a Blackhole setup and fractures a nearby pillar. */
	bool ApplyFireDetonation(AMoonFracturePillar* Pillar);

	/** Core extraction is legal only after the environment-linked Fire detonation. */
	bool TryExtractCore(AMoonCharacterBase* ExecutingCharacter);

	bool IsExecutable() const { return SignatureChainState == EMoonSignatureChainState::Executable; }
	EMoonSignatureChainState GetSignatureChainState() const { return SignatureChainState; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moon|Dummy", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moon|Dummy", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moon|Dummy", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPointLightComponent> StateLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moon|Dummy", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moon|Dummy", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMoonAttributeSet> AttributeSet;

	// Default attributes initialization effect — same GE-driven pattern as
	// AMoonCharacterBase::InitializeAttributes (current 5.8 GAS attribute-init pattern; not
	// hardcoded in a constructor). If left unset, the AttributeSet's own constructor defaults
	// (Health/MaxHealth = 100) apply, which is sufficient for this smoke test.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moon|Dummy")
	TSubclassOf<UGameplayEffect> DefaultAttributesEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moon|Signature Chain|Spike", meta = (ClampMin = "0.0"))
	float ExecutionTensionReward = 20.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Moon|Signature Chain|Spike")
	EMoonSignatureChainState SignatureChainState = EMoonSignatureChainState::Idle;

	void InitializeAttributes();
};
