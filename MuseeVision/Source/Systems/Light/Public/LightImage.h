#pragma once

#include "CoreMinimal.h"
#include "Templates/ValueOrError.h"

class UMaterialInterface;
struct FTimeMoment;

/**
 * Light's celestial image source: the picture an image surface that encloses its luminaire shows (the Sphere's, sphere.md
 * section 4), the real sky over the museum's city now, from Time's moment. The stars to V 6.5 at their true places,
 * brightnesses and colours (the Yale Bright Star Catalogue, the collection's assets/sky/stars.bin), drawn once into a star
 * field in the J2000 equatorial frame; the Moon at its true size, place and phase; no Sun and no atmosphere. The field is
 * turned to the sky of the moment, and the picture is shown as a light field: each point of the surface shows the sky
 * in the direction it is seen along, content at infinity, so the stars hold still as the eye moves. Every value is a
 * luminance in candelas per square metre, clipped at the panel's peak white. Light defines the picture; Studio makes its
 * star field and its material (M_Light_Image) in Folder, where Light finds them.
 */
namespace Musee::Light::Image
{
	/** Where Studio makes the image's assets for the museum, and their names. */
	inline constexpr const TCHAR* Folder = TEXT("/Game/Generated/Light");
	inline constexpr const TCHAR* MaterialName = TEXT("M_Light_Image");
	inline constexpr const TCHAR* StarFieldName = TEXT("T_Light_StarField");

	/** The star catalogue, a path in the collection (assets/sky/CREDITS.md gives its format and source). */
	inline constexpr const TCHAR* CatalogueSource = TEXT("assets/sky/stars.bin");

	/** The star field's texels: 4096 by 2048 over the whole sky, about 0.09 degrees each, finer than the eye's 0.05 at the Sphere's centre by less than two. */
	inline constexpr int32 StarFieldWidth = 4096;
	inline constexpr int32 StarFieldHeight = 2048;

	/** The faintest star drawn, the naked eye's limit (sphere.md, section 4). */
	inline constexpr double FaintestMagnitude = 6.5;

	/**
	 * The star field's mean luminance over the whole sky: the catalogue's 8,404 stars to V 6.5, their illuminances summed
	 * (StarIlluminanceLux) over 4 pi steradians. Musee.Studio.LightImage checks it against the catalogue.
	 */
	inline constexpr double StarFieldMeanLuminance = 1.5652e-5;

	/** The image material's parameters, which Light sets and Studio's material reads. */
	namespace Parameter
	{
		/** The star field, a texture of luminances over right ascension (across, from 0 h) and declination (down, from +90). */
		inline constexpr const TCHAR* StarField = TEXT("StarField");
		/** The rows of the turn from the world into the J2000 equatorial frame. */
		inline constexpr const TCHAR* SkyRowX = TEXT("SkyRowX");
		inline constexpr const TCHAR* SkyRowY = TEXT("SkyRowY");
		inline constexpr const TCHAR* SkyRowZ = TEXT("SkyRowZ");
		/** The unit vector towards the Moon, and the one from the Moon towards the Sun, both in the world. */
		inline constexpr const TCHAR* MoonDirection = TEXT("MoonDirection");
		inline constexpr const TCHAR* MoonSunward = TEXT("MoonSunward");
		/** The cosine of the Moon's angular radius, and the luminance of its lit face. */
		inline constexpr const TCHAR* MoonCosRadius = TEXT("MoonCosRadius");
		inline constexpr const TCHAR* MoonLuminance = TEXT("MoonLuminance");
		/** The panel's peak white, and the level it is run at (its switch times its channel). */
		inline constexpr const TCHAR* PeakLuminance = TEXT("PeakLuminance");
		inline constexpr const TCHAR* Level = TEXT("Level");
	}

	/** A star field drawn from the catalogue: its texels' luminances (linear Rec. 709, D65), row by row from the north. */
	struct FStarField
	{
		int32 Width = 0;
		int32 Height = 0;
		TArray<FLinearColor> Luminance;
		/** The stars drawn, and the field's mean luminance over the whole sky. */
		int32 Stars = 0;
		double MeanLuminance = 0.0;
	};

	/** The illuminance a star of a visual magnitude gives, above the atmosphere: 2.08 microlux at V 0, 2.512 times less a magnitude. */
	MUSEELIGHT_API double StarIlluminanceLux(double VisualMagnitude);

	/**
	 * A star's colour from its B-V index, of unit luminance: its temperature by Ballesteros's formula (EPL 97 (2012)
	 * 34008), as the light of a black body of that temperature, in linear Rec. 709 under D65.
	 */
	MUSEELIGHT_API FLinearColor StarColour(double BMinusV);

	/**
	 * Draws the catalogue's stars to FaintestMagnitude into a field of Width by Height texels: each star's illuminance
	 * shared between the four texels about its place, each share over that texel's solid angle, so the field's
	 * luminance integrates to the stars' illuminances. An error when the catalogue isn't in its format.
	 */
	MUSEELIGHT_API TValueOrError<FStarField, FString> DrawStarField(TConstArrayView<uint8> Catalogue, int32 Width, int32 Height);

	/**
	 * The image's emission, the HLSL of the material's custom node: from the view direction (ToEye, the engine's camera
	 * vector) and the parameters, the luminance the surface shows, the stars' or the Moon's, clipped at the peak white
	 * and run at its level.
	 */
	MUSEELIGHT_API const TCHAR* EmissionHlsl();

	/** The image material's package in the museum, <Folder>/<MaterialName>. */
	MUSEELIGHT_API FString MaterialPackage();

	/** The image material, in memory or on disk, or null while Studio hasn't made it. */
	MUSEELIGHT_API UMaterialInterface* FindMaterial();

	/** The sky of a moment, as the image shows it in the world. */
	struct FSkyPicture
	{
		FVector SkyRowX = FVector::XAxisVector;
		FVector SkyRowY = FVector::YAxisVector;
		FVector SkyRowZ = FVector::ZAxisVector;
		FVector MoonDirection = FVector::ZAxisVector;
		FVector MoonSunward = -FVector::ZAxisVector;
		double MoonCosRadius = 1.0;
		/** The luminance of the Moon's lit face, as the Moon gives it (before the panel clips it). */
		double MoonLuminance = 0.0;
		/** The picture's mean luminance over the whole sky, as the panel shows it (the Moon clipped at the peak white). */
		double MeanLuminance = 0.0;
	};

	/**
	 * The sky of a moment for an image whose horizon frame (x east, y south, z up, Time's) stands in the world as
	 * HorizonToWorld turns it: the star field turned by the moment's star-field orientation; the Moon where it is seen,
	 * its angular radius at its distance, its lit hemisphere turned towards the Sun by its phase angle and the position
	 * angle of its bright limb, and its lit face's luminance its illuminance by its phase law over its disc's lit area.
	 */
	MUSEELIGHT_API FSkyPicture SkyPicture(const FTimeMoment& Moment, const FTransform& HorizonToWorld, double PeakLuminance);
}
