#include "Tests/MatterTestMasters.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialParameterCollection.h"
#include "MatterMasterGraph.h"
#include "MatterMasters.h"
#include "MatterMomentCollection.h"
#include "MatterNames.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"

namespace Musee::Matter::TestMasters
{
UMaterialParameterCollection* TransientMoment()
{
	const FName Name = MakeUniqueObjectName(GetTransientPackage(), UMaterialParameterCollection::StaticClass(), TEXT("MPC_Moment_Test"));
	UMaterialParameterCollection* Moment = NewObject<UMaterialParameterCollection>(GetTransientPackage(), Name, RF_Transient);
	BuildMomentCollection(*Moment);
	return Moment;
}

FString KindText(const FMasterKind& Kind)
{
	return MasterName(Kind.Master, Kind.Layer);
}

TArray<FMadeMaster> TransientMasters(FAutomationTestBase& Test, UMaterialParameterCollection& Moment)
{
	TArray<FMadeMaster> Made;
	for (const FMasterKind& Kind : AllMasters())
	{
		TValueOrError<FMasterRecipe, FString> Recipe = MasterRecipe(Kind, Moment);
		if (Recipe.HasError())
		{
			Test.AddError(Recipe.GetError());
			continue;
		}
		TArray<FString> Problems;
		const FName Name = MakeUniqueObjectName(GetTransientPackage(), UMaterial::StaticClass(), FName(*KindText(Kind)));
		UMaterial* Material = BuildMaster(Recipe.GetValue(), GetTransientPackage(), Name, RF_Transient, Problems);
		for (const FString& Problem : Problems)
		{
			Test.AddError(FString::Printf(TEXT("%s: %s"), *KindText(Kind), *Problem));
		}
		if (Material)
		{
			Made.Add(FMadeMaster{ Kind, Material });
		}
	}
	return Made;
}

UMaterialInterface* FindIn(const TArray<FMadeMaster>& Masters, const FMasterKind& Kind)
{
	const FMadeMaster* Made = Masters.FindByPredicate([&Kind](const FMadeMaster& Candidate) { return Candidate.Kind == Kind; });
	return Made ? Made->Material : nullptr;
}

void Retire(const TArray<FMadeMaster>& Masters)
{
	for (const FMadeMaster& Made : Masters)
	{
		Made.Material->MarkAsGarbage();
	}
}

UMaterialInstanceConstant* TransientInstance()
{
	const FName Name = MakeUniqueObjectName(GetTransientPackage(), UMaterialInstanceConstant::StaticClass(), TEXT("MI_Test"));
	return NewObject<UMaterialInstanceConstant>(GetTransientPackage(), Name, RF_Transient);
}
}

#endif
