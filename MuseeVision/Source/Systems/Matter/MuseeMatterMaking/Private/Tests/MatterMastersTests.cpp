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
#include "Tests/MatterTestMasters.h"
#include "UObject/Package.h"

namespace Musee::Matter::MastersTests
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
}

using namespace Musee::Matter;
using namespace Musee::Matter::MastersTests;
using namespace Musee::Matter::TestMasters;

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
					|| Include == TEXT("/Project/Matter/Glass.ush") || Include == TEXT("/Project/Matter/Medium.ush") || Include == TEXT("/Project/Matter/Paint.ush")
					|| Include == TEXT("/Project/Matter/Sheet.ush") || Include == TEXT("/Project/Matter/Plate.ush");
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
		const FInstanceComposed ComposedA = ComposeInstance(Spec, *A, [&First](const FMasterKind& Kind) { return FindIn(First, Kind); });
		const FInstanceComposed ComposedB = ComposeInstance(Spec, *B, [&First](const FMasterKind& Kind) { return FindIn(First, Kind); });
		TestEqual(FString::Printf(TEXT("%s: the same parameters"), *Spec.ToString()), ComposedA.Written, ComposedB.Written);
		TestEqual(FString::Printf(TEXT("%s: the same ParameterStateId"), *Spec.ToString()), A->ParameterStateId, B->ParameterStateId);
		TestEqual(FString::Printf(TEXT("%s: the same records read"), *Spec.ToString()), ComposedA.Inputs.Hash(), ComposedB.Inputs.Hash());
		A->MarkAsGarbage();
		B->MarkAsGarbage();
	}
	Retire(First);
	Retire(Second);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatterPaletteInstancesCompose, "Musee.Matter.PaletteInstancesCompose", TestFlags)
bool FMatterPaletteInstancesCompose::RunTest(const FString& Parameters)
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
		const FInstanceComposed Composed = ComposeInstance(Spec, *Instance, [&Masters](const FMasterKind& Kind) { return FindIn(Masters, Kind); });
		for (const FString& Problem : Composed.Problems)
		{
			AddError(Problem);
		}
		TestTrue(FString::Printf(TEXT("%s is parented to its master"), *Spec.ToString()), Instance->Parent == FindIn(Masters, Composed.Master));
		TestFalse(FString::Printf(TEXT("%s read its records"), *Spec.ToString()), Composed.Inputs.IsEmpty());
		Instance->MarkAsGarbage();
	}

	// A spec the palette cannot shade is a problem, never a default.
	UMaterialInstanceConstant* Unknown = TransientInstance();
	const FInstanceComposed Refused = ComposeInstance(FMatterSpec{ FMuseeKey(TEXT("stone.travertine-romano")), FMuseeKey(TEXT("mirror")), FMuseeKey(TEXT("clean")) },
		*Unknown, [&Masters](const FMasterKind& Kind) { return FindIn(Masters, Kind); });
	TestFalse(TEXT("A working with no record is refused"), Refused.Succeeded());
	TestNull(TEXT("A refused instance has no parent"), Unknown->Parent.Get());
	Unknown->MarkAsGarbage();
	Retire(Masters);
	return true;
}

#endif
