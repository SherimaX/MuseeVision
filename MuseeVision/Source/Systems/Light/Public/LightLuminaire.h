#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"
#include "MuseeUnits.h"

/**
 * A luminaire type of the museum's catalogue, as Light lights it: one real product in one configuration, read from its
 * record in Data/Light/Luminaires (Docs/Light/Luminaires.md). It says how the light leaves the product (a beam from a
 * spot, a line from a strip or slot, a panel's even face, an image panel's picture) and how much light, of what colour.
 * An areal type (a luminous ceiling's modules, edge-lit panes) gives its light per square metre, and the room gives the
 * face it is laid over. A record whose maker
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
		/**
		 * An emissive picture: an image panel whose light is the picture it shows, up to its peak white. It stands at its
		 * black level until an image is shown on it.
		 */
		Image,
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
	/** An areal luminaire's flux per square metre of the face it is laid over, in lumens; zero for any other. */
	double FluxPerSquareMeterLumens = 0.0;
	/** An image panel's luminance at its peak white, in candelas per square metre. */
	double PeakLuminanceCandelaPerSquareMeter = 0.0;
	/** A spot's intensity on its axis; zero where the maker publishes only the flux. */
	Musee::Candela PeakIntensity;
	/**
	 * A spot's beam angle: the full angle at which the intensity falls to half its peak. Where the maker publishes only
	 * the flux and the peak (a step light), it is the beam of the Gaussian distribution those two fix.
	 */
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

	/** Whether the room gives the face it lights (an extent or a zone), as for a luminous ceiling or an image panel. */
	[[nodiscard]] bool IsAreal() const
	{
		return Emitter == EEmitter::Image || (Emitter == EEmitter::Rect && FluxPerSquareMeterLumens > 0.0);
	}
};

namespace Musee::Light
{
	/** The catalogue type with this key, or null. The catalogue is read once, on first use. */
	MUSEELIGHT_API const FLightLuminaire* FindLuminaire(const FMuseeKey& Key);

	/** Every catalogue type's key, in key order. */
	MUSEELIGHT_API TArray<FMuseeKey> LuminaireKeys();
}
