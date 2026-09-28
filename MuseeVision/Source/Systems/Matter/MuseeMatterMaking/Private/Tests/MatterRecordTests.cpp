#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "MatterColour.h"
#include "MatterNames.h"
#include "MatterRecords.h"
#include "MatterResolve.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"

namespace Musee::Matter::RecordTests
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	/** A colour channel reproduced to three decimals, the precision of IEC 61966-2-1's matrix. */
	constexpr float ChannelTolerance = 1e-3f;

	/** The fixtures beside this file. */
	FString FixtureFolder()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Source/Systems/Matter/MuseeMatterMaking/Private/Tests/Data"));
	}

	bool Near(FAutomationTestBase& Test, const TCHAR* What, const FLinearColor& Actual, const FLinearColor& Expected)
	{
		const bool bNear = FMath::Abs(Actual.R - Expected.R) < ChannelTolerance && FMath::Abs(Actual.G - Expected.G) < ChannelTolerance
			&& FMath::Abs(Actual.B - Expected.B) < ChannelTolerance;
		if (!bNear)
		{
			Test.AddError(FString::Printf(TEXT("%s: %s, expected %s."), What, *Actual.ToString(), *Expected.ToString()));
		}
		return bNear;
	}

	/** An instance made in memory at the path a spec resolves to, never saved; the caller retires it. */
	UMaterialInstanceConstant* MakeInMemory(FName Palette, FName Working, FName History)
	{
		const FString PackageName = InstancePackage(Palette, Working, History);
		UPackage* Package = CreatePackage(*PackageName);
		return NewObject<UMaterialInstanceConstant>(Package, FName(*InstanceName(Palette, Working, History)), RF_Public | RF_Transient);
	}

	void Retire(UObject* Object)
	{
		if (Object)
		{
			Object->ClearFlags(RF_Public | RF_Standalone);
			Object->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
			Object->MarkAsGarbage();
		}
	}
}

using namespace Musee::Matter;
using namespace Musee::Matter::RecordTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatterNamesArePascalCase, "Musee.Matter.Names", TestFlags)
bool FMatterNamesArePascalCase::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("A palette key"), PascalCase(TEXT("stone.travertine-romano")), FString(TEXT("StoneTravertineRomano")));
	TestEqual(TEXT("A scoped working"), PascalCase(TEXT("laminated.uv-filtering")), FString(TEXT("LaminatedUvFiltering")));
	TestEqual(TEXT("An instance"), InstancePackage(TEXT("bronze.cusn10"), TEXT("gilt"), TEXT("worn")),
		FString(TEXT("/Game/Generated/Matter/MI_BronzeCusn10_Gilt_Worn")));
	TestEqual(TEXT("A master"), MasterPackage(TEXT("stone")), FString(TEXT("/Game/Generated/Matter/M_Matter_Stone")));
	TestEqual(TEXT("A master with its layer"), MasterName(TEXT("metal"), TEXT("gilt")), FString(TEXT("M_Matter_MetalGilt")));
	TestEqual(TEXT("An object path"), ObjectPathOf(TEXT("/Game/Generated/Time/MPC_Moment")), FString(TEXT("/Game/Generated/Time/MPC_Moment.MPC_Moment")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatterColourFromCieLab, "Musee.Matter.ColourFromCieLab", TestFlags)
bool FMatterColourFromCieLab::RunTest(const FString& Parameters)
{
	// D65 white is L* 100; mid grey L* 50 has Y = (66/116)^3 = 0.18419 (CIE 15:2004); sRGB's red primary is
	// L* 53.2408, a* 80.0925, b* 67.2032 (IEC 61966-2-1 primaries through CIE 15:2004).
	Near(*this, TEXT("White"), LinearRec709FromCieLab(100.0, 0.0, 0.0), FLinearColor(1.0f, 1.0f, 1.0f));
	Near(*this, TEXT("Mid grey"), LinearRec709FromCieLab(50.0, 0.0, 0.0), FLinearColor(0.18419f, 0.18419f, 0.18419f));
	Near(*this, TEXT("The red primary"), LinearRec709FromCieLab(53.2408, 80.0925, 67.2032), FLinearColor(1.0f, 0.0f, 0.0f));

	FMatterTriple Lab;
	Lab.Value = { 50.0, 0.0, 0.0 };
	Lab.Space = CieLabSpace;
	TestTrue(TEXT("A CIELAB triple converts"), LinearRec709(Lab).HasValue());
	Lab.Space = TEXT("Munsell");
	TestTrue(TEXT("An unknown space is refused"), LinearRec709(Lab).HasError());
	Lab.Value = { 50.0 };
	TestTrue(TEXT("A triple of one number is refused"), LinearRec709(Lab).HasError());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatterRecordsRead, "Musee.Matter.RecordsRead", TestFlags)
bool FMatterRecordsRead::RunTest(const FString& Parameters)
{
	const TValueOrError<FMatterPalette, FString> Palette = ReadPalette();
	if (!TestTrue(TEXT("The palette reads"), Palette.HasValue()))
	{
		AddError(Palette.GetError());
		return false;
	}
	const FMatterPaletteEntry* Travertine = Palette.GetValue().Find(FMuseeKey(TEXT("stone.travertine-romano")));
	if (TestNotNull(TEXT("The palette names the travertine"), Travertine))
	{
		TestEqual(TEXT("Its substance"), Travertine->Substance.ToString(), FString(TEXT("travertine")));
		TestTrue(TEXT("It offers finishes"), Travertine->Finishes.Num() > 0);
	}

	// Tang sancai is three lead glazes over a white body (Shen et al. 2019): its palette entry offers each as a finish,
	// each working laying its own glaze, so the object can carry all three; none of the three is a stand-in for another.
	const FMatterPaletteEntry* Sancai = Palette.GetValue().Find(FMuseeKey(TEXT("accent.earthenware-sancai")));
	if (TestNotNull(TEXT("The palette names the sancai earthenware"), Sancai))
	{
		TSet<FString> Glazes;
		TMap<FString, TArray<double>> Albedos;
		for (const TCHAR* Working : {TEXT("glazed.sancai"), TEXT("glazed.sancai-green"), TEXT("glazed.sancai-clear")})
		{
			TestTrue(*FString::Printf(TEXT("It offers %s"), Working),
				Sancai->Finishes.ContainsByPredicate([Working](const FMatterPaletteFinish& F) { return F.Working == FMuseeKey(Working); }));
			const TValueOrError<FMatterWorking, FString> Worked = ReadWorking(FMuseeKey(Working));
			if (TestTrue(*FString::Printf(TEXT("%s reads"), Working), Worked.HasValue()))
			{
				const TValueOrError<FMatterSubstance, FString> Glaze = ReadSubstance(Worked.GetValue().Lays);
				if (TestTrue(*FString::Printf(TEXT("%s lays a glaze that reads"), Working), Glaze.HasValue()))
				{
					const auto& Albedo = Glaze.GetValue().Dielectric.DiffuseAlbedo;
					Glazes.Add(FString::JoinBy(Albedo.Value, TEXT(" "), [](double V) { return FString::SanitizeFloat(V); }));
					// The collection holds its own photograph of the horse it shows; its glazes are read from that horse, not
					// from another museum's, whose glazes lie differently (the Met's 1991.253.12 is cream with an amber saddle).
					TestTrue(*FString::Printf(TEXT("%s is read from the work's own reference photograph"), Working),
						Albedo.Source.Contains(TEXT("assets/reference/chinese-ref-sancai-horse.jpg")));
					// An uncalibrated photograph's pixels are no albedo: each glaze is derived from it, normalised against a
					// neutral in the same light, and says how.
					TestTrue(*FString::Printf(TEXT("%s is derived, its derivation written out"), Working),
						Albedo.Grade == TEXT("derived") && !Albedo.Derivation.IsEmpty());
					Albedos.Add(Working, Albedo.Value);
				}
			}
		}
		TestEqual(TEXT("the amber, the green and the clear are three colours"), Glazes.Num(), 3);

		// Checks that hold whatever the photograph's exposure: how the glazes stand to one another and to their body.
		const TArray<double>* Amber = Albedos.Find(TEXT("glazed.sancai"));
		const TArray<double>* Clear = Albedos.Find(TEXT("glazed.sancai-clear"));
		const TValueOrError<FMatterSubstance, FString> Body = ReadSubstance(Sancai->Substance);
		if (TestTrue(TEXT("the amber, the clear and their body are colours"), Amber && Clear && Amber->Num() == 3 && Clear->Num() == 3
			&& Body.HasValue() && Body.GetValue().Dielectric.DiffuseAlbedo.Value.Num() == 3))
		{
			// Iron colours the amber: it takes light the clear glaze passes, most of it at the blue end.
			TestTrue(TEXT("the amber is darker than the clear glaze"), (*Amber)[0] < (*Clear)[0]);
			TestTrue(TEXT("and redder"), (*Amber)[1] > (*Clear)[1]);
			// A colourless glass over the body shows the body and adds no light to it.
			TestTrue(TEXT("the clear glaze is no brighter than its body"), (*Clear)[0] <= Body.GetValue().Dielectric.DiffuseAlbedo.Value[0]);
		}
	}

	const TValueOrError<FMatterSubstance, FString> Substance = ReadSubstance(FMuseeKey(TEXT("travertine")));
	if (TestTrue(TEXT("The travertine's typed record reads"), Substance.HasValue()))
	{
		const FMatterSubstance& S = Substance.GetValue();
		TestEqual(TEXT("Its master"), S.Master.ToString(), FString(TEXT("stone")));
		TestTrue(TEXT("Its porosity is a fraction with a source"), S.Dielectric.Porosity.HasValue() && S.Dielectric.Porosity.Unit == TEXT("1") && !S.Dielectric.Porosity.Source.IsEmpty());
		TestEqual(TEXT("Its colour is the measured CIELAB"), S.Dielectric.DiffuseAlbedo.Space, FString(CieLabSpace));
	}
	else
	{
		AddError(Substance.GetError());
	}

	TestTrue(TEXT("A working reads"), ReadWorking(FMuseeKey(TEXT("laminated.uv-filtering"))).HasValue());
	TestTrue(TEXT("A history regime reads by master and regime"), ReadHistory(FMuseeKey(TEXT("stone")), FMuseeKey(TEXT("worn"))).HasValue());
	TestTrue(TEXT("An unknown substance is an error"), ReadSubstance(FMuseeKey(TEXT("unobtainium"))).HasError());
	TestTrue(TEXT("No key is an error"), ReadWorking(FMuseeKey()).HasError());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatterResolveNeverFallsBack, "Musee.Matter.Resolve", TestFlags)
bool FMatterResolveNeverFallsBack::RunTest(const FString& Parameters)
{
	const FName Palette(TEXT("matter-test.resolve"));
	UMaterialInstanceConstant* Made = MakeInMemory(Palette, TEXT("honed"), TEXT("touched"));
	TestEqual(TEXT("A made spec resolves to its instance"), static_cast<UObject*>(Resolve(Palette, TEXT("honed"), TEXT("touched"))), static_cast<UObject*>(Made));

	AddExpectedMessagePlain(TEXT("No material was made for"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 2);
	TestNull(TEXT("An unmade spec resolves to nothing"), Resolve(TEXT("matter-test.never-made"), TEXT("honed"), TEXT("clean")));

	const FString Folder = FixtureFolder();
	TestEqual(TEXT("A finish schedule's surface resolves"), static_cast<UObject*>(ResolveFinish(Folder, TEXT("coping"))), static_cast<UObject*>(Made));
	TestNull(TEXT("A surface whose material was never made resolves to nothing"), ResolveFinish(Folder, TEXT("nowhere")));

	AddExpectedMessagePlain(TEXT("names no surface"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	TestNull(TEXT("A surface the schedule does not name resolves to nothing"), ResolveFinish(Folder, TEXT("lintel")));

	AddExpectedMessagePlain(TEXT("is named by a palette key"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	TestNull(TEXT("A spec missing its history resolves to nothing"), Resolve(Palette, TEXT("honed"), NAME_None));
	Retire(Made);
	return true;
}

#endif
