#include "ArtWork.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "MatterColour.h"
#include "MatterRecords.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Modules/ModuleManager.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * The sancai glazes' colours are derived from the collection's reference photograph of the horse (Matter's
 * glaze.sancai and glaze.sancai-green, their derivations written out): the photograph normalised against a patch of
 * the white body in the same light, then read on the glazes. This re-derives them from the photograph itself, so it
 * lives with Art, which finds the collection's files; its checks are Matter's, and it runs with Musee.Matter.
 */
namespace Musee::ArtMaking::SancaiTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	/** The work's reference photograph, as the glazes' records cite it, and its size. */
	const TCHAR* const Photograph = TEXT("assets/reference/chinese-ref-sancai-horse.jpg");
	constexpr int32 PhotographWidth = 1024;
	constexpr int32 PhotographHeight = 728;

	/** A patch of the photograph, its first and last columns and rows (pixels, inclusive), as the derivations give them. */
	struct FPatch
	{
		int32 X0;
		int32 X1;
		int32 Y0;
		int32 Y1;
	};

	/** The reference: the cream of the saddle's hanging flap, the white body (under its clear glaze or bare). */
	constexpr FPatch Flap = { 505, 540, 300, 330 };

	/** The saddle's other creams, which the reference must agree with: its pommel, its seat, the flap's lower half. */
	constexpr FPatch OtherCreams[] = { { 478, 492, 205, 235 }, { 500, 540, 240, 270 }, { 510, 540, 340, 380 } };

	/** The glazed cream border of the saddle cloth, cast green by the copper glaze it borders: rejected. */
	constexpr FPatch Border = { 451, 457, 265, 344 };

	/** The white shelf the horse stands on, in the same light. */
	constexpr FPatch Shelf = { 100, 250, 600, 700 };

	/** The glazes, where they are lit, unweathered and free of drips: the amber's chest and flank, the green's cloth. */
	constexpr FPatch Amber[] = { { 385, 409, 325, 364 }, { 602, 611, 345, 399 } };
	constexpr FPatch Green[] = { { 470, 484, 315, 354 }, { 550, 569, 305, 349 } };

	/**
	 * How far a reference's chromaticity as photographed may stand from each of the other creams': 3 in a*, the red-green
	 * axis a copper cast or the green light off a glaze moves; 6 in b*, where the creams' own faint yellows differ.
	 */
	constexpr double RedGreenAgreement = 3.0;
	constexpr double YellowBlueAgreement = 6.0;

	/** How far the white shelf, scaled with the rest, may stand from neutral on the red-green axis. */
	constexpr double ShelfRedGreen = 2.0;

	/**
	 * How far a record's rounded CIELAB may stand from the photograph's: its rounding (0.5), and a JPEG decoder's
	 * upsampling of the colour, which can move a patch's median by a level. A reference with a cast moves the glazes by
	 * several units (the border's, 4 to 7 in a*).
	 */
	constexpr double RecordAgreement = 1.5;

	/** A photograph's pixels, 8-bit sRGB, row by row from the top. */
	struct FPhotograph
	{
		int32 Width = 0;
		int32 Height = 0;
		TArray<FColor> Pixels;
	};

	/** The photograph at a path; false when it can't be read as 8-bit colour. */
	bool ReadPhotograph(const FString& Path, FPhotograph& Out)
	{
		TArray64<uint8> Bytes;
		if (!FFileHelper::LoadFileToArray(Bytes, *Path, FILEREAD_Silent))
		{
			return false;
		}
		IImageWrapperModule& Wrappers = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
		const EImageFormat Format = Wrappers.DetectImageFormat(Bytes.GetData(), Bytes.Num());
		const TSharedPtr<IImageWrapper> Wrapper = Format == EImageFormat::Invalid ? nullptr : Wrappers.CreateImageWrapper(Format);
		TArray64<uint8> Raw;
		if (!Wrapper.IsValid() || !Wrapper->SetCompressed(Bytes.GetData(), Bytes.Num()) || Wrapper->GetBitDepth() != 8 || !Wrapper->GetRaw(Raw))
		{
			return false;
		}
		// The file's own layout: four channels, in red-first or blue-first order.
		const ERGBFormat Layout = Wrapper->GetFormat();
		constexpr int64 Channels = 4;
		Out.Width = int32(Wrapper->GetWidth());
		Out.Height = int32(Wrapper->GetHeight());
		if ((Layout != ERGBFormat::RGBA && Layout != ERGBFormat::BGRA) || Raw.Num() != int64(Out.Width) * Out.Height * Channels)
		{
			return false;
		}
		const bool bRedFirst = Layout == ERGBFormat::RGBA;
		Out.Pixels.SetNumUninitialized(Out.Width * Out.Height);
		for (int32 Index = 0; Index < Out.Pixels.Num(); ++Index)
		{
			const uint8* Pixel = &Raw[Index * Channels];
			Out.Pixels[Index] = bRedFirst ? FColor(Pixel[0], Pixel[1], Pixel[2]) : FColor(Pixel[2], Pixel[1], Pixel[0]);
		}
		return true;
	}

	/**
	 * A patch's colour, linear Rec.709: the median of its pixels channel by channel (the lower middle of an even count),
	 * so specks, silvering and drip edges inside it don't pull it, decoded from sRGB.
	 */
	FLinearColor MedianOf(const FPhotograph& Image, TConstArrayView<FPatch> Patches)
	{
		TArray<uint8> Channels[3];
		for (const FPatch& Patch : Patches)
		{
			for (int32 Y = Patch.Y0; Y <= Patch.Y1; ++Y)
			{
				for (int32 X = Patch.X0; X <= Patch.X1; ++X)
				{
					const FColor& Pixel = Image.Pixels[Y * Image.Width + X];
					Channels[0].Add(Pixel.R);
					Channels[1].Add(Pixel.G);
					Channels[2].Add(Pixel.B);
				}
			}
		}
		uint8 Median[3] = {};
		for (int32 Channel = 0; Channel < 3; ++Channel)
		{
			Channels[Channel].Sort();
			Median[Channel] = Channels[Channel].IsEmpty() ? 0 : Channels[Channel][(Channels[Channel].Num() - 1) / 2];
		}
		return FLinearColor::FromSRGBColor(FColor(Median[0], Median[1], Median[2]));
	}

	FVector3d LabOf(const FLinearColor& Colour)
	{
		return Musee::Matter::CieLabFromLinearRec709(Colour.R, Colour.G, Colour.B);
	}

	/** Each channel scaled by its own factor: a von Kries scaling. */
	FLinearColor Scaled(const FLinearColor& Colour, const FLinearColor& Scale)
	{
		return FLinearColor(Colour.R * Scale.R, Colour.G * Scale.G, Colour.B * Scale.B);
	}

	/** Whether a reference's chromaticity as photographed agrees with every one of the other creams'. */
	bool AgreesWithTheCreams(const FVector3d& Reference, TConstArrayView<FVector3d> Creams)
	{
		for (const FVector3d& Cream : Creams)
		{
			if (FMath::Abs(Reference.Y - Cream.Y) > RedGreenAgreement || FMath::Abs(Reference.Z - Cream.Z) > YellowBlueAgreement)
			{
				return false;
			}
		}
		return true;
	}

	FString Written(const FVector3d& Lab)
	{
		return FString::Printf(TEXT("(%.1f, %.1f, %.1f)"), Lab.X, Lab.Y, Lab.Z);
	}
}

/**
 * The sancai glazes as their photograph gives them: the reference the photograph is normalised against is a cream that
 * agrees with the photograph's other creams (not the cloth's border, cast green, which would turn the photograph
 * magenta); scaled so, a white shelf in the same light stays neutral; and the amber and the green re-derived from it
 * are the records' own.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatterSancaiFromPhotograph, "Musee.Matter.SancaiFromPhotograph", Musee::ArtMaking::SancaiTests::Flags)

bool FMatterSancaiFromPhotograph::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking::SancaiTests;
	FPhotograph Image;
	const FString Path = Musee::Art::SourcePath(Photograph);
	if (!TestTrue(*FString::Printf(TEXT("The collection's photograph of the horse reads (%s)"), *Path), ReadPhotograph(Path, Image))
		|| !TestTrue(TEXT("It is the photograph the derivations read, 1024 by 728"), Image.Width == PhotographWidth && Image.Height == PhotographHeight))
	{
		return false;
	}

	// The reference agrees with the other creams; the border, cast green, does not, and so is no reference.
	TArray<FVector3d> Creams;
	for (const FPatch& Cream : OtherCreams)
	{
		Creams.Add(LabOf(MedianOf(Image, { Cream })));
	}
	const FLinearColor Reference = MedianOf(Image, { Flap });
	const FVector3d ReferenceLab = LabOf(Reference);
	TestTrue(*FString::Printf(TEXT("The saddle flap %s agrees with the saddle's other creams"), *Written(ReferenceLab)),
		AgreesWithTheCreams(ReferenceLab, Creams));
	const FVector3d BorderLab = LabOf(MedianOf(Image, { Border }));
	AddInfo(FString::Printf(TEXT("As photographed: the flap %s, the other creams %s, the border %s."), *Written(ReferenceLab),
		*FString::JoinBy(Creams, TEXT(" "), [](const FVector3d& Cream) { return Written(Cream); }), *Written(BorderLab)));
	TestFalse(*FString::Printf(TEXT("The cloth's cream border %s is cast green, and would be no reference"), *Written(BorderLab)),
		AgreesWithTheCreams(BorderLab, Creams));

	// The body the reference is scaled to.
	const TValueOrError<FMatterSubstance, FString> Body = Musee::Matter::ReadSubstance(FMuseeKey(TEXT("earthenware.white")));
	if (!TestTrue(TEXT("The white body reads"), Body.HasValue()))
	{
		return false;
	}
	const TValueOrError<FLinearColor, FString> BodyColour = Musee::Matter::LinearRec709(Body.GetValue().Dielectric.DiffuseAlbedo);
	if (!TestTrue(TEXT("Its albedo is a colour"), BodyColour.HasValue()))
	{
		return false;
	}
	const FLinearColor Scale(BodyColour.GetValue().R / Reference.R, BodyColour.GetValue().G / Reference.G, BodyColour.GetValue().B / Reference.B);

	// A white shelf in the same light, scaled with the rest, is neither turned magenta nor green.
	const FVector3d ShelfLab = LabOf(Scaled(MedianOf(Image, { Shelf }), Scale));
	AddInfo(FString::Printf(TEXT("The scale %s; the shelf scaled %s."), *Scale.ToString(), *Written(ShelfLab)));
	TestTrue(*FString::Printf(TEXT("The white shelf, scaled, %s stays neutral on the red-green axis"), *Written(ShelfLab)),
		FMath::Abs(ShelfLab.Y) <= ShelfRedGreen);

	// The records are the photograph's, rounded.
	struct FGlaze
	{
		const TCHAR* Substance;
		TConstArrayView<FPatch> Patches;
	};
	for (const FGlaze& Glaze : { FGlaze{ TEXT("glaze.sancai"), Amber }, FGlaze{ TEXT("glaze.sancai-green"), Green } })
	{
		const TValueOrError<FMatterSubstance, FString> Record = Musee::Matter::ReadSubstance(FMuseeKey(Glaze.Substance));
		if (!TestTrue(*FString::Printf(TEXT("%s reads"), Glaze.Substance), Record.HasValue())
			|| !TestEqual(*FString::Printf(TEXT("%s is a colour"), Glaze.Substance), Record.GetValue().Dielectric.DiffuseAlbedo.Value.Num(), 3))
		{
			continue;
		}
		const TArray<double>& Value = Record.GetValue().Dielectric.DiffuseAlbedo.Value;
		const FVector3d Recorded(Value[0], Value[1], Value[2]);
		const FVector3d Derived = LabOf(Scaled(MedianOf(Image, Glaze.Patches), Scale));
		AddInfo(FString::Printf(TEXT("%s: the photograph gives %s."), Glaze.Substance, *Written(Derived)));
		TestTrue(*FString::Printf(TEXT("%s records %s, and the photograph gives %s"), Glaze.Substance, *Written(Recorded), *Written(Derived)),
			Recorded.Equals(Derived, RecordAgreement));
	}
	return true;
}

#endif
