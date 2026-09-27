#include "MatterMasterRecipe.h"

#include "Containers/StringConv.h"
#include "Materials/MaterialParameterCollection.h"
#include "Misc/Char.h"

namespace Musee::Matter
{
namespace
{
	/** Matter's shading library, where every include of a call must lie. */
	constexpr const TCHAR* LibraryFolder = TEXT("/Project/Matter/");
	constexpr const TCHAR* LibraryExtension = TEXT(".ush");

	/** The widths a Custom expression's pins can take: a texture object, or a scalar up to a four-component vector. */
	constexpr int32 TexturePin = 0;
	constexpr int32 NarrowestPin = 1;
	constexpr int32 WidestPin = 4;

	/** The widths a parameter can take: a texture, a scalar, or a colour of three. */
	constexpr int32 ScalarParameter = 1;
	constexpr int32 ColourParameter = 3;

	/** The outputs of the engine's nodes a recipe may read by name, as MaterialExpressions.cpp and
	 *  MaterialExpressionSubstrate.cpp name them. */
	const TCHAR* const BoundsOutputs[] = { TEXT("Half Extents"), TEXT("Extents"), TEXT("Min"), TEXT("Max") };
	const TCHAR* const TransmittanceOutputs[] = { TEXT("MFP"), TEXT("Thickness") };

	using FTextBuilder = TStringBuilder<8192>;

	FString SourceText(const FMasterSource& Source)
	{
		return Source.Output.IsNone() ? Source.Node.ToString() : Source.Node.ToString() + TEXT(".") + Source.Output.ToString();
	}

	/** A number as text that reads back to the same float, in every locale. */
	FString NumberText(float Value)
	{
		return FString::Printf(TEXT("%.9g"), Value);
	}

	/** Text on one line: line breaks become escapes, so each field of the canonical text is one line. Carriage returns
	 *  are dropped, so a recipe hashes the same whatever line endings its source file was checked out with. */
	FString OneLine(const FString& Text)
	{
		FString Line = Text.Replace(TEXT("\\"), TEXT("\\\\"));
		Line.ReplaceInline(TEXT("\r"), TEXT(""));
		Line.ReplaceInline(TEXT("\n"), TEXT("\\n"));
		return Line;
	}

	void AppendSettings(const FMasterRecipe& Recipe, FTextBuilder& Text)
	{
		const FMasterSettings& S = Recipe.Settings;
		Text << TEXT("blend ") << static_cast<int32>(S.BlendMode) << TEXT(" nanite ") << (S.bUsedWithNanite ? 1 : 0)
			<< TEXT(" surface-translucency ") << (S.bSurfaceTranslucency ? 1 : 0) << TEXT("\n");
		Text << TEXT("moment ") << (Recipe.Moment ? Recipe.Moment->GetPathName() : FString(TEXT("-"))) << TEXT("\n");
	}

	void AppendParameters(const FMasterRecipe& Recipe, FTextBuilder& Text)
	{
		for (const FMasterParameter& Parameter : Recipe.Parameters)
		{
			const FLinearColor& D = Parameter.Default;
			Text << TEXT("parameter ") << Parameter.Name.ToString() << TEXT(" group=") << Parameter.Group.ToString()
				<< TEXT(" components=") << Parameter.Components
				<< TEXT(" default=") << NumberText(D.R) << TEXT(",") << NumberText(D.G) << TEXT(",") << NumberText(D.B)
				<< TEXT(" texture=") << Parameter.Texture << TEXT(" primitive=") << Parameter.PrimitiveDataIndex
				<< TEXT(" unit=") << OneLine(Parameter.Unit) << TEXT(" meaning=") << OneLine(Parameter.Meaning) << TEXT("\n");
		}
		for (const FMasterMomentRead& Read : Recipe.MomentReads)
		{
			Text << TEXT("moment-read ") << Read.Name.ToString() << TEXT("\n");
		}
	}

	void AppendCalls(const FMasterRecipe& Recipe, FTextBuilder& Text)
	{
		for (const FMasterCall& Call : Recipe.Calls)
		{
			Text << TEXT("call ") << Call.Name.ToString() << TEXT(" return=") << Call.ReturnComponents << TEXT("\n");
			for (const FString& Include : Call.Includes)
			{
				Text << TEXT("  include ") << Include << TEXT("\n");
			}
			for (const FMasterPin& Pin : Call.Inputs)
			{
				Text << TEXT("  input ") << Pin.Name.ToString() << TEXT(" ") << Pin.Components << TEXT(" <- ") << SourceText(Pin.Source) << TEXT("\n");
			}
			for (const FMasterPin& Pin : Call.Outputs)
			{
				Text << TEXT("  output ") << Pin.Name.ToString() << TEXT(" ") << Pin.Components << TEXT("\n");
			}
			Text << TEXT("  code ") << OneLine(Call.Code) << TEXT("\n");
		}
	}

	void AppendPins(const TCHAR* Kind, const TArray<TPair<const TCHAR*, const FMasterSource*>>& Pins, FTextBuilder& Text)
	{
		for (const TPair<const TCHAR*, const FMasterSource*>& Pin : Pins)
		{
			Text << Kind << TEXT(" ") << Pin.Key << TEXT(" <- ") << (Pin.Value->IsSet() ? SourceText(*Pin.Value) : FString(TEXT("-"))) << TEXT("\n");
		}
	}

	void AppendNodesAndSurface(const FMasterRecipe& Recipe, FTextBuilder& Text)
	{
		for (const FMasterTransform& Transform : Recipe.Transforms)
		{
			Text << TEXT("transform ") << Transform.Name.ToString() << TEXT(" <- ") << SourceText(Transform.Input)
				<< TEXT(" from=") << static_cast<int32>(Transform.From) << TEXT(" to=") << static_cast<int32>(Transform.To) << TEXT("\n");
		}
		for (const FMasterTransmittance& Transmittance : Recipe.Transmittances)
		{
			Text << TEXT("transmittance ") << Transmittance.Name.ToString() << TEXT(" <- ") << SourceText(Transmittance.Transmittance) << TEXT("\n");
		}
		Text << TEXT("surface ") << static_cast<int32>(Recipe.Surface) << TEXT("\n");
		Text << TEXT("slab subsurface=") << static_cast<int32>(Recipe.Slab.SubSurface) << TEXT("\n");
		AppendPins(TEXT("slab"), SlabPins(Recipe.Slab), Text);
		if (Recipe.Surface == EMasterSurface::LayeredSlab)
		{
			Text << TEXT("layer subsurface=") << static_cast<int32>(Recipe.Layer.SubSurface) << TEXT(" coverage <- ") << SourceText(Recipe.LayerCoverage) << TEXT("\n");
			AppendPins(TEXT("layer"), SlabPins(Recipe.Layer), Text);
		}
		if (Recipe.Surface == EMasterSurface::Water)
		{
			AppendPins(TEXT("water"), WaterPins(Recipe.Water), Text);
		}
	}

	/** Every node name of the recipe, with the outputs each node of several outputs offers. */
	struct FNodeTable
	{
		TMap<FName, TSet<FName>> MultiOutputs;
		TSet<FName> Plain;
		TSet<FName> Textures;
		TArray<FString> Problems;

		void Add(FName Name, const TCHAR* Kind)
		{
			if (Name.IsNone())
			{
				Problems.Add(FString::Printf(TEXT("A %s has no name."), Kind));
			}
			else if (Plain.Contains(Name) || MultiOutputs.Contains(Name))
			{
				Problems.Add(FString::Printf(TEXT("The name %s is used twice; the %s needs its own."), *Name.ToString(), Kind));
			}
		}

		void AddPlain(FName Name, const TCHAR* Kind)
		{
			Add(Name, Kind);
			Plain.Add(Name);
		}

		void AddOutputs(FName Name, const TCHAR* Kind, const TArray<FName>& Outputs)
		{
			Add(Name, Kind);
			MultiOutputs.Add(Name, TSet<FName>(Outputs));
		}

		void Check(const FMasterSource& Source, const FString& Where)
		{
			if (!Source.IsSet())
			{
				Problems.Add(FString::Printf(TEXT("%s has no source."), *Where));
			}
			else if (const TSet<FName>* Outputs = MultiOutputs.Find(Source.Node))
			{
				if (!Source.Output.IsNone() && !Outputs->Contains(Source.Output))
				{
					Problems.Add(FString::Printf(TEXT("%s reads %s, which the node does not output."), *Where, *SourceText(Source)));
				}
			}
			else if (!Plain.Contains(Source.Node))
			{
				Problems.Add(FString::Printf(TEXT("%s reads %s, which the recipe does not define."), *Where, *SourceText(Source)));
			}
			else if (!Source.Output.IsNone())
			{
				Problems.Add(FString::Printf(TEXT("%s reads the output %s of %s, which has only one."), *Where, *Source.Output.ToString(), *Source.Node.ToString()));
			}
		}
	};

	TArray<FName> NamesOf(TArrayView<const TCHAR* const> Texts)
	{
		TArray<FName> Names;
		for (const TCHAR* Text : Texts)
		{
			Names.Add(FName(Text));
		}
		return Names;
	}

	FNodeTable TableOf(const FMasterRecipe& Recipe)
	{
		FNodeTable Table;
		Table.Plain.Add(FName(MasterInput::LocalPosition));
		Table.Plain.Add(FName(MasterInput::VertexNormalWorld));
		Table.MultiOutputs.Add(FName(MasterInput::LocalBounds), TSet<FName>(NamesOf(BoundsOutputs)));
		for (const FMasterParameter& Parameter : Recipe.Parameters)
		{
			Table.AddPlain(Parameter.Name, TEXT("parameter"));
			if (Parameter.IsTexture())
			{
				Table.Textures.Add(Parameter.Name);
			}
		}
		for (const FMasterMomentRead& Read : Recipe.MomentReads)
		{
			Table.AddPlain(Read.Name, TEXT("Moment read"));
		}
		for (const FMasterTransform& Transform : Recipe.Transforms)
		{
			Table.AddPlain(Transform.Name, TEXT("transform"));
		}
		for (const FMasterTransmittance& Transmittance : Recipe.Transmittances)
		{
			Table.AddOutputs(Transmittance.Name, TEXT("transmittance"), NamesOf(TransmittanceOutputs));
		}
		for (const FMasterCall& Call : Recipe.Calls)
		{
			TArray<FName> Outputs;
			for (const FMasterPin& Pin : Call.Outputs)
			{
				Outputs.Add(Pin.Name);
			}
			Table.AddOutputs(Call.Name, TEXT("call"), Outputs);
		}
		return Table;
	}

	/** The first number written in Code, or an empty string. A number is a literal that starts with a digit, or with a
	 *  point followed by a digit, outside an identifier (so float3 and F0 are names, not numbers). */
	FString FirstNumber(const FString& Code)
	{
		const int32 Length = Code.Len();
		for (int32 Index = 0; Index < Length; ++Index)
		{
			const TCHAR Char = Code[Index];
			if (TChar<TCHAR>::IsAlpha(Char) || Char == TEXT('_'))
			{
				while (Index + 1 < Length && (TChar<TCHAR>::IsAlnum(Code[Index + 1]) || Code[Index + 1] == TEXT('_')))
				{
					++Index;
				}
				continue;
			}
			const bool bPointNumber = Char == TEXT('.') && Index + 1 < Length && TChar<TCHAR>::IsDigit(Code[Index + 1]);
			if (TChar<TCHAR>::IsDigit(Char) || bPointNumber)
			{
				int32 End = Index + 1;
				while (End < Length && (TChar<TCHAR>::IsAlnum(Code[End]) || Code[End] == TEXT('.')))
				{
					++End;
				}
				return Code.Mid(Index, End - Index);
			}
		}
		return FString();
	}

	void ValidatePins(const FMasterCall& Call, FNodeTable& Table)
	{
		const FString CallName = Call.Name.ToString();
		TSet<FName> PinNames;
		for (const TArray<FMasterPin>* Pins : { &Call.Inputs, &Call.Outputs })
		{
			for (const FMasterPin& Pin : *Pins)
			{
				bool bAlreadyNamed = false;
				PinNames.Add(Pin.Name, &bAlreadyNamed);
				if (bAlreadyNamed || Pin.Name.IsNone())
				{
					Table.Problems.Add(FString::Printf(TEXT("Call %s has a pin without its own name (%s)."), *CallName, *Pin.Name.ToString()));
				}
				const bool bTexture = Pins == &Call.Inputs && Pin.Components == TexturePin;
				if (!bTexture && (Pin.Components < NarrowestPin || Pin.Components > WidestPin))
				{
					Table.Problems.Add(FString::Printf(TEXT("Call %s pin %s has %d components; a pin has 1 to 4, or 0 for a texture input."), *CallName, *Pin.Name.ToString(), Pin.Components));
				}
			}
		}
		for (const FMasterPin& Pin : Call.Inputs)
		{
			Table.Check(Pin.Source, FString::Printf(TEXT("Call %s input %s"), *CallName, *Pin.Name.ToString()));
			if ((Pin.Components == TexturePin) != Table.Textures.Contains(Pin.Source.Node))
			{
				Table.Problems.Add(FString::Printf(TEXT("Call %s input %s: a texture input reads a texture parameter, and only it does."), *CallName, *Pin.Name.ToString()));
			}
		}
	}

	void ValidateCall(const FMasterCall& Call, FNodeTable& Table)
	{
		const FString CallName = Call.Name.ToString();
		for (const FString& Include : Call.Includes)
		{
			if (!Include.StartsWith(LibraryFolder, ESearchCase::CaseSensitive) || !Include.EndsWith(LibraryExtension, ESearchCase::CaseSensitive))
			{
				Table.Problems.Add(FString::Printf(TEXT("Call %s includes %s, which is not in Matter's library (%s*%s)."), *CallName, *Include, LibraryFolder, LibraryExtension));
			}
		}
		const FString Number = FirstNumber(Call.Code);
		if (!Number.IsEmpty())
		{
			Table.Problems.Add(FString::Printf(TEXT("Call %s holds the number %s; numbers live in the library or in records."), *CallName, *Number));
		}
		if (Call.ReturnComponents < NarrowestPin || Call.ReturnComponents > WidestPin)
		{
			Table.Problems.Add(FString::Printf(TEXT("Call %s returns %d components; a call returns 1 to 4."), *CallName, Call.ReturnComponents));
		}
		ValidatePins(Call, Table);
	}

	void ValidatePinSet(const TArray<TPair<const TCHAR*, const FMasterSource*>>& Pins, const TCHAR* Where, FNodeTable& Table)
	{
		for (const TPair<const TCHAR*, const FMasterSource*>& Pin : Pins)
		{
			if (Pin.Value->IsSet())
			{
				Table.Check(*Pin.Value, FString::Printf(TEXT("%s's %s"), Where, Pin.Key));
			}
		}
	}

	void ValidateSlab(const FMasterSlab& Slab, const TCHAR* Where, FNodeTable& Table)
	{
		ValidatePinSet(SlabPins(Slab), Where, Table);
		if (Slab.Haziness.IsSet() && !Slab.Roughness.IsSet())
		{
			Table.Problems.Add(FString::Printf(TEXT("%s's Haziness is set without its Roughness; haze is a second lobe about the first."), Where));
		}
		if (Slab.SSSMFP.IsSet() != (Slab.SubSurface != MSS_None))
		{
			Table.Problems.Add(FString::Printf(TEXT("%s has a mean free path without a sub-surface type, or the other way round."), Where));
		}
	}

	void ValidateSurface(const FMasterRecipe& Recipe, FNodeTable& Table)
	{
		ValidateSlab(Recipe.Slab, TEXT("The Slab"), Table);
		if (Recipe.Surface == EMasterSurface::LayeredSlab)
		{
			ValidateSlab(Recipe.Layer, TEXT("The layer"), Table);
			Table.Check(Recipe.LayerCoverage, TEXT("The layer's coverage"));
		}
		if (Recipe.Surface == EMasterSurface::Water)
		{
			ValidatePinSet(WaterPins(Recipe.Water), TEXT("The water"), Table);
		}
	}

	void ValidateParameters(const FMasterRecipe& Recipe, FNodeTable& Table)
	{
		for (const FMasterParameter& Parameter : Recipe.Parameters)
		{
			const FString Name = Parameter.Name.ToString();
			if (Parameter.Components != TexturePin && Parameter.Components != ScalarParameter && Parameter.Components != ColourParameter)
			{
				Table.Problems.Add(FString::Printf(TEXT("Parameter %s has %d components; a parameter is a texture (0), a scalar (1) or a colour (3)."), *Name, Parameter.Components));
			}
			if (Parameter.IsTexture() == Parameter.Texture.IsEmpty())
			{
				Table.Problems.Add(FString::Printf(TEXT("Parameter %s: a texture parameter names its starting texture, and only it does."), *Name));
			}
			if (Parameter.PrimitiveDataIndex != INDEX_NONE && Parameter.Components != ScalarParameter)
			{
				Table.Problems.Add(FString::Printf(TEXT("Parameter %s: only a scalar comes from a primitive's data."), *Name));
			}
			if (Parameter.Unit.IsEmpty() || Parameter.Meaning.IsEmpty())
			{
				Table.Problems.Add(FString::Printf(TEXT("Parameter %s does not state its unit and meaning."), *Name));
			}
		}
		if (!Recipe.MomentReads.IsEmpty() && !Recipe.Moment)
		{
			Table.Problems.Add(TEXT("The recipe reads the museum Moment but names no parameter collection."));
		}
	}
}

TArray<TPair<const TCHAR*, const FMasterSource*>> SlabPins(const FMasterSlab& Slab)
{
	return {
		{ TEXT("DiffuseAlbedo"), &Slab.DiffuseAlbedo }, { TEXT("F0"), &Slab.F0 }, { TEXT("F90"), &Slab.F90 },
		{ TEXT("Roughness"), &Slab.Roughness }, { TEXT("Anisotropy"), &Slab.Anisotropy }, { TEXT("Haziness"), &Slab.Haziness },
		{ TEXT("Normal"), &Slab.Normal }, { TEXT("Tangent"), &Slab.Tangent }, { TEXT("SSSMFP"), &Slab.SSSMFP },
	};
}

TArray<TPair<const TCHAR*, const FMasterSource*>> WaterPins(const FMasterWater& Water)
{
	return {
		{ TEXT("Specular"), &Water.Specular }, { TEXT("Roughness"), &Water.Roughness }, { TEXT("Normal"), &Water.Normal },
		{ TEXT("WaterAlbedo"), &Water.WaterAlbedo }, { TEXT("WaterExtinction"), &Water.WaterExtinction },
	};
}

FGuid FMasterIdentity::Derive(FStringView Role) const
{
	const FTCHARToUTF8 RoleText(Role.GetData(), Role.Len());
	FBlake3 Hasher;
	Hasher.Update(InputHash.GetBytes(), sizeof(FBlake3Hash::ByteArray));
	Hasher.Update(RoleText.Get(), RoleText.Length());
	return FGuid::NewGuidFromHash(Hasher.Finalize());
}

FGuid FMasterIdentity::StateId() const
{
	return Derive(TEXT("Material.StateId"));
}

FString CanonicalText(const FMasterRecipe& Recipe)
{
	FTextBuilder Text;
	Text << TEXT("musee.matter.master builder=") << MasterBuilderVersion << TEXT("\n");
	Text << TEXT("package ") << Recipe.PackagePath << TEXT("\n");
	Text << TEXT("asset ") << Recipe.AssetName.ToString() << TEXT("\n");
	AppendSettings(Recipe, Text);
	AppendParameters(Recipe, Text);
	AppendCalls(Recipe, Text);
	AppendNodesAndSurface(Recipe, Text);
	return FString(Text.ToView());
}

FMasterIdentity IdentityOf(const FMasterRecipe& Recipe)
{
	const FString Text = CanonicalText(Recipe);
	const FTCHARToUTF8 Utf8(*Text, Text.Len());
	FMasterIdentity Identity;
	Identity.InputHash = FBlake3::HashBuffer(Utf8.Get(), Utf8.Length());
	return Identity;
}

TArray<FString> Validate(const FMasterRecipe& Recipe)
{
	FNodeTable Table = TableOf(Recipe);
	if (Recipe.PackagePath.IsEmpty() || Recipe.AssetName.IsNone())
	{
		Table.Problems.Add(TEXT("The recipe names no package path or asset."));
	}
	ValidateParameters(Recipe, Table);
	for (const FMasterCall& Call : Recipe.Calls)
	{
		ValidateCall(Call, Table);
	}
	for (const FMasterTransform& Transform : Recipe.Transforms)
	{
		Table.Check(Transform.Input, FString::Printf(TEXT("Transform %s"), *Transform.Name.ToString()));
	}
	for (const FMasterTransmittance& Transmittance : Recipe.Transmittances)
	{
		Table.Check(Transmittance.Transmittance, FString::Printf(TEXT("Transmittance %s"), *Transmittance.Name.ToString()));
	}
	ValidateSurface(Recipe, Table);
	return MoveTemp(Table.Problems);
}

FString PackageNameOf(const FMasterRecipe& Recipe)
{
	return Recipe.PackagePath / Recipe.AssetName.ToString();
}
}
