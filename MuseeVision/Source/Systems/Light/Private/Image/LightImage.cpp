#include "LightImage.h"

#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "Sky/SkyPhotometry.h"
#include "TimeMoment.h"
#include "UObject/Package.h"

namespace Musee::Light::Image
{
	namespace
	{
		/**
		 * A star of apparent magnitude 0 gives 2.08 microlux (Wikipedia, 'Lux', https://en.wikipedia.org/wiki/Lux ; and
		 * 'Apparent magnitude': 1 lux is about magnitude -14.2; read 2026-09-27); each magnitude is 100^(1/5) times fainter.
		 */
		constexpr double MagnitudeZeroLux = 2.08e-6;
		constexpr double LuxRatioPerMagnitude = 0.4;

		/** Ballesteros's temperature from B-V: T = 4600 (1 / (0.92 BV + 1.7) + 1 / (0.92 BV + 0.62)) kelvin. */
		constexpr double BallesterosKelvin = 4600.0;
		constexpr double BallesterosSlope = 0.92;
		constexpr double BallesterosHot = 1.7;
		constexpr double BallesterosCool = 0.62;

		/** Rec. 709's luminance weights (ITU-R BT.709-6, item 3.2). */
		const FLinearColor Rec709Luminance(0.2126f, 0.7152f, 0.0722f);

		/** The catalogue's header: its tag, and the count of stars after it; then four little-endian floats a star. */
		constexpr uint8 CatalogueTag[4] = { 'M', 'V', 'S', '1' };
		constexpr int32 HeaderBytes = 8;
		constexpr int32 StarBytes = 16;

		/** The whole sky, in steradians. */
		constexpr double SkySteradians = 4.0 * UE_DOUBLE_PI;

		float ReadFloat(TConstArrayView<uint8> Bytes, int32 At)
		{
			float Value = 0.0f;
			FMemory::Memcpy(&Value, Bytes.GetData() + At, sizeof(Value));
			return Value;
		}

		double LuminanceOf(const FLinearColor& Colour)
		{
			return Colour.R * Rec709Luminance.R + Colour.G * Rec709Luminance.G + Colour.B * Rec709Luminance.B;
		}
	}

	double StarIlluminanceLux(double VisualMagnitude)
	{
		return MagnitudeZeroLux * FMath::Pow(10.0, -LuxRatioPerMagnitude * VisualMagnitude);
	}

	FLinearColor StarColour(double BMinusV)
	{
		const double Kelvin = BallesterosKelvin * (1.0 / (BallesterosSlope * BMinusV + BallesterosHot) + 1.0 / (BallesterosSlope * BMinusV + BallesterosCool));
		const FLinearColor Colour = FLinearColor::MakeFromColorTemperature(static_cast<float>(Kelvin));
		const double Luminance = LuminanceOf(Colour);
		return Luminance > 0.0 ? Colour * static_cast<float>(1.0 / Luminance) : FLinearColor::White;
	}

	TValueOrError<FStarField, FString> DrawStarField(TConstArrayView<uint8> Catalogue, int32 Width, int32 Height)
	{
		if (Catalogue.Num() < HeaderBytes || FMemory::Memcmp(Catalogue.GetData(), CatalogueTag, sizeof(CatalogueTag)) != 0 || Width < 2 || Height < 2)
		{
			return MakeError(TEXT("the catalogue doesn't begin with MVS1 and its count, or the field has no texels"));
		}
		uint32 Count = 0;
		FMemory::Memcpy(&Count, Catalogue.GetData() + sizeof(CatalogueTag), sizeof(Count));
		if (static_cast<int64>(HeaderBytes) + static_cast<int64>(Count) * StarBytes != Catalogue.Num())
		{
			return MakeError(FString::Printf(TEXT("the catalogue says %u stars of %d bytes, and holds %d bytes"), Count, StarBytes, Catalogue.Num()));
		}
		FStarField Field;
		Field.Width = Width;
		Field.Height = Height;
		Field.Luminance.Init(FLinearColor::Black, Width * Height);
		// A texel's solid angle: its width in right ascension times its height in declination, times the cosine of its row's.
		const double TexelSteradians = (2.0 * UE_DOUBLE_PI / Width) * (UE_DOUBLE_PI / Height);
		const auto RowSteradians = [&](int32 Row) { return TexelSteradians * FMath::Sin(UE_DOUBLE_PI * (Row + 0.5) / Height); };
		double TotalLux = 0.0;
		for (uint32 Index = 0; Index < Count; ++Index)
		{
			const int32 At = HeaderBytes + static_cast<int32>(Index) * StarBytes;
			const double RightAscension = ReadFloat(Catalogue, At);
			const double Declination = ReadFloat(Catalogue, At + 4);
			const double Magnitude = ReadFloat(Catalogue, At + 8);
			const double Colour = ReadFloat(Catalogue, At + 12);
			if (Magnitude > FaintestMagnitude)
			{
				continue;
			}
			const double Lux = StarIlluminanceLux(Magnitude);
			TotalLux += Lux;
			++Field.Stars;
			// The star's place in texels, whose centres stand at half-texels; its light shared bilinearly between four.
			const double U = FMath::Frac(RightAscension / (2.0 * UE_DOUBLE_PI)) * Width - 0.5;
			const double V = FMath::Clamp((0.5 * UE_DOUBLE_PI - Declination) / UE_DOUBLE_PI, 0.0, 1.0) * Height - 0.5;
			const int32 Column = FMath::FloorToInt(U);
			const int32 Row = FMath::FloorToInt(V);
			const double Across = U - Column;
			const double Down = V - Row;
			const FLinearColor Tint = StarColour(Colour);
			for (int32 Corner = 0; Corner < 4; ++Corner)
			{
				const int32 C = ((Column + (Corner & 1)) % Width + Width) % Width;
				const int32 R = FMath::Clamp(Row + (Corner >> 1), 0, Height - 1);
				const double Share = ((Corner & 1) ? Across : 1.0 - Across) * ((Corner >> 1) ? Down : 1.0 - Down);
				Field.Luminance[R * Width + C] += Tint * static_cast<float>(Share * Lux / RowSteradians(R));
			}
		}
		Field.MeanLuminance = TotalLux / SkySteradians;
		return MakeValue(MoveTemp(Field));
	}

	const TCHAR* EmissionHlsl()
	{
		// The Moon hides the stars behind it; its face is lit where its surface, seen from here, faces the Sun.
		return TEXT(
			"float3 d = -normalize(ToEye);\n"
			"float3 e = float3(dot(SkyRowX.xyz, d), dot(SkyRowY.xyz, d), dot(SkyRowZ.xyz, d));\n"
			"float2 uv = float2(frac(atan2(e.y, e.x) / 6.28318530718), acos(clamp(e.z, -1.0, 1.0)) / 3.14159265359);\n"
			"float3 c = Texture2DSampleLevel(StarField, StarFieldSampler, uv, 0.0).rgb;\n"
			"float3 m = normalize(MoonDirection.xyz);\n"
			"float cm = dot(d, m);\n"
			"if (cm > MoonCosRadius)\n"
			"{\n"
			"\tfloat3 p = (d - m * cm) / sqrt(max(1.0 - MoonCosRadius * MoonCosRadius, 1e-12));\n"
			"\tfloat3 n = p - m * sqrt(saturate(1.0 - dot(p, p)));\n"
			"\tc = dot(n, MoonSunward.xyz) > 0.0 ? MoonLuminance.xxx : float3(0.0, 0.0, 0.0);\n"
			"}\n"
			"return min(c, PeakLuminance.xxx) * Level;\n");
	}

	FString MaterialPackage()
	{
		return FString::Printf(TEXT("%s/%s"), Folder, MaterialName);
	}

	UMaterialInterface* FindMaterial()
	{
		const FString Path = MaterialPackage() + TEXT(".") + MaterialName;
		if (UMaterialInterface* Loaded = FindObject<UMaterialInterface>(nullptr, *Path))
		{
			return Loaded;
		}
		return FPackageName::DoesPackageExist(MaterialPackage()) ? LoadObject<UMaterialInterface>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet) : nullptr;
	}

	FSkyPicture SkyPicture(const FTimeMoment& Moment, const FTransform& HorizonToWorld, double PeakLuminance)
	{
		FSkyPicture Picture;
		const FMatrix& Orientation = Moment.StarFieldOrientation;
		const auto ToWorld = [&HorizonToWorld, &Orientation](const FVector& Equatorial)
		{
			return HorizonToWorld.TransformVectorNoScale(Orientation.TransformVector(Equatorial)).GetSafeNormal();
		};
		// The turn from the equator to the world is orthonormal, so the turn back is its transpose: its columns are the rows.
		Picture.SkyRowX = ToWorld(FVector::XAxisVector);
		Picture.SkyRowY = ToWorld(FVector::YAxisVector);
		Picture.SkyRowZ = ToWorld(FVector::ZAxisVector);

		// The Moon on the celestial sphere, and the directions north and east on the sky at it (J2000 is right-handed).
		const FVector Moon = Orientation.InverseTransformVector(Moment.MoonDirection).GetSafeNormal();
		const FVector East = FVector::CrossProduct(FVector::ZAxisVector, Moon).GetSafeNormal(UE_SMALL_NUMBER, FVector::YAxisVector);
		const FVector North = FVector::CrossProduct(Moon, East);
		const double Limb = FMath::DegreesToRadians(Moment.MoonBrightLimbDegrees);
		const double Phase = FMath::DegreesToRadians(Moment.MoonPhaseAngleDegrees);
		// The sub-solar point, seen from the Earth: behind the Moon at new (phase 180), facing us at full (phase 0), and
		// turned towards the bright limb between.
		const FVector Towards = North * FMath::Cos(Limb) + East * FMath::Sin(Limb);
		Picture.MoonDirection = ToWorld(Moon);
		Picture.MoonSunward = ToWorld(-Moon * FMath::Cos(Phase) + Towards * FMath::Sin(Phase));

		const double Radius = 0.5 * FMath::DegreesToRadians(SkyPhotometry::MoonAngularDiameter(Moment.MoonDistanceKilometers).Get());
		Picture.MoonCosRadius = FMath::Cos(Radius);
		const double Disc = 2.0 * UE_DOUBLE_PI * (1.0 - Picture.MoonCosRadius);
		const double Lit = FMath::Clamp(Moment.MoonIlluminatedFraction, 0.0, 1.0);
		const double Lux = SkyPhotometry::MoonIlluminance(Musee::Degrees(Moment.MoonPhaseAngleDegrees), Moment.MoonDistanceKilometers).Get();
		Picture.MoonLuminance = Lit > 0.0 && Disc > 0.0 ? Lux / (Disc * Lit) : 0.0;
		Picture.MeanLuminance = StarFieldMeanLuminance + FMath::Min(Picture.MoonLuminance, PeakLuminance) * Disc * Lit / (4.0 * UE_DOUBLE_PI);
		return Picture;
	}
}
