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
