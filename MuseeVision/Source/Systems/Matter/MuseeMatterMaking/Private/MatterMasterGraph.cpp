#include "MatterMasterGraph.h"

#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionLocalPosition.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSubstrate.h"
#include "Materials/MaterialExpressionTransform.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "UObject/UnrealType.h"

namespace Musee::Matter
{
namespace
{
	/** The outputs of Substrate's Haziness-To-Secondary-Roughness, by the names it gives them. */
	constexpr const TCHAR* SecondRoughnessOutput = TEXT("Second Roughness");
	constexpr const TCHAR* SecondRoughnessWeightOutput = TEXT("Second Roughness Weight");

	/** UMaterialInterface keeps its lighting guid in this private reflected property. */
	constexpr const TCHAR* LightingGuidProperty = TEXT("LightingGuid");

	/** The graph under construction: the material, its identity, and each recipe node's expression. */
	struct FGraph
	{
		UMaterial& Material;
		const FMasterIdentity& Identity;
		TArray<FString>& Problems;
		TMap<FName, UMaterialExpression*> Nodes;
	};

	/** Adds an expression named <Kind>_<Node>, with identifiers derived from that name and the recipe's hash. */
	template <typename TExpression>
	TExpression* AddExpression(FGraph& Graph, const TCHAR* Kind, FName Node)
	{
		const FString ObjectName = FString::Printf(TEXT("%s_%s"), Kind, *Node.ToString());
		TExpression* Expression = NewObject<TExpression>(&Graph.Material, FName(*ObjectName), RF_Transactional);
		Graph.Material.GetExpressionCollection().AddExpression(Expression);
		Expression->Material = &Graph.Material;
		Expression->GetMaterialExpressionId() = Graph.Identity.Derive(TEXT("Expression.") + ObjectName);
		if (Expression->bIsParameterExpression)
		{
			Expression->GetParameterExpressionId() = Graph.Identity.Derive(TEXT("Parameter.") + ObjectName);
		}
		return Expression;
	}

	ECustomMaterialOutputType OutputTypeOf(int32 Components)
	{
		switch (Components)
		{
		case 1: return CMOT_Float1;
		case 2: return CMOT_Float2;
		case 3: return CMOT_Float3;
		default: return CMOT_Float4;
		}
	}

	int32 OutputIndexOf(UMaterialExpression& Expression, FName Output)
	{
		return Expression.GetOutputs().IndexOfByPredicate([Output](const FExpressionOutput& Candidate) { return Candidate.OutputName == Output; });
	}

	void Connect(FGraph& Graph, FExpressionInput& Input, const FMasterSource& Source, const FString& Where)
	{
		UMaterialExpression* const* From = Graph.Nodes.Find(Source.Node);
		if (!From)
		{
			Graph.Problems.Add(FString::Printf(TEXT("%s: the builder made no node %s."), *Where, *Source.Node.ToString()));
			return;
		}
		const int32 OutputIndex = Source.Output.IsNone() ? 0 : OutputIndexOf(**From, Source.Output);
		if (OutputIndex == INDEX_NONE)
		{
			Graph.Problems.Add(FString::Printf(TEXT("%s: node %s has no output %s."), *Where, *Source.Node.ToString(), *Source.Output.ToString()));
			return;
		}
		Input.Connect(OutputIndex, *From);
	}

	/** Every node the recipe's pins read, so that only the engine inputs a recipe uses are made. */
	TSet<FName> ReadNodes(const FMasterRecipe& Recipe)
	{
		TSet<FName> Read;
		for (const FMasterCall& Call : Recipe.Calls)
		{
			for (const FMasterPin& Pin : Call.Inputs)
			{
				Read.Add(Pin.Source.Node);
			}
		}
		for (const FMasterTransform& Transform : Recipe.Transforms)
		{
			Read.Add(Transform.Input.Node);
		}
		for (const FMasterSource* Pin : { &Recipe.Slab.DiffuseAlbedo, &Recipe.Slab.F0, &Recipe.Slab.F90, &Recipe.Slab.Roughness,
				&Recipe.Slab.Anisotropy, &Recipe.Slab.Haziness, &Recipe.Slab.Normal, &Recipe.Slab.Tangent })
		{
			Read.Add(Pin->Node);
		}
		return Read;
	}

	void AddEngineInputs(FGraph& Graph, const FMasterRecipe& Recipe)
	{
		const TSet<FName> Read = ReadNodes(Recipe);
		const FName LocalPosition(MasterInput::LocalPosition);
		const FName VertexNormalWorld(MasterInput::VertexNormalWorld);
		if (Read.Contains(LocalPosition))
		{
			Graph.Nodes.Add(LocalPosition, AddExpression<UMaterialExpressionLocalPosition>(Graph, TEXT("Input"), LocalPosition));
		}
		if (Read.Contains(VertexNormalWorld))
		{
			Graph.Nodes.Add(VertexNormalWorld, AddExpression<UMaterialExpressionVertexNormalWS>(Graph, TEXT("Input"), VertexNormalWorld));
		}
	}

	void AddParameters(FGraph& Graph, const FMasterRecipe& Recipe)
	{
		for (const FMasterParameter& Parameter : Recipe.Parameters)
		{
			const FString Description = FString::Printf(TEXT("%s [%s]"), *Parameter.Meaning, *Parameter.Unit);
			const FLinearColor& Default = Parameter.Default;
			if (Parameter.Components == 1)
			{
				UMaterialExpressionScalarParameter* Scalar = AddExpression<UMaterialExpressionScalarParameter>(Graph, TEXT("Parameter"), Parameter.Name);
				Scalar->ParameterName = Parameter.Name;
				Scalar->Group = Parameter.Group;
				Scalar->DefaultValue = Default.R;
				Scalar->Desc = Description;
				Graph.Nodes.Add(Parameter.Name, Scalar);
			}
			else
			{
				UMaterialExpressionVectorParameter* Vector = AddExpression<UMaterialExpressionVectorParameter>(Graph, TEXT("Parameter"), Parameter.Name);
				Vector->ParameterName = Parameter.Name;
				Vector->Group = Parameter.Group;
				Vector->DefaultValue = FLinearColor(Default.R, Default.G, Default.B);
				Vector->Desc = Description;
				Graph.Nodes.Add(Parameter.Name, Vector);
			}
		}
	}

	void AddCalls(FGraph& Graph, const FMasterRecipe& Recipe)
	{
		for (const FMasterCall& Call : Recipe.Calls)
		{
			UMaterialExpressionCustom* Custom = AddExpression<UMaterialExpressionCustom>(Graph, TEXT("Call"), Call.Name);
			Custom->Code = Call.Code;
			Custom->Description = Call.Name.ToString();
			Custom->OutputType = OutputTypeOf(Call.ReturnComponents);
			Custom->IncludeFilePaths = Call.Includes;
			Custom->Inputs.Reset();
			for (const FMasterPin& Pin : Call.Inputs)
			{
				Custom->Inputs.AddDefaulted_GetRef().InputName = Pin.Name;
			}
			Custom->AdditionalOutputs.Reset();
			for (const FMasterPin& Pin : Call.Outputs)
			{
				FCustomOutput& Output = Custom->AdditionalOutputs.AddDefaulted_GetRef();
				Output.OutputName = Pin.Name;
				Output.OutputType = OutputTypeOf(Pin.Components);
			}
			Custom->RebuildOutputs();
			Graph.Nodes.Add(Call.Name, Custom);
		}
	}

	void AddTransforms(FGraph& Graph, const FMasterRecipe& Recipe)
	{
		for (const FMasterTransform& Transform : Recipe.Transforms)
		{
			UMaterialExpressionTransform* Expression = AddExpression<UMaterialExpressionTransform>(Graph, TEXT("Transform"), Transform.Name);
			Expression->TransformSourceType = Transform.From;
			Expression->TransformType = Transform.To;
			Graph.Nodes.Add(Transform.Name, Expression);
		}
	}

	/** Wires every call's inputs and every transform's input, once all nodes exist, so a recipe lists them in any order. */
	void ConnectNodes(FGraph& Graph, const FMasterRecipe& Recipe)
	{
		for (const FMasterCall& Call : Recipe.Calls)
		{
			UMaterialExpressionCustom* Custom = CastChecked<UMaterialExpressionCustom>(Graph.Nodes[Call.Name]);
			for (int32 Index = 0; Index < Call.Inputs.Num(); ++Index)
			{
				const FMasterPin& Pin = Call.Inputs[Index];
				Connect(Graph, Custom->Inputs[Index].Input, Pin.Source, FString::Printf(TEXT("Call %s input %s"), *Call.Name.ToString(), *Pin.Name.ToString()));
			}
		}
		for (const FMasterTransform& Transform : Recipe.Transforms)
		{
			UMaterialExpressionTransform* Expression = CastChecked<UMaterialExpressionTransform>(Graph.Nodes[Transform.Name]);
			Connect(Graph, Expression->Input, Transform.Input, FString::Printf(TEXT("Transform %s"), *Transform.Name.ToString()));
		}
	}

	/** The Slab the master ends in, with haze resolved onto its second specular lobe. */
	UMaterialExpressionSubstrateSlabBSDF* AddSlab(FGraph& Graph, const FMasterSlab& Pins)
	{
		const FName Front(TEXT("Front"));
		UMaterialExpressionSubstrateSlabBSDF* Slab = AddExpression<UMaterialExpressionSubstrateSlabBSDF>(Graph, TEXT("Slab"), Front);
		const TPair<FExpressionInput*, const FMasterSource*> Wires[] = {
			{ &Slab->DiffuseAlbedo, &Pins.DiffuseAlbedo }, { &Slab->F0, &Pins.F0 }, { &Slab->F90, &Pins.F90 },
			{ &Slab->Roughness, &Pins.Roughness }, { &Slab->Anisotropy, &Pins.Anisotropy }, { &Slab->Normal, &Pins.Normal },
			{ &Slab->Tangent, &Pins.Tangent },
		};
		for (const TPair<FExpressionInput*, const FMasterSource*>& Wire : Wires)
		{
			if (Wire.Value->IsSet())
			{
				Connect(Graph, *Wire.Key, *Wire.Value, TEXT("The Slab"));
			}
		}
		if (Pins.Haziness.IsSet())
		{
			UMaterialExpressionSubstrateHazinessToSecondaryRoughness* Haze =
				AddExpression<UMaterialExpressionSubstrateHazinessToSecondaryRoughness>(Graph, TEXT("Haze"), Front);
			Connect(Graph, Haze->BaseRoughness, Pins.Roughness, TEXT("The haze's base roughness"));
			Connect(Graph, Haze->Haziness, Pins.Haziness, TEXT("The haze's haziness"));
			const int32 SecondRoughness = OutputIndexOf(*Haze, SecondRoughnessOutput);
			const int32 SecondRoughnessWeight = OutputIndexOf(*Haze, SecondRoughnessWeightOutput);
			if (SecondRoughness == INDEX_NONE || SecondRoughnessWeight == INDEX_NONE)
			{
				Graph.Problems.Add(TEXT("Substrate's Haziness-To-Secondary-Roughness no longer names its outputs 'Second Roughness' and 'Second Roughness Weight'."));
				return Slab;
			}
			Slab->SecondRoughness.Connect(SecondRoughness, Haze);
			Slab->SecondRoughnessWeight.Connect(SecondRoughnessWeight, Haze);
		}
		return Slab;
	}
}

UMaterial* BuildMaster(const FMasterRecipe& Recipe, UObject* Outer, FName Name, EObjectFlags Flags, TArray<FString>& OutProblems)
{
	OutProblems = Validate(Recipe);
	if (!OutProblems.IsEmpty())
	{
		return nullptr;
	}
	const FMasterIdentity Identity = IdentityOf(Recipe);
	UMaterial* Material = NewObject<UMaterial>(Outer, Name, Flags);
	Material->SetUsageByFlag(MATUSAGE_Nanite, Recipe.bUsedWithNanite);

	FGraph Graph{ *Material, Identity, OutProblems, {} };
	AddEngineInputs(Graph, Recipe);
	AddParameters(Graph, Recipe);
	AddCalls(Graph, Recipe);
	AddTransforms(Graph, Recipe);
	ConnectNodes(Graph, Recipe);
	UMaterialExpressionSubstrateSlabBSDF* Slab = AddSlab(Graph, Recipe.Slab);
	if (!UMaterialEditingLibrary::ConnectMaterialProperty(Slab, FString(), MP_FrontMaterial))
	{
		OutProblems.Add(TEXT("The Slab could not be connected to the material's front material; is Substrate enabled (r.Substrate)?"));
	}
	UMaterialEditingLibrary::LayoutMaterialExpressions(Material);
	if (!OutProblems.IsEmpty())
	{
		Material->ClearFlags(RF_Public | RF_Standalone);
		Material->MarkAsGarbage();
		return nullptr;
	}

	// The StateId is set before the material's one PostEditChange, which is told not to regenerate it; its shaders
	// are then cached under the recipe's identity from the start.
	Material->StateId = Identity.StateId();
	FPropertyChangedEvent Changed(nullptr);
	Material->PostEditChangePropertyInternal(Changed, UMaterial::EPostEditChangeEffectOnShaders::DoesNotInvalidate);
	StampMaterialIdentity(*Material, Identity);
	return Material;
}

void StampMaterialIdentity(UMaterial& Material, const FMasterIdentity& Identity)
{
	Material.StateId = Identity.StateId();
	const FStructProperty* Lighting = FindFProperty<FStructProperty>(UMaterialInterface::StaticClass(), LightingGuidProperty);
	checkf(Lighting && Lighting->Struct == TBaseStructure<FGuid>::Get(),
		TEXT("UMaterialInterface no longer reflects its LightingGuid as an FGuid; the master's identity cannot be stamped."));
	*Lighting->ContainerPtrToValuePtr<FGuid>(&Material) = Identity.Derive(TEXT("Material.LightingGuid"));
}
}
