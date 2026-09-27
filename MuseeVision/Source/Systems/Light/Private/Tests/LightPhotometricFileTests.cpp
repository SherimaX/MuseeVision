#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "MuseeRecords.h"
#include "Photometry/PhotometricFile.h"

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
}

#endif
