#include "World/CapeSky.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/VolumetricCloudComponent.h"

ACapeSky::ACapeSky()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(Root);
	// Movable throughout: nothing here is baked, so the campus needs no lighting
	// build and the sun can be dragged around while the editor is open.
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->bAtmosphereSunLight = true;
	Sun->DynamicShadowDistanceMovableLight = 40000.0f;
	Sun->bUseTemperature = true;
	Sun->Temperature = 5900.0f;

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmosphere->SetupAttachment(Root);

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(Root);
	SkyLight->SetMobility(EComponentMobility::Movable);
	// Real-time capture means the sky light follows the atmosphere, so moving
	// the sun re-lights the shadows instead of leaving a stale bake behind.
	SkyLight->bRealTimeCapture = true;

	HeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFog"));
	HeightFog->SetupAttachment(Root);

	Clouds = CreateDefaultSubobject<UVolumetricCloudComponent>(TEXT("Clouds"));
	Clouds->SetupAttachment(Root);

	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcess->SetupAttachment(Root);
	// Unbound: one grading setup for the whole level rather than a volume you
	// can walk out of.
	PostProcess->bUnbound = true;
	PostProcess->BlendWeight = 1.0f;
}

void ACapeSky::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyLookSettings();
}

void ACapeSky::ApplyLookSettings()
{
	/* --- sun ------------------------------------------------------------- */
	if (Sun)
	{
		// Pitch is negative-down in UE, so a positive elevation points the light
		// downward from that height in the sky.
		Sun->SetWorldRotation(FRotator(-SunElevationDegrees, SunAzimuthDegrees, 0.0f));
		Sun->SetIntensity(SunIntensityLux);
		Sun->SetLightColor(SunColor);
		Sun->LightSourceAngle = SunAngularDiameter;
		Sun->MarkRenderStateDirty();
	}

	/* --- sky light ------------------------------------------------------- */
	if (SkyLight)
	{
		SkyLight->SetIntensity(SkyLightIntensity);
		SkyLight->SetLightColor(FLinearColor::White);
		SkyLight->RecaptureSky();
	}

	/* --- fog ------------------------------------------------------------- */
	if (HeightFog)
	{
		HeightFog->SetVisibility(bHeightFog);
		HeightFog->SetFogDensity(FogDensity);
		HeightFog->SetFogInscatteringColor(FogColor);
		HeightFog->SetFogHeightFalloff(FogHeightFalloff);
		HeightFog->SetStartDistance(FogStartDistanceM * 100.0f);

		// Volumetric fog is what makes sun shafts through the VAB doorway read,
		// but it hazes the whole scene to get there and is not cheap.
		HeightFog->SetVolumetricFog(bVolumetricFog);
		HeightFog->SetVolumetricFogExtinctionScale(0.25f);
	}

	/* --- clouds ---------------------------------------------------------- */
	if (Clouds)
	{
		Clouds->SetVisibility(bVolumetricClouds);
		Clouds->LayerBottomAltitude = 4.0f;
		Clouds->LayerHeight = 6.0f;
		Clouds->MarkRenderStateDirty();
	}

	/* --- grading --------------------------------------------------------- */
	if (!PostProcess)
	{
		return;
	}

	FPostProcessSettings& PP = PostProcess->Settings;

	// Saturation and contrast carry most of the stylized read. Anything much
	// above 1.35 saturation starts clipping skin and sky into poster colours.
	PP.bOverride_ColorSaturation = true;
	PP.ColorSaturation = FVector4(Saturation, Saturation, Saturation, 1.0f);

	PP.bOverride_ColorContrast = true;
	PP.ColorContrast = FVector4(Contrast, Contrast, Contrast, 1.0f);

	// A touch of warmth in the highlights and cool in the shadows — the split
	// that makes stylized renders feel lit rather than flat.
	PP.bOverride_ColorGain = true;
	PP.ColorGain = FVector4(1.02f, 1.0f, 0.97f, 1.0f);
	PP.bOverride_ColorOffset = true;
	PP.ColorOffset = FVector4(-0.005f, 0.0f, 0.012f, 0.0f);

	PP.bOverride_BloomIntensity = true;
	PP.BloomIntensity = BloomIntensity;
	PP.bOverride_BloomThreshold = true;
	PP.BloomThreshold = -0.2f;

	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = VignetteIntensity;

	// Exposure. These values are EV100 because the project sets
	// ExtendDefaultLuminanceRange (see DefaultEngine.ini) — without that they
	// would be raw luminance and every number here would mean something else.
	PP.bOverride_AutoExposureBias = true;
	PP.AutoExposureBias = ExposureCompensation;

	// The physical camera model would multiply another aperture/ISO term on top
	// of the lock, so the locked value would not be the value you get.
	PP.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	PP.AutoExposureApplyPhysicalCameraExposure = 0;

	if (bLockExposure)
	{
		// Pinning min and max together is the reliable way to defeat
		// auto-exposure. Walking from Florida sun into the VAB should go dark,
		// because it IS dark — the camera hunting to compensate reads as a bug.
		PP.bOverride_AutoExposureMinBrightness = true;
		PP.bOverride_AutoExposureMaxBrightness = true;
		PP.AutoExposureMinBrightness = LockedExposureEV100;
		PP.AutoExposureMaxBrightness = LockedExposureEV100;
	}
	else
	{
		// Auto, but bounded, so it cannot hunt to either extreme.
		PP.bOverride_AutoExposureMinBrightness = true;
		PP.bOverride_AutoExposureMaxBrightness = true;
		PP.AutoExposureMinBrightness = 6.0f;
		PP.AutoExposureMaxBrightness = 16.0f;
		PP.bOverride_AutoExposureSpeedUp = true;
		PP.AutoExposureSpeedUp = 2.0f;
		PP.bOverride_AutoExposureSpeedDown = true;
		PP.AutoExposureSpeedDown = 1.0f;
	}

	// Lumen, explicitly, so the look does not depend on project defaults.
	PP.bOverride_DynamicGlobalIlluminationMethod = true;
	PP.DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::Lumen;
	PP.bOverride_ReflectionMethod = true;
	PP.ReflectionMethod = EReflectionMethod::Lumen;

	// Quality up from the defaults; the campus is small enough to afford it.
	PP.bOverride_LumenSceneLightingQuality = true;
	PP.LumenSceneLightingQuality = 2.0f;
	PP.bOverride_LumenFinalGatherQuality = true;
	PP.LumenFinalGatherQuality = 2.0f;

	PostProcess->MarkRenderStateDirty();
}
