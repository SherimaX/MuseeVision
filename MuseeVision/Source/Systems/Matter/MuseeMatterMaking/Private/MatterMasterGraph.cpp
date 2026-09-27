#include "MatterMasterGraph.h"

#include "MaterialEditingLibrary.h"
#include "Engine/Texture.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCollectionParameter.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionLocalPosition.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionSubstrate.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionTransform.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialParameterCollection.h"
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

	/** The engine's object-bounds node, whose class the engine does not export; it is made by its reflected class. */
	constexpr const TCHAR* LocalBoundsClass = TEXT("/Script/Engine.MaterialExpressionObjectLocalBounds");

	/** The graph under construction: the material, its identity, and each recipe node's expression. */
	struct FGraph
	{
		UMaterial& Material;
		const FMasterIdentity& Identity;
		TArray<FString>& Problems;
		TMap<FName, UMaterialExpression*> Nodes;
	};

	/** Adds an expression of Class named <Kind>_<Node>, with identifiers derived from that name and the recipe's hash. */
	UMaterialExpression* AddExpressionOf(FGraph& Graph, UClass* Class, const TCHAR* Kind, FName Node)
	{
		const FString ObjectName = FString::Printf(TEXT("%s_%s"), Kind, *Node.ToString());
		UMaterialExpression* Expression = NewObject<UMaterialExpression>(&Graph.Material, Class, FName(*ObjectName), RF_Transactional);
		Graph.Material.GetExpressionCollection().AddExpression(Expression);
		Expression->Material = &Graph.Material;
		Expression->GetMaterialExpressionId() = Graph.Identity.Derive(TEXT("Expression.") + ObjectName);
		if (Expression->bIsParameterExpression)
		{
			Expression->GetParameterExpressionId() = Graph.Identity.Derive(TEXT("Parameter.") + ObjectName);
		}
		return Expression;
	}

	template <typename TExpression>
	TExpression* AddExpression(FGraph& Graph, const TCHAR* Kind, FName Node)
	{
		return CastChecked<TExpression>(AddExpressionOf(Graph, TExpression::StaticClass(), Kind, Node));
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
		for (const FMasterTransmittance& Transmittance : Recipe.Transmittances)
		{
			Read.Add(Transmittance.Transmittance.Node);
		}
		for (const FMasterSlab* Slab : { &Recipe.Slab, &Recipe.Layer })
		{
			for (const TPair<const TCHAR*, const FMasterSource*>& Pin : SlabPins(*Slab))
			{
				Read.Add(Pin.Value->Node);
			}
		}
		for (const TPair<const TCHAR*, const FMasterSource*>& Pin : WaterPins(Recipe.Water))
		{
			Read.Add(Pin.Value->Node);
		}
		Read.Add(Recipe.LayerCoverage.Node);
		return Read;
	}

	void AddEngineInputs(FGraph& Graph, const FMasterRecipe& Recipe)
	{
		const TSet<FName> Read = ReadNodes(Recipe);
		const FName LocalPosition(MasterInput::LocalPosition);
		const FName VertexNormalWorld(MasterInput::VertexNormalWorld);
		const FName LocalBounds(MasterInput::LocalBounds);
		const FName TexCoord0(MasterInput::TexCoord0);
		const FName VertexColor(MasterInput::VertexColor);
		if (Read.Contains(TexCoord0))
		{
			// UV channel 0, unscaled: the kernel writes it in metres.
			Graph.Nodes.Add(TexCoord0, AddExpression<UMaterialExpressionTextureCoordinate>(Graph, TEXT("Input"), TexCoord0));
		}
		if (Read.Contains(VertexColor))
		{
			Graph.Nodes.Add(VertexColor, AddExpression<UMaterialExpressionVertexColor>(Graph, TEXT("Input"), VertexColor));
		}
		if (Read.Contains(LocalPosition))
		{
			Graph.Nodes.Add(LocalPosition, AddExpression<UMaterialExpressionLocalPosition>(Graph, TEXT("Input"), LocalPosition));
		}
		if (Read.Contains(VertexNormalWorld))
		{
			Graph.Nodes.Add(VertexNormalWorld, AddExpression<UMaterialExpressionVertexNormalWS>(Graph, TEXT("Input"), VertexNormalWorld));
		}
		if (Read.Contains(LocalBounds))
		{
			UClass* BoundsClass = FindObject<UClass>(nullptr, LocalBoundsClass);
			if (!BoundsClass)
			{
				Graph.Problems.Add(FString::Printf(TEXT("The engine has no class %s."), LocalBoundsClass));
				return;
			}
			Graph.Nodes.Add(LocalBounds, AddExpressionOf(Graph, BoundsClass, TEXT("Input"), LocalBounds));
		}
	}

	void AddScalarParameter(FGraph& Graph, const FMasterParameter& Parameter, const FString& Description)
	{
		UMaterialExpressionScalarParameter* Scalar = AddExpression<UMaterialExpressionScalarParameter>(Graph, TEXT("Parameter"), Parameter.Name);
		Scalar->ParameterName = Parameter.Name;
		Scalar->Group = Parameter.Group;
		Scalar->DefaultValue = Parameter.Default.R;
		Scalar->Desc = Description;
		if (Parameter.PrimitiveDataIndex != INDEX_NONE)
		{
			Scalar->bUseCustomPrimitiveData = true;
			Scalar->PrimitiveDataIndex = static_cast<uint8>(Parameter.PrimitiveDataIndex);
		}
		Graph.Nodes.Add(Parameter.Name, Scalar);
	}

	void AddTextureParameter(FGraph& Graph, const FMasterParameter& Parameter, const FString& Description)
	{
		UMaterialExpressionTextureObjectParameter* Texture = AddExpression<UMaterialExpressionTextureObjectParameter>(Graph, TEXT("Parameter"), Parameter.Name);
		Texture->ParameterName = Parameter.Name;
		Texture->Group = Parameter.Group;
		Texture->Desc = Description;
		Texture->Texture = LoadObject<UTexture>(nullptr, *Parameter.Texture);
		Texture->SamplerType = SAMPLERTYPE_Color;
		if (!Texture->Texture)
		{
			Graph.Problems.Add(FString::Printf(TEXT("Parameter %s: there is no texture %s."), *Parameter.Name.ToString(), *Parameter.Texture));
		}
		Graph.Nodes.Add(Parameter.Name, Texture);
	}

	void AddParameters(FGraph& Graph, const FMasterRecipe& Recipe)
	{
		for (const FMasterParameter& Parameter : Recipe.Parameters)
		{
			const FString Description = FString::Printf(TEXT("%s [%s]"), *Parameter.Meaning, *Parameter.Unit);
			if (Parameter.IsTexture())
			{
				AddTextureParameter(Graph, Parameter, Description);
			}
			else if (Parameter.Components == 1)
			{
				AddScalarParameter(Graph, Parameter, Description);
			}
			else
			{
				UMaterialExpressionVectorParameter* Vector = AddExpression<UMaterialExpressionVectorParameter>(Graph, TEXT("Parameter"), Parameter.Name);
				Vector->ParameterName = Parameter.Name;
				Vector->Group = Parameter.Group;
				Vector->DefaultValue = FLinearColor(Parameter.Default.R, Parameter.Default.G, Parameter.Default.B);
				Vector->Desc = Description;
				Graph.Nodes.Add(Parameter.Name, Vector);
			}
		}
	}

	void AddMomentReads(FGraph& Graph, const FMasterRecipe& Recipe)
	{
		for (const FMasterMomentRead& Read : Recipe.MomentReads)
		{
			UMaterialExpressionCollectionParameter* Expression = AddExpression<UMaterialExpressionCollectionParameter>(Graph, TEXT("Moment"), Read.Name);
			Expression->Collection = Recipe.Moment;
			Expression->ParameterName = Read.Name;
			Expression->ParameterId = Recipe.Moment->GetParameterId(Read.Name);
			if (!Expression->ParameterId.IsValid())
			{
				Graph.Problems.Add(FString::Printf(TEXT("The Moment's collection %s has no parameter %s."), *Recipe.Moment->GetPathName(), *Read.Name.ToString()));
			}
			Graph.Nodes.Add(Read.Name, Expression);
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

	void AddTransformsAndUtilities(FGraph& Graph, const FMasterRecipe& Recipe)
	{
		for (const FMasterTransform& Transform : Recipe.Transforms)
		{
			UMaterialExpressionTransform* Expression = AddExpression<UMaterialExpressionTransform>(Graph, TEXT("Transform"), Transform.Name);
			Expression->TransformSourceType = Transform.From;
			Expression->TransformType = Transform.To;
			Graph.Nodes.Add(Transform.Name, Expression);
		}
		for (const FMasterTransmittance& Transmittance : Recipe.Transmittances)
		{
			Graph.Nodes.Add(Transmittance.Name, AddExpression<UMaterialExpressionSubstrateTransmittanceToMFP>(Graph, TEXT("Transmittance"), Transmittance.Name));
		}
	}

	/** Wires every call's, transform's and utility's inputs, once all nodes exist, so a recipe lists them in any order. */
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
		for (const FMasterTransmittance& Transmittance : Recipe.Transmittances)
		{
			UMaterialExpressionSubstrateTransmittanceToMFP* Expression = CastChecked<UMaterialExpressionSubstrateTransmittanceToMFP>(Graph.Nodes[Transmittance.Name]);
			Connect(Graph, Expression->TransmittanceColor, Transmittance.Transmittance, FString::Printf(TEXT("Transmittance %s"), *Transmittance.Name.ToString()));
		}
	}

	/** A Slab named Role, with haze resolved onto its second specular lobe. */
	UMaterialExpressionSubstrateSlabBSDF* AddSlab(FGraph& Graph, const FMasterSlab& Pins, FName Role)
	{
		UMaterialExpressionSubstrateSlabBSDF* Slab = AddExpression<UMaterialExpressionSubstrateSlabBSDF>(Graph, TEXT("Slab"), Role);
		Slab->SubSurfaceType = Pins.SubSurface;
		const TPair<FExpressionInput*, const FMasterSource*> Wires[] = {
			{ &Slab->DiffuseAlbedo, &Pins.DiffuseAlbedo }, { &Slab->F0, &Pins.F0 }, { &Slab->F90, &Pins.F90 },
			{ &Slab->Roughness, &Pins.Roughness }, { &Slab->Anisotropy, &Pins.Anisotropy }, { &Slab->Normal, &Pins.Normal },
			{ &Slab->Tangent, &Pins.Tangent }, { &Slab->SSSMFP, &Pins.SSSMFP }, { &Slab->FuzzAmount, &Pins.FuzzAmount },
			{ &Slab->FuzzColor, &Pins.FuzzColor }, { &Slab->FuzzRoughness, &Pins.FuzzRoughness },
		};
		for (const TPair<FExpressionInput*, const FMasterSource*>& Wire : Wires)
		{
			if (Wire.Value->IsSet())
			{
				Connect(Graph, *Wire.Key, *Wire.Value, FString::Printf(TEXT("The %s Slab"), *Role.ToString()));
			}
		}
		if (Pins.Haziness.IsSet())
		{
			UMaterialExpressionSubstrateHazinessToSecondaryRoughness* Haze =
				AddExpression<UMaterialExpressionSubstrateHazinessToSecondaryRoughness>(Graph, TEXT("Haze"), Role);
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

	/** The layer's Slab, weighted by its coverage, laid vertically over the base Slab. */
	UMaterialExpression* AddLayered(FGraph& Graph, const FMasterRecipe& Recipe)
	{
		UMaterialExpressionSubstrateSlabBSDF* Base = AddSlab(Graph, Recipe.Slab, TEXT("Base"));
		UMaterialExpressionSubstrateSlabBSDF* Top = AddSlab(Graph, Recipe.Layer, TEXT("Layer"));
		const FName Role(TEXT("Layer"));
		UMaterialExpressionSubstrateWeight* Weight = AddExpression<UMaterialExpressionSubstrateWeight>(Graph, TEXT("Coverage"), Role);
		Weight->A.Connect(0, Top);
		Connect(Graph, Weight->Weight, Recipe.LayerCoverage, TEXT("The layer's coverage"));
		UMaterialExpressionSubstrateVerticalLayering* Layering = AddExpression<UMaterialExpressionSubstrateVerticalLayering>(Graph, TEXT("Vertical"), Role);
		Layering->Top.Connect(0, Weight);
		Layering->Base.Connect(0, Base);
		return Layering;
	}

	UMaterialExpression* AddWater(FGraph& Graph, const FMasterWater& Pins)
	{
		const FName Role(TEXT("Water"));
		UMaterialExpressionSubstrateSingleLayerWaterBSDF* Water = AddExpression<UMaterialExpressionSubstrateSingleLayerWaterBSDF>(Graph, TEXT("Surface"), Role);
		const TPair<FExpressionInput*, const FMasterSource*> Wires[] = {
			{ &Water->Specular, &Pins.Specular }, { &Water->Roughness, &Pins.Roughness }, { &Water->Normal, &Pins.Normal },
			{ &Water->WaterAlbedo, &Pins.WaterAlbedo }, { &Water->WaterExtinction, &Pins.WaterExtinction },
		};
		for (const TPair<FExpressionInput*, const FMasterSource*>& Wire : Wires)
		{
			if (Wire.Value->IsSet())
			{
				Connect(Graph, *Wire.Key, *Wire.Value, TEXT("The water"));
			}
		}
		return Water;
	}

	UMaterialExpression* AddSurface(FGraph& Graph, const FMasterRecipe& Recipe)
	{
		switch (Recipe.Surface)
		{
		case EMasterSurface::LayeredSlab: return AddLayered(Graph, Recipe);
		case EMasterSurface::Water: return AddWater(Graph, Recipe.Water);
		default: return AddSlab(Graph, Recipe.Slab, TEXT("Front"));
		}
	}

	void ApplySettings(UMaterial& Material, const FMasterSettings& Settings)
	{
		Material.BlendMode = Settings.BlendMode;
		Material.SetUsageByFlag(MATUSAGE_Nanite, Settings.bUsedWithNanite);
		Material.TwoSided = Settings.bTwoSided;
		if (Settings.bSurfaceTranslucency)
		{
			Material.TranslucencyLightingMode = TLM_SurfacePerPixelLighting;
			Material.RefractionMethod = RM_None;
		}
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
	ApplySettings(*Material, Recipe.Settings);

	FGraph Graph{ *Material, Identity, OutProblems, {} };
	AddEngineInputs(Graph, Recipe);
	AddParameters(Graph, Recipe);
	AddMomentReads(Graph, Recipe);
	AddCalls(Graph, Recipe);
	AddTransformsAndUtilities(Graph, Recipe);
	ConnectNodes(Graph, Recipe);
	UMaterialExpression* Surface = AddSurface(Graph, Recipe);
	if (!UMaterialEditingLibrary::ConnectMaterialProperty(Surface, FString(), MP_FrontMaterial))
	{
		OutProblems.Add(TEXT("The surface could not be connected to the material's front material; is Substrate enabled (r.Substrate)?"));
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
	StampLightingGuid(Material, Identity.Derive(TEXT("Material.LightingGuid")));
}

void StampLightingGuid(UMaterialInterface& Material, const FGuid& Guid)
{
	const FStructProperty* Lighting = FindFProperty<FStructProperty>(UMaterialInterface::StaticClass(), LightingGuidProperty);
	checkf(Lighting && Lighting->Struct == TBaseStructure<FGuid>::Get(),
		TEXT("UMaterialInterface no longer reflects its LightingGuid as an FGuid; a made material's identity cannot be stamped."));
	*Lighting->ContainerPtrToValuePtr<FGuid>(&Material) = Guid;
}
}
