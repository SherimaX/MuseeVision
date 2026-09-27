#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"
#include "MuseeUnits.h"

class UTextureLightProfile;

/**
 * A luminaire type of the museum's catalogue, as Light lights it: one real product in one configuration, read from its
 * record in Data/Light/Luminaires (Docs/Light/Luminaires.md). It says how the light leaves the product (a beam from a
 * spot, a line from a strip or slot, a panel's even face, an image panel's picture) and how much light, of what colour.
 * An areal type (a luminous ceiling's modules, edge-lit panes) gives its light per square metre, and the room gives the
 * face it is laid over. A record whose maker
 * leaves a number the emitter needs unpublished is a real object that isn't lit, and says why. A lamp luminaire (a
 * lantern's filament lamp, an electrolier's lamps) is its bare lamps: the shade or globe around them is the stage's own
 * piece, in Matter's glass or silk, and passes their light on. A type whose light only its maker's measurement describes
 * (a wallwasher's asymmetric lens) is lit by its photometric file, kept in Data/Light/Photometry: the file's candela
 * table as the engine's light profile, which Studio imports from the file.
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
		/** Lamps that light all round them: a filament lamp in a lantern, the lamps of an electrolier on its ring, the lamp
	 *  in a post-top lantern's globe. */
		Point,
		/**
		 * An emissive picture: an image panel whose light is the picture it shows, up to its peak white. It stands at its
		 * black level until an image is shown on it.
		 */
		Image,
		/**
		 * The maker's measured distribution: a source the size of the luminous opening whose intensity in every direction
		 * is its photometric file's candela table, rendered through the light profile Studio imports from the file.
		 */
		Profile,
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

	/**
	 * A spot's or panel's luminous flux: the light that leaves the housing. For a type lit by its photometric file, the
	 * flux the file's candela table integrates to, which must agree with its record's to within 1 %.
	 */
	Musee::Lumens Flux;
	/** A linear luminaire's flux per metre of its length, in lumens. */
	double FluxPerMeterLumens = 0.0;
	/** An areal luminaire's flux per square metre of the face it is laid over, in lumens; zero for any other. */
	double FluxPerSquareMeterLumens = 0.0;
	/** An image panel's luminance at its peak white, in candelas per square metre. */
	double PeakLuminanceCandelaPerSquareMeter = 0.0;
	/**
	 * A spot's intensity on its axis, zero where the maker publishes only the flux; for a type lit by its photometric
	 * file, the greatest intensity in the file's table.
	 */
	Musee::Candela PeakIntensity;
	/**
	 * For a type lit by its photometric file, the direction of that greatest intensity in the file's Type C frame: its
	 * vertical angle from the nadir and its horizontal angle. It is the luminaire's throw, which Light carries onto the
	 * schedule's aim; zero for any other type, whose peak lies on its axis.
	 */
	Musee::Degrees PeakVertical;
	Musee::Degrees PeakHorizontal;
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

	/**
	 * Whether the room gives the face it lights (an extent or a zone), as for a luminous ceiling, edge-lit panes or an
	 * image panel. The kind decides it, so panes whose maker publishes no photometry are still laid over their face, and
	 * stand there unlit, saying why.
	 */
	bool bAreal = false;

	/** The full path of the maker's photometric file it is lit by, in Data/Light/Photometry; empty for any other type. */
	FString PhotometricFile;

	[[nodiscard]] bool IsAreal() const { return bAreal; }
};

namespace Musee::Light
{
	/** The catalogue type with this key, or null. The catalogue is read once, on first use. */
	MUSEELIGHT_API const FLightLuminaire* FindLuminaire(const FMuseeKey& Key);

	/** Every catalogue type's key, in key order. */
	MUSEELIGHT_API TArray<FMuseeKey> LuminaireKeys();

	/** The folder the museum's light profiles are made in, one for each photometric file. */
	inline constexpr const TCHAR* ProfilesFolder = TEXT("/Game/Generated/Light/Profiles");

	/**
	 * The name of the light profile made from a type's photometric file: IES_ and the file's name, its dashes and dots
	 * made underscores (iguzzini-362G_O25F.ies: IES_iguzzini_362G_O25F). Empty for a type with no file.
	 */
	MUSEELIGHT_API FString ProfileName(const FLightLuminaire& Type);

	/** The package of that profile in the museum, <ProfilesFolder>/<ProfileName>; empty for a type with no file. */
	MUSEELIGHT_API FString ProfilePackage(const FLightLuminaire& Type);

	/** The profile the type is lit by, in memory or on disk, or null while Studio hasn't made it. */
	MUSEELIGHT_API UTextureLightProfile* FindProfile(const FLightLuminaire& Type);
}
