#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "HAL/PlatformTime.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialParameterCollection.h"
#include "MatterInstances.h"
#include "MatterMake.h"
#include "MatterMasterCompile.h"
#include "MatterMasterGraph.h"
#include "MatterMasters.h"
#include "MatterMomentCollection.h"
#include "MatterNames.h"
#include "UObject/Package.h"

namespace Musee::Matter::MastersTests
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	/** The museum Moment's collection, made in memory as the make step makes it. */
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

	/** Every master, built in memory over Moment; a master that does not build is reported on Test. */
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

using namespace Musee::Matter;
using namespace Musee::Matter::MastersTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatterMomentCollection, "Musee.Matter.MomentCollection", TestFlags)
bool FMatterMomentCollection::RunTest(const FString& Parameters)
{
	UMaterialParameterCollection* First = TransientMoment();
	UMaterialParameterCollection* Second = TransientMoment();
	const TArray<FName> Contract = {
		TEXT("SunElevationDeg"), TEXT("SunAzimuthDeg"), TEXT("Daylight"), TEXT("RainRateMmH"), TEXT("Wetness"), TEXT("SnowCover"),
		TEXT("TemperatureC"), TEXT("WindSpeedMS"), TEXT("SolarTerm"), TEXT("GivernyWetness"), TEXT("LondonWetness"),
	};
	TestTrue(TEXT("The collection holds the stage contract's scalars, in order"), First->GetScalarParameterNames() == Contract);
	TestEqual(TEXT("It holds no vectors"), First->VectorParameters.Num(), 0);
	TestEqual(TEXT("Its StateId is the names' own"), First->StateId, Second->StateId);
	for (const FName Name : Contract)
	{
		TestEqual(FString::Printf(TEXT("%s has the same id in every make"), *Name.ToString()), First->GetParameterId(Name), Second->GetParameterId(Name));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatterMastersValidate, "Musee.Matter.MastersValidate", TestFlags)
bool FMatterMastersValidate::RunTest(const FString& Parameters)
{
	UMaterialParameterCollection* Moment = TransientMoment();
	const TArray<FMadeMaster> Masters = TransientMasters(*this, *Moment);
	TestEqual(TEXT("Every master builds"), Masters.Num(), AllMasters().Num());
	for (const FMadeMaster& Made : Masters)
	{
		bool bHistory = false;
		for (UMaterialExpression* Expression : UMaterialEditingLibrary::GetMaterialExpressions(Made.Material))
		{
			const UMaterialExpressionCustom* Custom = Cast<UMaterialExpressionCustom>(Expression);
			for (const FString& Include : Custom ? Custom->IncludeFilePaths : TArray<FString>())
			{
				bHistory |= Include == TEXT("/Project/Matter/History.ush") || Include == TEXT("/Project/Matter/Metal.ush")
					|| Include == TEXT("/Project/Matter/Glass.ush") || Include == TEXT("/Project/Matter/Medium.ush") || Include == TEXT("/Project/Matter/Paint.ush");
			}
		}
		// A master has no emissive input by construction: a recipe's surfaces have no emissive pin.
		TestTrue(FString::Printf(TEXT("%s applies the surface law (History.ush, directly or through its family's file)"), *KindText(Made.Kind)), bHistory);
	}
	Retire(Masters);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatterMastersCompile, "Musee.Matter.MastersCompile", TestFlags)
bool FMatterMastersCompile::RunTest(const FString& Parameters)
{
	UMaterialParameterCollection* Moment = TransientMoment();
	const TArray<FMadeMaster> Masters = TransientMasters(*this, *Moment);
	for (const FMadeMaster& Made : Masters)
	{
		const double Start = FPlatformTime::Seconds();
		const FMasterCompile Compile = CompileMasterShaders(*Made.Material, MuseumShaderPlatform, MuseumFeatureLevel);
		for (const FString& Error : Compile.Errors)
		{
			AddError(FString::Printf(TEXT("%s: %s"), *KindText(Made.Kind), *Error));
		}
		TestTrue(FString::Printf(TEXT("%s compiles for PCD3D_SM6"), *KindText(Made.Kind)), Compile.bCompiled);
		AddInfo(FString::Printf(TEXT("%s: %u shaders for PCD3D_SM6 in %.1f s."), *KindText(Made.Kind), Compile.ShaderCount, FPlatformTime::Seconds() - Start));
	}
	Retire(Masters);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatterDeterministic, "Musee.Matter.Deterministic", TestFlags)
bool FMatterDeterministic::RunTest(const FString& Parameters)
{
	UMaterialParameterCollection* Moment = TransientMoment();
	const TArray<FMadeMaster> First = TransientMasters(*this, *Moment);
	const TArray<FMadeMaster> Second = TransientMasters(*this, *Moment);
	for (int32 Index = 0; Index < FMath::Min(First.Num(), Second.Num()); ++Index)
	{
		UMaterial& A = *First[Index].Material;
		UMaterial& B = *Second[Index].Material;
		const FString Master = KindText(First[Index].Kind);
		TestEqual(FString::Printf(TEXT("%s: the same StateId"), *Master), A.StateId, B.StateId);
		const TArray<UMaterialExpression*> ExpressionsA = UMaterialEditingLibrary::GetMaterialExpressions(&A);
		const TArray<UMaterialExpression*> ExpressionsB = UMaterialEditingLibrary::GetMaterialExpressions(&B);
		if (TestEqual(FString::Printf(TEXT("%s: the same expressions"), *Master), ExpressionsA.Num(), ExpressionsB.Num()))
		{
			for (int32 E = 0; E < ExpressionsA.Num(); ++E)
			{
				TestEqual(FString::Printf(TEXT("%s: expression %s keeps its guid"), *Master, *ExpressionsA[E]->GetName()),
					ExpressionsA[E]->GetMaterialExpressionId(), ExpressionsB[E]->GetMaterialExpressionId());
			}
		}
	}

	TValueOrError<TArray<FMatterSpec>, FString> Specs = PaletteSpecs();
	if (!TestTrue(TEXT("The palette's finishes read"), Specs.HasValue()))
	{
		return false;
	}
	for (const FMatterSpec& Spec : Specs.GetValue())
	{
		UMaterialInstanceConstant* A = TransientInstance();
		UMaterialInstanceConstant* B = TransientInstance();
		const FInstanceFilled FilledA = FillInstance(Spec, *A, [&First](const FMasterKind& Kind) { return FindIn(First, Kind); });
		const FInstanceFilled FilledB = FillInstance(Spec, *B, [&First](const FMasterKind& Kind) { return FindIn(First, Kind); });
		TestEqual(FString::Printf(TEXT("%s: the same parameters"), *Spec.ToString()), FilledA.Written, FilledB.Written);
		TestEqual(FString::Printf(TEXT("%s: the same ParameterStateId"), *Spec.ToString()), A->ParameterStateId, B->ParameterStateId);
		TestEqual(FString::Printf(TEXT("%s: the same records read"), *Spec.ToString()), FilledA.Inputs.Hash(), FilledB.Inputs.Hash());
		A->MarkAsGarbage();
		B->MarkAsGarbage();
	}
	Retire(First);
	Retire(Second);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatterPaletteInstancesFill, "Musee.Matter.PaletteInstancesFill", TestFlags)
bool FMatterPaletteInstancesFill::RunTest(const FString& Parameters)
{
	UMaterialParameterCollection* Moment = TransientMoment();
	const TArray<FMadeMaster> Masters = TransientMasters(*this, *Moment);
	TValueOrError<TArray<FMatterSpec>, FString> Specs = PaletteSpecs();
	if (!TestTrue(TEXT("The palette's finishes read"), Specs.HasValue()))
	{
		AddError(Specs.GetError());
		return false;
	}
	TestTrue(TEXT("The palette offers finishes"), Specs.GetValue().Num() > 0);
	for (const FMatterSpec& Spec : Specs.GetValue())
	{
		UMaterialInstanceConstant* Instance = TransientInstance();
		const FInstanceFilled Filled = FillInstance(Spec, *Instance, [&Masters](const FMasterKind& Kind) { return FindIn(Masters, Kind); });
		for (const FString& Problem : Filled.Problems)
		{
			AddError(Problem);
		}
		TestTrue(FString::Printf(TEXT("%s is parented to its master"), *Spec.ToString()), Instance->Parent == FindIn(Masters, Filled.Master));
		TestFalse(FString::Printf(TEXT("%s read its records"), *Spec.ToString()), Filled.Inputs.IsEmpty());
		Instance->MarkAsGarbage();
	}

	// A spec the palette cannot shade is a problem, never a default.
	UMaterialInstanceConstant* Unknown = TransientInstance();
	const FInstanceFilled Refused = FillInstance(FMatterSpec{ FMuseeKey(TEXT("stone.travertine-romano")), FMuseeKey(TEXT("mirror")), FMuseeKey(TEXT("clean")) },
		*Unknown, [&Masters](const FMasterKind& Kind) { return FindIn(Masters, Kind); });
	TestFalse(TEXT("A working with no record is refused"), Refused.Succeeded());
	TestNull(TEXT("A refused instance has no parent"), Unknown->Parent.Get());
	Unknown->MarkAsGarbage();
	Retire(Masters);
	return true;
}

#endif
