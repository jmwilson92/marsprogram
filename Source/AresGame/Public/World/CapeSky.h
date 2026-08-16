// Sky, sun, clouds, fog and colour grading, in one actor.
//
// Drop it in a level and the lighting is done. This exists because lighting is
// the largest visual lever that lives in CODE rather than in binary assets —
// meshes and textures have to be authored, but atmosphere, exposure and grading
// are numbers, and they change how everything else reads more than any single
// mesh does.
//
// Tuned stylized rather than photoreal: saturated, high-contrast, strong sun
// with a warm key and a cool sky fill, bloom carried deliberately high. That is
// a deliberate overrule of the brief's original "grounded, NASA-flight-
// controller" tone, at the project owner's direction.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "CapeSky.generated.h"

class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class UPostProcessComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UVolumetricCloudComponent;

UCLASS()
class ARESGAME_API ACapeSky : public AActor
{
	GENERATED_BODY()

public:
	ACapeSky();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Re-applies sun angle, colours and grading. Called on any property change. */
	UFUNCTION(BlueprintCallable, Category = "Ares|Sky")
	void ApplyLookSettings();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Ares|Sky")
	TObjectPtr<UDirectionalLightComponent> Sun;

	UPROPERTY(VisibleAnywhere, Category = "Ares|Sky")
	TObjectPtr<USkyAtmosphereComponent> Atmosphere;

	UPROPERTY(VisibleAnywhere, Category = "Ares|Sky")
	TObjectPtr<USkyLightComponent> SkyLight;

	UPROPERTY(VisibleAnywhere, Category = "Ares|Sky")
	TObjectPtr<UExponentialHeightFogComponent> HeightFog;

	UPROPERTY(VisibleAnywhere, Category = "Ares|Sky")
	TObjectPtr<UVolumetricCloudComponent> Clouds;

	UPROPERTY(VisibleAnywhere, Category = "Ares|Sky")
	TObjectPtr<UPostProcessComponent> PostProcess;

	/* --- sun ------------------------------------------------------------- */

	/** Degrees above the horizon. Low sun gives long shadows and warm light. */
	UPROPERTY(EditAnywhere, Category = "Ares|Sky|Sun", meta = (ClampMin = "-10", ClampMax = "89"))
	float SunElevationDegrees = 38.0f;

	UPROPERTY(EditAnywhere, Category = "Ares|Sky|Sun", meta = (ClampMin = "0", ClampMax = "360"))
	float SunAzimuthDegrees = 125.0f;

	UPROPERTY(EditAnywhere, Category = "Ares|Sky|Sun")
	float SunIntensityLux = 9.0f;

	UPROPERTY(EditAnywhere, Category = "Ares|Sky|Sun")
	FLinearColor SunColor = FLinearColor(1.0f, 0.92f, 0.78f);

	/**
	 * Angular size of the sun, degrees. Widening it softens shadow edges, which
	 * is a large part of why stylized renders look less harsh than photoreal.
	 */
	UPROPERTY(EditAnywhere, Category = "Ares|Sky|Sun")
	float SunAngularDiameter = 1.6f;

	/* --- grading --------------------------------------------------------- */

	/** Above 1 pushes colour. The single biggest knob on "stylized". */
	UPROPERTY(EditAnywhere, Category = "Ares|Sky|Look")
	float Saturation = 1.28f;

	UPROPERTY(EditAnywhere, Category = "Ares|Sky|Look")
	float Contrast = 1.1f;

	UPROPERTY(EditAnywhere, Category = "Ares|Sky|Look")
	float BloomIntensity = 1.35f;

	UPROPERTY(EditAnywhere, Category = "Ares|Sky|Look")
	float VignetteIntensity = 0.32f;

	/**
	 * Locks exposure. Auto-exposure hunting as you walk from bright sun into a
	 * dark VAB reads as a bug, and it makes screenshots impossible to compare.
	 */
	UPROPERTY(EditAnywhere, Category = "Ares|Sky|Look")
	bool bLockExposure = true;

	UPROPERTY(EditAnywhere, Category = "Ares|Sky|Look")
	float ExposureBias = 1.0f;

	/* --- atmosphere ------------------------------------------------------ */

	UPROPERTY(EditAnywhere, Category = "Ares|Sky|Atmosphere")
	bool bVolumetricClouds = true;

	/** Haze near the ground. A little separates the pad from the horizon. */
	UPROPERTY(EditAnywhere, Category = "Ares|Sky|Atmosphere")
	float FogDensity = 0.012f;

	UPROPERTY(EditAnywhere, Category = "Ares|Sky|Atmosphere")
	FLinearColor FogColor = FLinearColor(0.42f, 0.58f, 0.78f);

	/** Bounce light from the sky. Fills shadows so interiors are not black. */
	UPROPERTY(EditAnywhere, Category = "Ares|Sky|Atmosphere")
	float SkyLightIntensity = 1.6f;
};
