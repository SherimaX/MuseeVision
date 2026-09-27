#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"
#include "MuseeUnits.h"

/**
 * A luminaire type of the museum's catalogue, as Light lights it: one real product in one configuration, read from its
 * record in Data/Light/Luminaires (Docs/Light/Luminaires.md). It says how the light leaves the product (a beam from a
 * spot, a line from a strip or slot, a panel's even face) and how much light, of what colour. A record whose maker
 * leaves a number the emitter needs unpublished is a real object that isn't lit, and says why. A lamp luminaire (a
 * lantern's filament lamp, an electrolier's lamps) is its bare lamps: the shade or globe around them is the stage's own
 * piece, in Matter's glass or silk, and passes their light on.
 */
struct MUSEELIGHT_API FLightLuminaire
{
	/** How the light leaves the luminaire. */
	enum class EEmitter : uint8
	{
		/** A beam about the luminaire's axis: a spotlight or a downlight. */
		Spot,
		/** An even, near-Lambertian face of a given size: a luminous panel. */
		Rect,
		/** A line of near-Lambertian emitters, its length set where it is laid: a strip, a cove line, a slot. */
		Linear,
		/** Lamps that light all round them: a filament lamp in a lantern, the lamps of an electrolier on its ring. */
		Point,
	};

	FMuseeKey Key;
	/** The record's kind (`track-spot`, `linear-strip`). */
	FName Kind;
	EEmitter Emitter = EEmitter::Spot;
	/** The control class Light's operation uses (`gallery`, `circulation`). */
	FName ControlClass;

	/** Whether Light lights it; if not, why not. */
	bool bLit = false;
	FString WhyUnlit;

	/** A spot's or panel's luminous flux: the light that leaves the housing. */
	Musee::Lumens Flux;
	/** A linear luminaire's flux per metre of its length, in lumens. */
	double FluxPerMeterLumens = 0.0;
	/** A spot's intensity on its axis; zero where the maker publishes only the flux. */
	Musee::Candela PeakIntensity;
	/** A spot's beam angle: the full angle at which the intensity falls to half its peak. */
	Musee::Degrees BeamAngle;
	/** The correlated colour temperature of the light. */
	Musee::Kelvin ColourTemperature;
	/** The radius of the emitting face: half the housing's diameter, for a spot. */
	Musee::Meters SourceRadius;
	/** A linear luminaire's emitting width across its length; a panel's emitting width and height. */
	Musee::Meters EmittingWidth;
	Musee::Meters EmittingHeight;
	/** A lamp luminaire's lamps, which share its flux equally, and the radius of the ring they stand on about its axis
	 *  (zero for a single lamp on the axis). */
	int32 LampCount = 1;
	Musee::Meters LampRingRadius;
};

namespace Musee::Light
{
	/** The catalogue type with this key, or null. The catalogue is read once, on first use. */
	MUSEELIGHT_API const FLightLuminaire* FindLuminaire(const FMuseeKey& Key);

	/** Every catalogue type's key, in key order. */
	MUSEELIGHT_API TArray<FMuseeKey> LuminaireKeys();
}
