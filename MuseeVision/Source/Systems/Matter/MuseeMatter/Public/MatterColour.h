#pragma once

#include "CoreMinimal.h"
#include "MatterTriple.h"
#include "Templates/ValueOrError.h"

/**
 * Colour in Matter: every colour a master reads is linear, in the working space of Rec.709 primaries with a D65 white.
 * A record may give a measured CIELAB colour instead (a spectrophotometer's reading of a specimen, D65 illuminant,
 * 2-degree observer), which is converted here, and nowhere else.
 */
namespace Musee::Matter
{
	/** The space names a colour triple may carry (FMatterTriple::Space). */
	inline constexpr const TCHAR* LinearRec709Space = TEXT("linear Rec.709 (sRGB primaries), D65 white");
	inline constexpr const TCHAR* CieLabSpace = TEXT("CIELAB (D65, 2 deg)");

	/**
	 * A CIELAB colour (D65 white, 2-degree observer) as linear Rec.709: CIELAB to CIE XYZ by the CIE's own inverse
	 * (CIE 15:2004, Colorimetry, section 8.2.1), then XYZ to linear Rec.709 by the matrix of IEC 61966-2-1:1999
	 * (sRGB), whose white is the same D65. A colour outside the Rec.709 gamut keeps its negative or over-unit channels.
	 */
	MUSEEMATTER_API FLinearColor LinearRec709FromCieLab(double L, double A, double B);

	/**
	 * Linear Rec.709 as CIELAB (D65 white, 2-degree observer), LinearRec709FromCieLab's inverse: linear Rec.709 to CIE
	 * XYZ by IEC 61966-2-1:1999's matrix, then CIE 15:2004's CIELAB. X is L*, Y is a*, Z is b*. For a colour derived
	 * rather than measured (a photograph's pixels, scaled), to be written as a record writes a measured one.
	 */
	MUSEEMATTER_API FVector3d CieLabFromLinearRec709(double R, double G, double B);

	/** A triple as linear Rec.709, converting a CIELAB colour; an error names a triple without three numbers or with a
	 *  space it cannot read. */
	MUSEEMATTER_API TValueOrError<FLinearColor, FString> LinearRec709(const FMatterTriple& Triple);
}
