#pragma once

#include "CoreMinimal.h"

#include "SiteIris.generated.h"

/**
 * The iris that closes a vertical port: a camera's diaphragm laid in the port's plane, as the Élan kit's irises are
 * built. Each blade is an arc of the ring's middle circle with round ends, turning on a pin outside that circle; open,
 * the blades lie in the ring round the well, and shut, each has turned SwingDegrees on its pin and the blades meet in a
 * spiral over the well. The blades are stacked at a pitch, the top blade's face on the closure's plane. Read with its port
 * class from Data/Site/PortClasses.json.
 */
USTRUCT()
struct MUSEESITE_API FSiteIris
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	int32 Blades = 0;

	/** The ring's middle circle, which every blade's centre line follows open, metres. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double MidRadiusMeters = 0.0;

	/** A blade's width across its arc, metres; its ends are half-circles of this diameter. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double WidthMeters = 0.0;

	/** The arc of the middle circle a blade spans, degrees. */
	UPROPERTY(meta = (MuseeRequired, Units = "Degrees"))
	double ArcDegrees = 0.0;

	/** How far outside the middle circle a blade's pin stands, at its first end, metres. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double PinOutsetMeters = 0.0;

	/** How far a blade turns on its pin from open to shut, degrees. */
	UPROPERTY(meta = (MuseeRequired, Units = "Degrees"))
	double SwingDegrees = 0.0;

	/** A blade's thickness, and the pitch the blades are stacked at, metres. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double BladeMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double PitchMeters = 0.0;

	/** Whether the stack's blades are apart: each thinner than the pitch. */
	[[nodiscard]] bool IsStacked() const { return Blades > 0 && BladeMeters > 0.0 && BladeMeters < PitchMeters; }
};
