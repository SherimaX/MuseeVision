#include "MatterMake.h"

#include "Hash/Blake3.h"
#include "MatterMasterAsset.h"
#include "MatterMasterCompile.h"
#include "MatterMomentCollection.h"
#include "MatterNames.h"
#include "MatterRecords.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialParameterCollection.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

namespace Musee::Matter
{
namespace
{
	constexpr EObjectFlags MadeFlags = RF_Public | RF_Standalone | RF_Transactional;

	void Record(FMatterMakeReport& Report, const FString& What, const FMadeSave& Save, const FString& Detail = FString())
	{
		Report.Problems.Append(Save.Problems);
		if (!Save.Succeeded())
		{
			return;
		}
		Save.bWritten ? ++Report.Written : ++Report.Unchanged;
		Report.Lines.Add(FString::Printf(TEXT("%s %s: %lld bytes, BLAKE3 %s%s"), Save.bWritten ? TEXT("Wrote") : TEXT("Unchanged"),
			*What, Save.Bytes, *LexToString(Save.Hash), *Detail));
	}

	FString RecordsText(const FMuseeDependencySet& Inputs)
	{
		TArray<FString> Files;
		for (const TPair<FString, FString>& File : Inputs.Files())
		{
			Files.Add(File.Key + TEXT("=") + File.Value);
		}
		Files.Sort();
		return FString::Join(Files, TEXT(";"));
	}

	void Retire(UObject& Object)
	{
		Object.ClearFlags(RF_Public | RF_Standalone);
		Object.MarkAsGarbage();
	}

	FString CompileText(UMaterial& Material, FMatterMakeReport& Report)
	{
		const FMasterCompile Compile = CompileMasterShaders(Material, MuseumShaderPlatform, MuseumFeatureLevel);
		for (const FString& Error : Compile.Errors)
		{
			Report.Problems.Add(FString::Printf(TEXT("%s: %s"), *Material.GetName(), *Error));
		}
		return FString::Printf(TEXT(", %s for PCD3D_SM6 (%u shaders)"), Compile.bCompiled ? TEXT("compiled") : TEXT("failed"), Compile.ShaderCount);
	}

	void MakeInstance(const FMatterSpec& Spec, const TArray<FMadeMaster>& Masters, FMatterMakeReport& Report)
	{
		const FName Palette = Spec.Palette.ToName();
		const FName Working = Spec.Working.ToName();
		const FName History = Spec.History.ToName();
		const FName AssetName(*InstanceName(Palette, Working, History));
		UPackage* Package = PrepareMadePackage(InstancePackage(Palette, Working, History), AssetName);
		UMaterialInstanceConstant* Instance = NewObject<UMaterialInstanceConstant>(Package, AssetName, MadeFlags);
		const FInstanceComposed Composed = ComposeInstance(Spec, *Instance, [&Masters](const FMasterKind& Kind) -> UMaterialInterface*
		{
			const FMadeMaster* Made = Masters.FindByPredicate([&Kind](const FMadeMaster& Candidate) { return Candidate.Kind == Kind; });
			return Made ? Made->Material : nullptr;
		});
		if (!Composed.Succeeded())
		{
			Report.Problems.Append(Composed.Problems);
			Retire(*Instance);
			return;
		}
		const TMap<FString, FString> Tags = {
			{ MadeTag::Made, Spec.ToString() },
			{ MadeTag::Inputs, Composed.Inputs.Hash() },
			{ MadeTag::Records, RecordsText(Composed.Inputs) },
			{ MadeTag::Generator, FString::Printf(TEXT("musee.matter.instance composer=%d"), InstanceComposerVersion) },
		};
		Record(Report, Instance->GetPathName(), SaveMadeAsset(*Instance, InstanceGuid(Composed, Spec, TEXT("Package.PersistentGuid")), Tags));
	}
}

UMaterialParameterCollection* MakeMomentCollection(FMatterMakeReport& Report)
{
	const FString PackageName = MomentCollectionPackage;
	// Time loads the made collection as the editor starts, which holds its file open: a collection already made with
	// these names is kept as it is, as a re-make of it would write the same thing.
	if (FPackageName::DoesPackageExist(PackageName))
	{
		UMaterialParameterCollection* Made = LoadObject<UMaterialParameterCollection>(nullptr, *ObjectPathOf(PackageName));
		const TArray<FName> Names = MomentParameterNames();
		if (Made && Made->StateId == MomentStateId(Names) && Made->GetScalarParameterNames() == Names && Made->VectorParameters.IsEmpty())
		{
			++Report.Unchanged;
			Report.Lines.Add(FString::Printf(TEXT("Unchanged %s: made with the Moment's names"), *Made->GetPathName()));
			return Made;
		}
	}
	const FName AssetName(*FPackageName::GetShortName(PackageName));
	UPackage* Package = PrepareMadePackage(PackageName, AssetName);
	UMaterialParameterCollection* Moment = NewObject<UMaterialParameterCollection>(Package, AssetName, MadeFlags);
	BuildMomentCollection(*Moment);
	const TMap<FString, FString> Tags = {
		{ MadeTag::Made, PackageName },
		{ MadeTag::Inputs, Moment->StateId.ToString() },
		{ MadeTag::Generator, TEXT("musee.matter.moment") },
	};
	const FMadeSave Save = SaveMadeAsset(*Moment, MomentStateId(MomentParameterNames()), Tags);
	Record(Report, Moment->GetPathName(), Save);
	return Save.Succeeded() ? Moment : nullptr;
}

TArray<FMadeMaster> MakeMasters(UMaterialParameterCollection& Moment, bool bCompile, FMatterMakeReport& Report)
{
	TArray<FMadeMaster> Made;
	for (const FMasterKind& Kind : AllMasters())
	{
		TValueOrError<FMasterRecipe, FString> Recipe = MasterRecipe(Kind, Moment);
		if (Recipe.HasError())
		{
			Report.Problems.Add(Recipe.StealError());
			continue;
		}
		const FMasterMade Master = MakeMasterAsset(Recipe.GetValue());
		Report.Problems.Append(Master.Problems);
		if (!Master.Material)
		{
			continue;
		}
		const FString Compiled = bCompile ? CompileText(*Master.Material, Report) : FString();
		Record(Report, Master.Material->GetPathName(), Master.Save, FString::Printf(TEXT(", StateId %s%s"), *Master.Material->StateId.ToString(), *Compiled));
		Made.Add(FMadeMaster{ Kind, Master.Material });
	}
	return Made;
}

TValueOrError<TArray<FMatterSpec>, FString> PaletteSpecs()
{
	TValueOrError<FMatterPalette, FString> Palette = ReadPalette();
	if (Palette.HasError())
	{
		return MakeError(Palette.StealError());
	}
	TArray<FMatterSpec> Specs;
	for (const FMatterPaletteEntry& Entry : Palette.GetValue().Entries)
	{
		for (const FMatterPaletteFinish& Finish : Entry.Finishes)
		{
			Specs.Add(FMatterSpec{ Entry.Key, Finish.Working, Finish.History });
		}
	}
	return MakeValue(MoveTemp(Specs));
}

void MakeInstances(const TArray<FMadeMaster>& Masters, FMatterMakeReport& Report)
{
	TValueOrError<TArray<FMatterSpec>, FString> Specs = PaletteSpecs();
	if (Specs.HasError())
	{
		Report.Problems.Add(Specs.StealError());
		return;
	}
	for (const FMatterSpec& Spec : Specs.GetValue())
	{
		MakeInstance(Spec, Masters, Report);
	}
}

FMatterMakeReport MakeMatter(bool bCompile)
{
	FMatterMakeReport Report;
	UMaterialParameterCollection* Moment = MakeMomentCollection(Report);
	if (!Moment)
	{
		return Report;
	}
	const TArray<FMadeMaster> Masters = MakeMasters(*Moment, bCompile, Report);
	MakeInstances(Masters, Report);
	return Report;
}
}
