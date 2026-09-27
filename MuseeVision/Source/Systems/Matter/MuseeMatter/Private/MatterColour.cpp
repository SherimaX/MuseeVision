#include "MatterColour.h"

namespace Musee::Matter
{
namespace
{
	/** The D65 white point of the 2-degree observer, normalised to Y = 1 (CIE 15:2004, Colorimetry, Table T.3). */
	constexpr double WhiteX = 0.95047;
	constexpr double WhiteY = 1.0;
	constexpr double WhiteZ = 1.08883;

	/** CIELAB's constants (CIE 15:2004, section 8.2.1): L* = 116 f(Y/Yn) - 16, a* = 500 (f(X/Xn) - f(Y/Yn)),
	 *  b* = 200 (f(Y/Yn) - f(Z/Zn)), with f linear below (6/29)^3. */
	constexpr double LightnessSlope = 116.0;
	constexpr double LightnessOffset = 16.0;
	constexpr double RedGreenSlope = 500.0;
	constexpr double YellowBlueSlope = 200.0;
	constexpr double Delta = 6.0 / 29.0;
	constexpr double LinearOffset = 4.0 / 29.0;
	constexpr double LinearSlope = 3.0 * Delta * Delta;

	/** CIE XYZ (D65) to linear Rec.709: IEC 61966-2-1:1999 (sRGB), section 5.2. */
	constexpr double XyzToRec709[3][3] = {
		{ 3.2406, -1.5372, -0.4986 },
		{ -0.9689, 1.8758, 0.0415 },
		{ 0.0557, -0.2040, 1.0570 },
	};

	constexpr int32 TripleComponents = 3;

	/** The inverse of CIELAB's f. */
	double InverseF(double T)
	{
		return T > Delta ? T * T * T : LinearSlope * (T - LinearOffset);
	}
}

FLinearColor LinearRec709FromCieLab(double L, double A, double B)
{
	const double Fy = (L + LightnessOffset) / LightnessSlope;
	const double Fx = Fy + A / RedGreenSlope;
	const double Fz = Fy - B / YellowBlueSlope;
	const double Xyz[TripleComponents] = { WhiteX * InverseF(Fx), WhiteY * InverseF(Fy), WhiteZ * InverseF(Fz) };
	double Rgb[TripleComponents];
	for (int32 Row = 0; Row < TripleComponents; ++Row)
	{
		Rgb[Row] = XyzToRec709[Row][0] * Xyz[0] + XyzToRec709[Row][1] * Xyz[1] + XyzToRec709[Row][2] * Xyz[2];
	}
	return FLinearColor(static_cast<float>(Rgb[0]), static_cast<float>(Rgb[1]), static_cast<float>(Rgb[2]));
}

TValueOrError<FLinearColor, FString> LinearRec709(const FMatterTriple& Triple)
{
	if (!Triple.HasValue())
	{
		return MakeError(FString::Printf(TEXT("has %d numbers where a colour has %d"), Triple.Value.Num(), TripleComponents));
	}
	const TArray<double>& V = Triple.Value;
	if (Triple.Space == LinearRec709Space)
	{
		return MakeValue(FLinearColor(static_cast<float>(V[0]), static_cast<float>(V[1]), static_cast<float>(V[2])));
	}
	if (Triple.Space == CieLabSpace)
	{
		return MakeValue(LinearRec709FromCieLab(V[0], V[1], V[2]));
	}
	return MakeError(FString::Printf(TEXT("is in the space '%s'; Matter reads '%s' and '%s'"), *Triple.Space, LinearRec709Space, CieLabSpace));
}
}
