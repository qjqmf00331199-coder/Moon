#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MoonCameraSettings.generated.h"

/**
 * Designer-authored source of truth for the player follow-camera tuning values.
 *
 * Create a Data Asset whose class is MoonCameraSettings, then assign it to
 * AMoonCharacterBase::CameraSettings. The character validates and applies the asset once in
 * BeginPlay; invalid or missing assets fall back to this class's documented safe defaults.
 *
 * C++ validation example:
 * @code
 * FString FailureReason;
 * if (!Settings->IsWithinSafeRanges(FailureReason))
 * {
 *     UE_LOG(LogTemp, Error, TEXT("Invalid camera settings: %s"), *FailureReason);
 * }
 * @endcode
 *
 * Thread safety: game-thread only. UDataAsset instances and camera components are Unreal
 * objects and must not be read or mutated from worker threads without external synchronization.
 */
UCLASS(BlueprintType)
class MOON_API UMoonCameraSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Natural third-person boom length in Unreal units. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moon|Camera", meta = (ClampMin = "250.0", ClampMax = "500.0", UIMin = "250.0", UIMax = "500.0", Units = "cm"))
	float TargetArmLength = 450.0f;

	/** Relative camera offset at the end of the boom. X is fixed at zero by the camera contract. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moon|Camera")
	FVector CameraSocketOffset = FVector(0.0f, 45.0f, 20.0f);

	/** Minimum allowed controller view pitch, applied by the camera-manager story. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moon|Camera", meta = (ClampMin = "-80.0", ClampMax = "-45.0", UIMin = "-80.0", UIMax = "-45.0", Units = "deg"))
	float CameraPitchMin = -60.0f;

	/** Maximum allowed controller view pitch, applied by the camera-manager story. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moon|Camera", meta = (ClampMin = "15.0", ClampMax = "45.0", UIMin = "15.0", UIMax = "45.0", Units = "deg"))
	float CameraPitchMax = 30.0f;

	/** Positional SpringArm lag speed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moon|Camera", meta = (ClampMin = "5.0", ClampMax = "20.0", UIMin = "5.0", UIMax = "20.0"))
	float CameraLagSpeed = 18.0f;

	/** Rotation-lag speed retained for future tuning; rotation lag stays disabled for responsive aim. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moon|Camera", meta = (ClampMin = "8.0", ClampMax = "25.0", UIMin = "8.0", UIMax = "25.0"))
	float CameraRotationLagSpeed = 15.0f;

	/** Maximum distance the lagged camera target may trail the player. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moon|Camera", meta = (ClampMin = "40.0", ClampMax = "120.0", UIMin = "40.0", UIMax = "120.0", Units = "cm"))
	float CameraLagMaxDistance = 60.0f;

	/** Normal gameplay field of view. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moon|Camera", meta = (ClampMin = "80.0", ClampMax = "100.0", UIMin = "80.0", UIMax = "100.0", Units = "deg"))
	float BaseFOV = 90.0f;

	/** Overdrive target field of view, consumed by the later dynamic-FOV story. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moon|Camera", meta = (ClampMin = "95.0", ClampMax = "110.0", UIMin = "95.0", UIMax = "110.0", Units = "deg"))
	float OverdriveFOV = 100.0f;

	/** Execution target boom length, consumed by the later execution-blend story. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moon|Camera", meta = (ClampMin = "100.0", ClampMax = "250.0", UIMin = "100.0", UIMax = "250.0", Units = "cm"))
	float ExecutionArmLength = 150.0f;

	/** SpringArm collision probe radius. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moon|Camera", meta = (ClampMin = "5.0", ClampMax = "25.0", UIMin = "5.0", UIMax = "25.0", Units = "cm"))
	float CameraProbeSize = 12.0f;

	/**
	 * Returns whether every value obeys the approved GDD safe range.
	 *
	 * Example: `Settings->IsWithinSafeRanges(FailureReason)` before applying the asset.
	 * This method performs no allocation on the success path beyond retaining the caller-owned
	 * string and is intended for one-time initialization, not a hot path.
	 */
	bool IsWithinSafeRanges(FString& OutFailureReason) const;
};
