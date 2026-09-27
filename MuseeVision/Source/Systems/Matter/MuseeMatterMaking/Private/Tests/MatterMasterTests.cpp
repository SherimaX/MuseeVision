#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionSubstrate.h"
#include "MatterMasterAsset.h"
#include "MatterMasterCompile.h"
#include "MatterMasterGraph.h"
#include "MatterMasterRecipe.h"
#include "MatterStoneProbe.h"
#include "Misc/FileHelper.h"
#include "UObject/Package.h"

namespace Musee::Matter::Tests
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	bool AnyContains(const TArray<FString>& Problems, const TCHAR* Text)
	{
		return Problems.ContainsByPredicate([Text](const FString& Problem) { return Problem.Contains(Text); });
	}

	TArray<uint8> ReadPackage(const FString& Filename)
	{
		TArray<uint8> Bytes;
		FFileHelper::LoadFileToArray(Bytes, *Filename);
		return Bytes;
	}

	/** A recipe whose one call reaches every function of Matter's shading library, so that compiling it compiles the
	 *  whole library. It is built in the transient package and never saved. */
	FMasterRecipe LibraryProofRecipe()
	{
		FMasterRecipe Recipe;
		Recipe.PackagePath = TEXT("/Game/Generated/Matter");
		Recipe.AssetName = TEXT("M_Matter_Library_Proof");

		FMasterParameter& Probe = Recipe.Parameters.AddDefaulted_GetRef();
		Probe.Name = TEXT("Probe");
		Probe.Group = TEXT("Proof");
		Probe.Components = 3;
		Probe.Default = FLinearColor(0.5f, 0.5f, 0.5f);
		Probe.Unit = TEXT("1");
		Probe.Meaning = TEXT("Any value: every argument of every library function is drawn from it");

		FMasterCall& Call = Recipe.Calls.AddDefaulted_GetRef();
		Call.Name = TEXT("Library");
		Call.Includes = { TEXT("/Project/Matter/Frame.ush"), TEXT("/Project/Matter/Optics.ush"), TEXT("/Project/Matter/History.ush"),
			TEXT("/Project/Matter/Stone.ush"), TEXT("/Project/Matter/Metal.ush"), TEXT("/Project/Matter/Glass.ush"),
			TEXT("/Project/Matter/White.ush"), TEXT("/Project/Matter/Medium.ush"), TEXT("/Project/Matter/Paint.ush") };
		Call.Code = TEXT(R"(const float S = Probe.x;
FMatterSurface Surface;
Surface.DiffuseAlbedo = Probe;
Surface.F0 = MatterConductorF0(Probe, Probe);
Surface.Roughness = S;
Surface.Haziness = S;
Surface.Slope = Probe.xy;
FMatterWeather Weather;
Weather.RainHistoryMillimeters = S;
Weather.RainVelocityMetersPerSecond = -Probe;
Weather.SnowWaterMillimeters = S;
FMatterPlace Place;
Place.NormalWorld = normalize(Probe);
Place.SkyExposure = S;
Place.Cavity = S;
Place.WashExposure = S;
Place.PondingDepthMillimeters = S;
Place.HollowDepthMillimeters = S;
Place.RunoffConcentration = S;
Place.PassagesPerYear = S;
Place.AgeYears = S;
FMatterResponse Response;
Response.Porosity = S;
Response.WettingDepthMillimeters = S;
Response.TextureDepthMillimeters = S;
Response.WornRoughness = S;
Response.WornHaziness = S;
Response.WearPassages = S;
Response.SoilAlbedo = Probe;
Response.DepositionPerYear = S;
Response.WashRemovalPerYear = S;
FMatterSnow Snow;
Snow.Albedo = Probe;
Snow.Roughness = S;
Snow.OpticalWaterMillimeters = S;
FMatterStone Stone;
Stone.GrainMillimeters = S;
Stone.BedMillimeters = S;
Stone.BedAspect = S;
Stone.HurstExponent = S;
Stone.RoughnessSpread = S;
Stone.ToneSpread = S;
Stone.ReliefMillimeters = S;
FMatterPatina Patina;
Patina.CupriteAlbedo = Probe;
Patina.CupriteRefractiveIndex = S;
Patina.GreenAlbedo = Probe;
Patina.GreenRefractiveIndex = S;
Patina.Roughness = S;
const FMatterStoneFace Face = MatterStoneFace(MatterMetersFromEngine(Probe), Probe, S, Stone, S);
const FMatterSurface Marked = MatterHistory(Surface, Weather, Place, Response, Snow);
const float Worn = MatterWornFraction(Place, Response);
const FMatterSurface Patinated = MatterPatinate(Marked, MatterPatinaCover(S, S, S, S, Worn), Patina);
const float2 Lobe = MatterBrushedLobe(S, S);
const FMatterPane Pane = MatterPane(S, S, Probe, S);
FMatterMoment Moment;
Moment.Wetness = S;
Moment.SnowCover = S;
const FMatterPlace Open = MatterOpenPlace(Place.NormalWorld, S, S, S);
const FMatterSurface Metal = MatterFilmed(MatterBareMetal(Probe, S), Probe, S, S, MatterWornFilmCover(S, Worn));
const FMatterSurface Moistened = MatterMomentHistory(Metal, Moment, Open, Response, S);
const FMatterStoneFace White = MatterWhiteFace(Probe, S, MatterWhiteStructure(S, S, S, S, S, S), S);
const FMatterSurface Still = MatterStillSurface(S, S);
const FMatterSurface Paint = MatterPaintSurface(Probe, Probe, S, MatterCanvasCoordinates(Probe, Probe, Probe).x);
const FMatterSurface Glazing = MatterClearPane(S, S);
const float3 Library = Moistened.F0 + White.ReliefSlope + Still.F0 + Paint.DiffuseAlbedo + Glazing.F0
	+ MatterFilmedEdgeTint(MatterEdgeTintFromF82(Probe, Probe), MatterLeafCover(S))
	+ MatterSoiledTransmittance(MatterPaneFaceTransmittance(Probe, S), S) + MatterNaturalBed()
	+ MatterMeanFreePathCentimeters(Probe, Probe) + MatterExtinctionPerCentimeter(Probe, Probe)
	+ MatterSingleScatteringAlbedo(Probe, Probe) + MatterSpecularFromF0(S) + MatterFilmF0(Probe);
return Library + Patinated.DiffuseAlbedo + Patinated.F0 + MatterSlopeNormal(Patinated.Slope)
	+ MatterConductorEdgeTint(Probe, Probe) + MatterStrokeTangent(Probe, Place.NormalWorld) + Lobe.xyx
	+ Face.Tone + Face.Roughness + Face.ReliefSlope
	+ Pane.Transmittance + Pane.Reflectance + MatterPaneEdgeColor(Probe, S)
	+ MatterPerpendicular(Place.NormalWorld) + MatterDielectricF0(S) + MatterInternalDiffuseReflectance(S);)");
		Call.Inputs = { FMasterPin{ TEXT("Probe"), 3, FMasterSource{ TEXT("Probe"), NAME_None } } };
		Call.ReturnComponents = 3;

		Recipe.Slab.DiffuseAlbedo = FMasterSource{ TEXT("Library"), NAME_None };
		return Recipe;
	}
}

using namespace Musee::Matter;
using namespace Musee::Matter::Tests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatterRecipeValidates, "Musee.Matter.RecipeValidates", TestFlags)
bool FMatterRecipeValidates::RunTest(const FString& Parameters)
{
	for (const FString& Problem : Validate(StoneProbeRecipe()))
	{
		AddError(FString::Printf(TEXT("The stone probe's recipe: %s"), *Problem));
	}
	for (const FString& Problem : Validate(LibraryProofRecipe()))
	{
		AddError(FString::Printf(TEXT("The library proof's recipe: %s"), *Problem));
	}

	FMasterRecipe WithNumber = StoneProbeRecipe();
	WithNumber.Calls[0].Code.ReplaceInline(TEXT("return Face.Tone;"), TEXT("return Face.Tone * 0.5;"));
	TestTrue(TEXT("A number written in a call is refused"), AnyContains(Validate(WithNumber), TEXT("the number 0.5")));

	FMasterRecipe Unresolved = StoneProbeRecipe();
	Unresolved.Slab.F0 = FMasterSource{ TEXT("History"), TEXT("Nowhere") };
	TestTrue(TEXT("A source the recipe does not define is refused"), AnyContains(Validate(Unresolved), TEXT("does not output")));

	FMasterRecipe Outside = StoneProbeRecipe();
	Outside.Calls[0].Includes.Add(TEXT("/Engine/Private/Common.ush"));
	TestTrue(TEXT("An include outside Matter's library is refused"), AnyContains(Validate(Outside), TEXT("not in Matter's library")));

	FMasterRecipe Twice = StoneProbeRecipe();
	const FMasterParameter Repeated = Twice.Parameters[0];
	Twice.Parameters.Add(Repeated);
	TestTrue(TEXT("A name used twice is refused"), AnyContains(Validate(Twice), TEXT("used twice")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatterIdentityFollowsRecipe, "Musee.Matter.IdentityFollowsRecipe", TestFlags)
bool FMatterIdentityFollowsRecipe::RunTest(const FString& Parameters)
{
	const FMasterIdentity Identity = IdentityOf(StoneProbeRecipe());
	TestTrue(TEXT("The same recipe has the same identity"), Identity.InputHash == IdentityOf(StoneProbeRecipe()).InputHash);

	FMasterRecipe OtherDefault = StoneProbeRecipe();
	OtherDefault.Parameters[0].Default.R += 0.01f;
	TestTrue(TEXT("A changed default changes the identity"), IdentityOf(OtherDefault).InputHash != Identity.InputHash);

	FMasterRecipe OtherCode = StoneProbeRecipe();
	OtherCode.Calls[1].Code += TEXT("\n");
	TestTrue(TEXT("Changed code changes the identity"), IdentityOf(OtherCode).InputHash != Identity.InputHash);

	FMasterRecipe WindowsLines = StoneProbeRecipe();
	WindowsLines.Calls[1].Code.ReplaceInline(TEXT("\n"), TEXT("\r\n"));
	TestTrue(TEXT("Line endings do not change the identity"), IdentityOf(WindowsLines).InputHash == Identity.InputHash);

	TestNotEqual(TEXT("Each role has its own guid"), Identity.StateId(), Identity.Derive(TEXT("Material.LightingGuid")));
	TestTrue(TEXT("The StateId is a valid guid"), Identity.StateId().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatterMasterBuilds, "Musee.Matter.MasterBuilds", TestFlags)
bool FMatterMasterBuilds::RunTest(const FString& Parameters)
{
	const FMasterRecipe Recipe = StoneProbeRecipe();
	const FMasterMade Made = MakeMasterAsset(Recipe);
	for (const FString& Problem : Made.Problems)
	{
		AddError(Problem);
	}
	if (!Made.Succeeded())
	{
		return false;
	}
	TestTrue(TEXT("The package is saved"), IFileManager::Get().FileSize(*Made.Save.Filename) > 0);
	UMaterial* Material = Made.Material;
	TestEqual(TEXT("The master is made at its path"), Material->GetPathName(), PackageNameOf(Recipe) + TEXT(".") + Recipe.AssetName.ToString());

	const UMaterialExpressionSubstrateSlabBSDF* Slab =
		Cast<UMaterialExpressionSubstrateSlabBSDF>(UMaterialEditingLibrary::GetMaterialPropertyInputNode(Material, MP_FrontMaterial));
	if (!TestNotNull(TEXT("The front material is a Substrate Slab"), Slab))
	{
		return false;
	}
	TestTrue(TEXT("The Slab's second lobe comes from the haze"),
		Cast<UMaterialExpressionSubstrateHazinessToSecondaryRoughness>(Slab->SecondRoughness.Expression) != nullptr
		&& Slab->SecondRoughnessWeight.Expression == Slab->SecondRoughness.Expression);
	TestTrue(TEXT("The Slab takes its albedo, F0, roughness and normal"),
		Slab->DiffuseAlbedo.IsConnected() && Slab->F0.IsConnected() && Slab->Roughness.IsConnected() && Slab->Normal.IsConnected());

	bool bIncludesHistory = false;
	for (UMaterialExpression* Expression : UMaterialEditingLibrary::GetMaterialExpressions(Material))
	{
		const UMaterialExpressionCustom* Custom = Cast<UMaterialExpressionCustom>(Expression);
		bIncludesHistory |= Custom && Custom->IncludeFilePaths.Contains(TEXT("/Project/Matter/History.ush"));
	}
	TestTrue(TEXT("A Custom expression includes the surface law, /Project/Matter/History.ush"), bIncludesHistory);

	TArray<FName> Scalars;
	TArray<FName> Vectors;
	UMaterialEditingLibrary::GetScalarParameterNames(Material, Scalars);
	UMaterialEditingLibrary::GetVectorParameterNames(Material, Vectors);
	TestEqual(TEXT("Every parameter of the recipe is a parameter of the master"), Scalars.Num() + Vectors.Num(), Recipe.Parameters.Num());
	TestTrue(TEXT("The master is flagged for Nanite"), Material->GetUsageByFlag(MATUSAGE_Nanite));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatterMasterCompiles, "Musee.Matter.MasterCompiles", TestFlags)
bool FMatterMasterCompiles::RunTest(const FString& Parameters)
{
	const FMasterMade Made = MakeMasterAsset(StoneProbeRecipe());
	for (const FString& Problem : Made.Problems)
	{
		AddError(Problem);
	}
	if (!Made.Succeeded())
	{
		return false;
	}
	const double Start = FPlatformTime::Seconds();
	const FMasterCompile Compile = CompileMasterShaders(*Made.Material, MuseumShaderPlatform, MuseumFeatureLevel);
	for (const FString& Error : Compile.Errors)
	{
		AddError(Error);
	}
	TestTrue(TEXT("The stone probe compiles for PCD3D_SM6"), Compile.bCompiled);
	TestTrue(TEXT("Its shader map holds shaders"), Compile.ShaderCount > 0);
	AddInfo(FString::Printf(TEXT("M_Matter_Stone_Probe: %u shaders for PCD3D_SM6 in %.1f s."), Compile.ShaderCount, FPlatformTime::Seconds() - Start));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatterMasterReproducible, "Musee.Matter.MasterReproducible", TestFlags)
bool FMatterMasterReproducible::RunTest(const FString& Parameters)
{
	const FMasterRecipe Recipe = StoneProbeRecipe();
	const FGuid Expected = IdentityOf(Recipe).StateId();

	const FMasterMade First = MakeMasterAsset(Recipe);
	if (!TestTrue(TEXT("The first make succeeds"), First.Succeeded()))
	{
		return false;
	}
	const FGuid FirstState = First.Material->StateId;
	const TArray<uint8> FirstBytes = ReadPackage(First.Save.Filename);

	const FMasterMade Second = MakeMasterAsset(Recipe);
	if (!TestTrue(TEXT("The second make succeeds"), Second.Succeeded()))
	{
		return false;
	}
	const TArray<uint8> SecondBytes = ReadPackage(Second.Save.Filename);

	TestEqual(TEXT("The StateId is the recipe's"), FirstState, Expected);
	TestEqual(TEXT("A re-make keeps the StateId"), Second.Material->StateId, Expected);
	TestTrue(TEXT("A re-make builds a fresh material"), First.Material != Second.Material);
	TestFalse(TEXT("A re-make of the same recipe writes nothing"), Second.Save.bWritten);
	TestTrue(TEXT("The package is not empty"), FirstBytes.Num() > 0);
	if (FirstBytes != SecondBytes)
	{
		const int32 Shared = FMath::Min(FirstBytes.Num(), SecondBytes.Num());
		int32 Offset = 0;
		while (Offset < Shared && FirstBytes[Offset] == SecondBytes[Offset])
		{
			++Offset;
		}
		AddError(FString::Printf(TEXT("A re-make wrote different bytes: %d and %d bytes, first difference at offset %d."),
			FirstBytes.Num(), SecondBytes.Num(), Offset));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatterLibraryCompiles, "Musee.Matter.LibraryCompiles", TestFlags)
bool FMatterLibraryCompiles::RunTest(const FString& Parameters)
{
	TArray<FString> Problems;
	const FName Name = MakeUniqueObjectName(GetTransientPackage(), UMaterial::StaticClass(), TEXT("M_Matter_Library_Proof"));
	UMaterial* Proof = BuildMaster(LibraryProofRecipe(), GetTransientPackage(), Name, RF_Transient, Problems);
	for (const FString& Problem : Problems)
	{
		AddError(Problem);
	}
	if (!Proof)
	{
		return false;
	}
	const double Start = FPlatformTime::Seconds();
	const FMasterCompile Compile = CompileMasterShaders(*Proof, MuseumShaderPlatform, MuseumFeatureLevel);
	for (const FString& Error : Compile.Errors)
	{
		AddError(Error);
	}
	TestTrue(TEXT("Every function of Matter's shading library compiles for PCD3D_SM6"), Compile.bCompiled);
	AddInfo(FString::Printf(TEXT("The library proof: %u shaders for PCD3D_SM6 in %.1f s."), Compile.ShaderCount, FPlatformTime::Seconds() - Start));
	Proof->MarkAsGarbage();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatterCompileReportsErrors, "Musee.Matter.CompileReportsErrors", TestFlags)
bool FMatterCompileReportsErrors::RunTest(const FString& Parameters)
{
	// A call into a function the library does not have must fail the compile and name the function, or a passing
	// LibraryCompiles would prove nothing.
	FMasterRecipe Broken = LibraryProofRecipe();
	Broken.Calls[0].Code = TEXT("return MatterUndefinedLaw(Probe);");
	TArray<FString> Problems;
	const FName Name = MakeUniqueObjectName(GetTransientPackage(), UMaterial::StaticClass(), TEXT("M_Matter_Broken_Proof"));
	UMaterial* Proof = BuildMaster(Broken, GetTransientPackage(), Name, RF_Transient, Problems);
	if (!TestNotNull(TEXT("The broken proof builds; only its compile fails"), Proof))
	{
		return false;
	}
	AddExpectedMessagePlain(TEXT("Failed to compile Material"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0);
	const FMasterCompile Compile = CompileMasterShaders(*Proof, MuseumShaderPlatform, MuseumFeatureLevel);
	TestFalse(TEXT("A call into a missing function does not compile"), Compile.bCompiled);
	TestTrue(TEXT("The compiler's error names the missing function"), AnyContains(Compile.Errors, TEXT("MatterUndefinedLaw")));
	Proof->MarkAsGarbage();
	return true;
}

#endif
