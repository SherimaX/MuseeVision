#include "VisitorEye.h"

#include "Components/PostProcessComponent.h"
#include "GameFramework/Actor.h"

namespace Musee::Visitor
{
	namespace
	{
		/**
		 * The reflected-light meter equation, EV = log2(L S / K), with the calibration constant K = 12.5 cd s m^-2 at the
		 * arithmetic speed S = 100 (ISO 2720:1974). The engine's meter reads the same way in its extended luminance range.
		 */
		constexpr double MeterCalibration = 12.5;
		constexpr double ReferenceSpeed = 100.0;

		/** The lower bound of mesopic vision, 0.005 cd/m2 (CIE 191:2010): below it the eye sees by rods alone. */
		constexpr double MesopicFloorNits = 0.005;

		/**
		 * The brightest diffuse surface under the sky: a perfect white facing the sun above the atmosphere, E / pi, with
		 * E the luminous solar constant, 133.8 klx (Darula, Kittler and Gueymard, Solar Energy 79, 2005).
		 */
		constexpr double SunlitWhiteNits = 133800.0 / UE_DOUBLE_PI;

		/**
		 * How fast the eye adapts: its neural adaptation moves sensitivity by about two log units within about two
		 * seconds, in either direction (D. C. Hood and M. A. Finkelstein, "Sensitivity to light", Handbook of Perception
		 * and Human Performance, vol. 1, 1986), so 6.6 stops in 2 s. The slower bleaching recovery of the dark-adapting
		 * eye is left out.
		 */
		constexpr double StopsPerLogUnit = 3.3219280948873622; // log2(10)
		constexpr double AdaptedLogUnits = 2.0;
		constexpr double AdaptationSeconds = 2.0;
		constexpr double AdaptationStopsPerSecond = AdaptedLogUnits * StopsPerLogUnit / AdaptationSeconds;

		/**
		 * The meter leaves out the darkest and the brightest tenth of the frame, as a spot-average meter ignores
		 * specular glints and deep shadow: a window's sky or a lamp's face doesn't darken the room it is seen from.
		 */
		constexpr float MeteredFromPercent = 10.0f;
		constexpr float MeteredToPercent = 90.0f;

		/**
		 * The tone curve: the Academy Color Encoding System's reference rendering (ACES 1.0 RRT and sRGB ODT) as the
		 * engine's filmic curve fits it, whose shoulder rolls highlights off towards white rather than clipping them.
		 */
		constexpr float FilmSlope = 0.88f;
		constexpr float FilmToe = 0.55f;
		constexpr float FilmShoulder = 0.26f;
		constexpr float FilmBlackClip = 0.0f;
		constexpr float FilmWhiteClip = 0.04f;
	}
}

FPostProcessSettings UVisitorEye::CameraSettings()
{
	using namespace Musee::Visitor;
	FPostProcessSettings Camera;
	Camera.bOverride_AutoExposureMethod = true;
	Camera.AutoExposureMethod = AEM_Histogram;
	Camera.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	Camera.AutoExposureApplyPhysicalCameraExposure = false;
	Camera.bOverride_AutoExposureBias = true;
	Camera.AutoExposureBias = 0.0f;
	Camera.bOverride_AutoExposureMinBrightness = true;
	Camera.AutoExposureMinBrightness = static_cast<float>(DarkestExposure().Get());
	Camera.bOverride_AutoExposureMaxBrightness = true;
	Camera.AutoExposureMaxBrightness = static_cast<float>(BrightestExposure().Get());
	Camera.bOverride_HistogramLogMin = true;
	Camera.HistogramLogMin = static_cast<float>(DarkestExposure().Get());
	Camera.bOverride_HistogramLogMax = true;
	Camera.HistogramLogMax = static_cast<float>(BrightestExposure().Get());
	Camera.bOverride_AutoExposureSpeedUp = true;
	Camera.AutoExposureSpeedUp = static_cast<float>(AdaptationStopsPerSecond);
	Camera.bOverride_AutoExposureSpeedDown = true;
	Camera.AutoExposureSpeedDown = static_cast<float>(AdaptationStopsPerSecond);
	Camera.bOverride_AutoExposureLowPercent = true;
	Camera.AutoExposureLowPercent = MeteredFromPercent;
	Camera.bOverride_AutoExposureHighPercent = true;
	Camera.AutoExposureHighPercent = MeteredToPercent;
	Camera.bOverride_FilmSlope = true;
	Camera.FilmSlope = FilmSlope;
	Camera.bOverride_FilmToe = true;
	Camera.FilmToe = FilmToe;
	Camera.bOverride_FilmShoulder = true;
	Camera.FilmShoulder = FilmShoulder;
	Camera.bOverride_FilmBlackClip = true;
	Camera.FilmBlackClip = FilmBlackClip;
	Camera.bOverride_FilmWhiteClip = true;
	Camera.FilmWhiteClip = FilmWhiteClip;
	return Camera;
}

void UVisitorEye::OnRegister()
{
	Super::OnRegister();
	AActor* Owner = GetOwner();
	if (EyeVolume || !Owner) { return; }
	EyeVolume = NewObject<UPostProcessComponent>(Owner, NAME_None, RF_Transient);
	EyeVolume->bUnbound = true;
	EyeVolume->Settings = CameraSettings();
	EyeVolume->SetupAttachment(Owner->GetRootComponent());
	EyeVolume->RegisterComponent();
}

void UVisitorEye::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	if (EyeVolume)
	{
		EyeVolume->DestroyComponent();
		EyeVolume = nullptr;
	}
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

Musee::EV100 UVisitorEye::ExposureOf(Musee::Nits Luminance)
{
	using namespace Musee::Visitor;
	return Musee::EV100(FMath::Log2(Luminance.Get() * ReferenceSpeed / MeterCalibration));
}

Musee::EV100 UVisitorEye::DarkestExposure()
{
	return ExposureOf(Musee::Nits(Musee::Visitor::MesopicFloorNits));
}

Musee::EV100 UVisitorEye::BrightestExposure()
{
	return ExposureOf(Musee::Nits(Musee::Visitor::SunlitWhiteNits));
}
