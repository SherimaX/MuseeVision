#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "LightLuminaire.h"
#include "Misc/FileHelper.h"
#include "MuseeRecords.h"
#include "Photometry/PhotometricFile.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

// A maker's photometric file as Light reads it: the 362G wallwasher's LM-63 file, kept in Data/Light/Photometry,
// integrates to the flux its maker rates it at, and a file Light can't read is refused with its reason.
namespace Musee::Light::PhotometricFileTests
{
	inline constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** The iGuzzini Sipario LV 362G's file, under Data. */
	constexpr const TCHAR* WallwasherFile = TEXT("Light/Photometry/iguzzini-362G_O25F.ies");

	/** Its rated flux, 999 lm (the file's [_ABSOLUTELUMENS] and the maker's page), and its greatest intensity, 1566.3 cd. */
	constexpr double RatedFluxLumens = 999.0;
	constexpr double PeakCandela = 1566.3;

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLightPhotometricFileTest, "Musee.Light.PhotometricFile", Flags)
	bool FLightPhotometricFileTest::RunTest(const FString& Parameters)
	{
		const TValueOrError<FPhotometricFile, FString> Read = ReadPhotometricFile(Records::DataRoot() / WallwasherFile);
		if (!TestFalse(TEXT("The 362G's file reads"), Read.HasError()))
		{
			AddError(Read.GetError());
			return false;
		}
		const FPhotometricFile& File = Read.GetValue();
		TestEqual(TEXT("It is a Type C table"), File.PhotometricType, 1);
		TestTrue(TEXT("of 181 vertical angles, 0 to 180 degrees"), File.VerticalDegrees.Num() == 181 && File.VerticalDegrees[0] == 0.0
			&& File.VerticalDegrees.Last() == 180.0);
		TestTrue(TEXT("and 37 horizontal angles, 0 to 180 degrees: symmetric about its 0-180 plane"), File.HorizontalDegrees.Num() == 37
			&& File.HorizontalDegrees[0] == 0.0 && File.HorizontalDegrees.Last() == 180.0);
		const double Flux = File.FluxLumens();
		TestTrue(FString::Printf(TEXT("Its candela table integrates to its rated 999 lm, to within 1 %% (%.2f lm)"), Flux),
			FMath::Abs(Flux - RatedFluxLumens) <= 0.01 * RatedFluxLumens);
		TestTrue(TEXT("Its greatest intensity is 1566.3 cd"), FMath::IsNearlyEqual(File.PeakCandela(), PeakCandela, 1e-6));
		TestTrue(TEXT("found at 21 degrees from the nadir, in its 0 degree plane"), FMath::IsNearlyEqual(File.At(0, 21), PeakCandela, 1e-6));
		const FPhotometricFile::FDirection Throw = File.PeakDirection();
		TestTrue(FString::Printf(TEXT("which is its throw (%.1f, %.1f)"), Throw.VerticalDegrees, Throw.HorizontalDegrees),
			Throw.VerticalDegrees == 21.0 && Throw.HorizontalDegrees == 0.0);
		const FLightLuminaire* Type = FindLuminaire(FMuseeKey(TEXTVIEW("track-wallwasher.3000k")));
		TestTrue(TEXT("and the 362G's type carries it, to be aimed"), Type && Type->PeakVertical.Get() == 21.0 && Type->PeakHorizontal.Get() == 0.0
			&& Type->PeakIntensity.Get() == File.PeakCandela());
		const double Towards = File.FluxTowardsZeroPlaneLumens();
		TestTrue(FString::Printf(TEXT("Its lens throws about two thirds of its light into the half towards 0 degrees (%.1f lm)"), Towards),
			Towards > 0.6 * Flux && Towards < 0.7 * Flux);
		double Upward = 0.0;
		for (int32 H = 0; H < File.HorizontalDegrees.Num(); ++H)
		{
			for (int32 V = 0; V < File.VerticalDegrees.Num(); ++V)
			{
				Upward = File.VerticalDegrees[V] >= 90.0 ? FMath::Max(Upward, File.At(H, V)) : Upward;
			}
		}
		TestTrue(TEXT("and none above the horizontal"), Upward < 0.1);
		TestTrue(TEXT("Its luminous opening is round, 44 mm across"), File.RoundOpeningRadiusMeters().IsSet()
			&& FMath::IsNearlyEqual(File.RoundOpeningRadiusMeters().GetValue(), 0.022, 1e-9));
		TestTrue(TEXT("measured at 16.1 W"), FMath::IsNearlyEqual(File.InputWatts, 16.1, 1e-9));

		// A single plane stands for the whole turn: a lamp giving 1 cd every way carries 4 pi lumens.
		const TValueOrError<FPhotometricFile, FString> Even = ParsePhotometricFile(
			TEXT("IESNA:LM-63-2002\n[TEST] even\nTILT=NONE\n1 -1 1 3 1 1 2 0 0 0\n1 1 1\n0 90 180\n0\n1 1 1\n"));
		TestTrue(TEXT("An even sphere of 1 cd carries 4 pi lm"), !Even.HasError()
			&& FMath::IsNearlyEqual(Even.GetValue().FluxLumens(), 4.0 * UE_DOUBLE_PI, 1e-9));
		TestTrue(TEXT("half of it towards any plane"), !Even.HasError()
			&& FMath::IsNearlyEqual(Even.GetValue().FluxTowardsZeroPlaneLumens(), 2.0 * UE_DOUBLE_PI, 1e-9));
		TestTrue(TEXT("A tilted lamp's file is refused"), ParsePhotometricFile(TEXT("IESNA:LM-63-2002\nTILT=INCLUDE\n1\n")).HasError());
		TestTrue(TEXT("and so is a table short of its numbers"),
			ParsePhotometricFile(TEXT("IESNA:LM-63-2002\nTILT=NONE\n1 -1 1 3 1 1 2 0 0 0\n1 1 1\n0 90 180\n0\n1 1\n")).HasError());
		return true;
	}

	/** The catalogue's records, under Data. */
	constexpr const TCHAR* LuminaireRecords = TEXT("Light/Luminaires");

	/** The record's field that names its photometric file, and so the covers entry of the source that is the file. */
	constexpr const TCHAR* PhotometricFileField = TEXT("photometricFile");

	/** How far a record's flux may stand from the flux its file integrates to: the catalogue's own 1 % (Catalogue.cpp). */
	constexpr double FluxAgreement = 0.01;

	/** The most decimals a record writes a number to. */
	constexpr int32 MostDecimals = 6;

	/** The number at a dotted path in a record ("light.peakCandela"), or unset. */
	TOptional<double> NumberAt(const TSharedPtr<FJsonObject>& Record, const FString& Path)
	{
		FString Section;
		FString Field;
		if (!Path.Split(TEXT("."), &Section, &Field)) { return {}; }
		const TSharedPtr<FJsonObject>* Object = nullptr;
		double Value = 0.0;
		if (!Record->TryGetObjectField(Section, Object) || !(*Object)->TryGetNumberField(Field, Value)) { return {}; }
		return Value;
	}

	/** Half a unit in the last decimal a number is written to: how far the true value may lie from what is written. */
	double HalfLastDigit(double Written)
	{
		for (int32 Decimals = 0; Decimals < MostDecimals; ++Decimals)
		{
			const double Scaled = Written * FMath::Pow(10.0, Decimals);
			if (FMath::IsNearlyEqual(Scaled, FMath::RoundHalfFromZero(Scaled), 1e-6)) { return 0.5 * FMath::Pow(10.0, -Decimals); }
		}
		return 0.5 * FMath::Pow(10.0, -MostDecimals);
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLightPhotometricEvidenceTest, "Musee.Light.PhotometricEvidence", Flags)
	bool FLightPhotometricEvidenceTest::RunTest(const FString& Parameters)
	{
		// Each number a record credits to its photometric file (its source that covers the file) is the file's: the flux
		// its table integrates to, within the catalogue's 1 %; its greatest intensity and its input power, as the record
		// writes them, rounded from the file's. A file gives nothing else a record could credit to it.
		const FString Folder = Records::DataRoot() / LuminaireRecords;
		TArray<FString> Names;
		IFileManager::Get().FindFiles(Names, *(Folder / TEXT("*.json")), true, false);
		Names.Sort();
		int32 Credited = 0;
		for (const FString& Name : Names)
		{
			FString Text;
			TSharedPtr<FJsonObject> Record;
			if (!FFileHelper::LoadFileToString(Text, *(Folder / Name)) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Record)
				|| !Record.IsValid())
			{
				AddError(FString::Printf(TEXT("%s doesn't read as JSON"), *Name));
				continue;
			}
			const TSharedPtr<FJsonObject>* Photometry = nullptr;
			FString FilePath;
			if (!Record->TryGetObjectField(PhotometricFileField, Photometry) || !(*Photometry)->TryGetStringField(TEXT("path"), FilePath)) { continue; }
			const TValueOrError<FPhotometricFile, FString> File = ReadPhotometricFile(Records::DataRoot() / FilePath);
			if (File.HasError())
			{
				AddError(FString::Printf(TEXT("%s: its photometric file doesn't read: %s"), *Name, *File.GetError()));
				continue;
			}
			const TMap<FString, double> FromFile = {
				{TEXT("light.luminousFluxLumens"), File.GetValue().FluxLumens()},
				{TEXT("light.peakCandela"), File.GetValue().PeakCandela()},
				{TEXT("electrical.systemPowerWatts"), File.GetValue().InputWatts},
			};
			const TArray<TSharedPtr<FJsonValue>>* Sources = nullptr;
			if (!Record->TryGetArrayField(TEXT("sources"), Sources)) { continue; }
			for (const TSharedPtr<FJsonValue>& Source : *Sources)
			{
				const TArray<TSharedPtr<FJsonValue>>* Covers = nullptr;
				if (!Source->AsObject().IsValid() || !Source->AsObject()->TryGetArrayField(TEXT("covers"), Covers)) { continue; }
				TArray<FString> Covered;
				for (const TSharedPtr<FJsonValue>& Cover : *Covers) { Covered.Add(Cover->AsString()); }
				if (!Covered.Contains(PhotometricFileField)) { continue; }
				for (const FString& Path : Covered)
				{
					if (Path == PhotometricFileField) { continue; }
					++Credited;
					const double* Measured = FromFile.Find(Path);
					const TOptional<double> Written = NumberAt(Record, Path);
					if (!Measured || !Written.IsSet())
					{
						AddError(FString::Printf(TEXT("%s credits '%s' to its photometric file, which gives no such number"), *Name, *Path));
						continue;
					}
					const double Allowed = Path == TEXT("light.luminousFluxLumens") ? FluxAgreement * *Measured : HalfLastDigit(Written.GetValue());
					TestTrue(FString::Printf(TEXT("%s: its %s, %g, is its photometric file's %.6g"), *Name, *Path, Written.GetValue(), *Measured),
						FMath::Abs(Written.GetValue() - *Measured) <= Allowed);
				}
			}
		}
		TestTrue(TEXT("Some record credits numbers to its photometric file"), Credited > 0);
		return true;
	}
}

#endif
