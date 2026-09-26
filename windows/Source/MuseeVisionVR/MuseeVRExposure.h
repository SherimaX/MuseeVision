#pragma once

#include "CoreMinimal.h"
#include "SceneView.h"
#include "SceneViewExtension.h"

/**
 * The view's auto-exposure, for the signs in the room. Widgets in the world are unlit emissive, so
 * the museum's exposure (daylight metering, EV100 12–15) would darken them to near black; the
 * visitor tints them by the inverse so they read as they do on the desktop overlay.
 */
class FMuseeVRExposure : public FSceneViewExtensionBase
{
public:
	FMuseeVRExposure(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister) {}

	/** The last frame's exposure scale (1: none). */
	float Exposure() const { return LastExposure; }

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override
	{
		if (InViewFamily.Views.Num() > 0 && InViewFamily.Views[0])
		{
			const float Value = InViewFamily.Views[0]->GetLastEyeAdaptationExposure();
			if (Value > 0.f && FMath::IsFinite(Value)) { LastExposure = Value; }
		}
	}

private:
	float LastExposure = 1.f;
};
