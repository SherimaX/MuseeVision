#pragma once

#include "CoreMinimal.h"

class SWidget;
class UWidgetComponent;

/**
 * The overlay's signage (UI/MuseeStyle) as things in the room: Slate drawn into widget components
 * at twice its size, so it stays sharp in the headset. The glass is a denser smoked bronze, since
 * there is no view behind a panel to blur.
 */
namespace MuseeVRStyle
{
	/** Pixels per Slate unit in the widget textures. */
	inline constexpr float Dpi = 2.f;

	/** The component scale at which one Slate unit spans the same angle (≈ 0.07°) at any distance (cm). */
	inline float UnitCm(float DistanceCm) { return 0.0006f * DistanceCm; }

	/** A world-space panel: transparent, sized to its content, pivoted at its centre, never collides. */
	void SetUpPanel(UWidgetComponent& Panel);

	/** Content drawn at Dpi pixels per unit. */
	TSharedRef<SWidget> Scaled(const TSharedRef<SWidget>& Content, float Scale = Dpi);

	/** The key shown on signs in the headset: the pinch. */
	FText PinchKey();

	/** A sign: a keycap and a line of caps on smoked bronze with a gilt hairline. */
	TSharedRef<SWidget> Sign(const FText& Key, const FText& Title);

	/** Smoked bronze glass with a gilt hairline, round Content. */
	TSharedRef<SWidget> Glass(const TSharedRef<SWidget>& Content, float Radius, const FMargin& Padding);
}
