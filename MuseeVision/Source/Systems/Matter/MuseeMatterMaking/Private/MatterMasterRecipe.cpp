#include "MatterMasterRecipe.h"

#include "Containers/StringConv.h"
#include "Misc/Char.h"

namespace Musee::Matter
{
namespace
{
	/** Matter's shading library, where every include of a call must lie. */
	constexpr const TCHAR* LibraryFolder = TEXT("/Project/Matter/");
	constexpr const TCHAR* LibraryExtension = TEXT(".ush");

	/** The widths a Custom expression's pins can take: a scalar up to a four-component vector. */
	constexpr int32 NarrowestPin = 1;
	constexpr int32 WidestPin = 4;

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

	void AppendParameters(const FMasterRecipe& Recipe, TStringBuilder<4096>& Text)
	{
		for (const FMasterParameter& Parameter : Recipe.Parameters)
		{
			const FLinearColor& D = Parameter.Default;
			Text << TEXT("parameter ") << Parameter.Name.ToString() << TEXT(" group=") << Parameter.Group.ToString()
				<< TEXT(" components=") << Parameter.Components
				<< TEXT(" default=") << NumberText(D.R) << TEXT(",") << NumberText(D.G) << TEXT(",") << NumberText(D.B)
				<< TEXT(" unit=") << OneLine(Parameter.Unit) << TEXT(" meaning=") << OneLine(Parameter.Meaning) << TEXT("\n");
		}
	}

	void AppendCalls(const FMasterRecipe& Recipe, TStringBuilder<4096>& Text)
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

	void AppendTransformsAndSlab(const FMasterRecipe& Recipe, TStringBuilder<4096>& Text)
	{
		for (const FMasterTransform& Transform : Recipe.Transforms)
		{
			Text << TEXT("transform ") << Transform.Name.ToString() << TEXT(" <- ") << SourceText(Transform.Input)
				<< TEXT(" from=") << static_cast<int32>(Transform.From) << TEXT(" to=") << static_cast<int32>(Transform.To) << TEXT("\n");
		}
		const TPair<const TCHAR*, const FMasterSource*> Pins[] = {
			{ TEXT("DiffuseAlbedo"), &Recipe.Slab.DiffuseAlbedo }, { TEXT("F0"), &Recipe.Slab.F0 }, { TEXT("F90"), &Recipe.Slab.F90 },
			{ TEXT("Roughness"), &Recipe.Slab.Roughness }, { TEXT("Anisotropy"), &Recipe.Slab.Anisotropy },
			{ TEXT("Haziness"), &Recipe.Slab.Haziness }, { TEXT("Normal"), &Recipe.Slab.Normal }, { TEXT("Tangent"), &Recipe.Slab.Tangent },
		};
		for (const TPair<const TCHAR*, const FMasterSource*>& Pin : Pins)
		{
			Text << TEXT("slab ") << Pin.Key << TEXT(" <- ") << (Pin.Value->IsSet() ? SourceText(*Pin.Value) : FString(TEXT("-"))) << TEXT("\n");
		}
	}

	/** Every node name of the recipe, with the call outputs each call offers. */
	struct FNodeTable
	{
		TMap<FName, TSet<FName>> CallOutputs;
		TSet<FName> Plain;
		TArray<FString> Problems;

		void Add(FName Name, const TCHAR* Kind)
		{
			if (Name.IsNone())
			{
				Problems.Add(FString::Printf(TEXT("A %s has no name."), Kind));
			}
			else if (Plain.Contains(Name) || CallOutputs.Contains(Name))
			{
				Problems.Add(FString::Printf(TEXT("The name %s is used twice; the %s needs its own."), *Name.ToString(), Kind));
			}
		}

		void Check(const FMasterSource& Source, const FString& Where)
		{
			if (!Source.IsSet())
			{
				Problems.Add(FString::Printf(TEXT("%s has no source."), *Where));
			}
			else if (const TSet<FName>* Outputs = CallOutputs.Find(Source.Node))
			{
				if (!Source.Output.IsNone() && !Outputs->Contains(Source.Output))
				{
					Problems.Add(FString::Printf(TEXT("%s reads %s, which the call does not output."), *Where, *SourceText(Source)));
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

	FNodeTable TableOf(const FMasterRecipe& Recipe)
	{
		FNodeTable Table;
		for (const TCHAR* Input : { MasterInput::LocalPosition, MasterInput::VertexNormalWorld })
		{
			Table.Plain.Add(FName(Input));
		}
		for (const FMasterParameter& Parameter : Recipe.Parameters)
		{
			Table.Add(Parameter.Name, TEXT("parameter"));
			Table.Plain.Add(Parameter.Name);
		}
		for (const FMasterTransform& Transform : Recipe.Transforms)
		{
			Table.Add(Transform.Name, TEXT("transform"));
			Table.Plain.Add(Transform.Name);
		}
		for (const FMasterCall& Call : Recipe.Calls)
		{
			Table.Add(Call.Name, TEXT("call"));
			TSet<FName>& Outputs = Table.CallOutputs.Add(Call.Name);
			for (const FMasterPin& Pin : Call.Outputs)
			{
				Outputs.Add(Pin.Name);
			}
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
				if (Pin.Components < NarrowestPin || Pin.Components > WidestPin)
				{
					Table.Problems.Add(FString::Printf(TEXT("Call %s pin %s has %d components; a pin has 1 to 4."), *CallName, *Pin.Name.ToString(), Pin.Components));
				}
			}
		}
		for (const FMasterPin& Pin : Call.Inputs)
		{
			Table.Check(Pin.Source, FString::Printf(TEXT("Call %s input %s"), *CallName, *Pin.Name.ToString()));
		}
	}

	void ValidateSlab(const FMasterSlab& Slab, FNodeTable& Table)
	{
		const TPair<const TCHAR*, const FMasterSource*> Pins[] = {
			{ TEXT("DiffuseAlbedo"), &Slab.DiffuseAlbedo }, { TEXT("F0"), &Slab.F0 }, { TEXT("F90"), &Slab.F90 },
			{ TEXT("Roughness"), &Slab.Roughness }, { TEXT("Anisotropy"), &Slab.Anisotropy }, { TEXT("Haziness"), &Slab.Haziness },
			{ TEXT("Normal"), &Slab.Normal }, { TEXT("Tangent"), &Slab.Tangent },
		};
		for (const TPair<const TCHAR*, const FMasterSource*>& Pin : Pins)
		{
			if (Pin.Value->IsSet())
			{
				Table.Check(*Pin.Value, FString::Printf(TEXT("The Slab's %s"), Pin.Key));
			}
		}
		if (Slab.Haziness.IsSet() && !Slab.Roughness.IsSet())
		{
			Table.Problems.Add(TEXT("The Slab's Haziness is set without its Roughness; haze is a second lobe about the first."));
		}
	}
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
	TStringBuilder<4096> Text;
	Text << TEXT("musee.matter.master builder=") << MasterBuilderVersion << TEXT("\n");
	Text << TEXT("package ") << Recipe.PackagePath << TEXT("\n");
	Text << TEXT("asset ") << Recipe.AssetName.ToString() << TEXT("\n");
	Text << TEXT("nanite ") << (Recipe.bUsedWithNanite ? 1 : 0) << TEXT("\n");
	AppendParameters(Recipe, Text);
	AppendCalls(Recipe, Text);
	AppendTransformsAndSlab(Recipe, Text);
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
	for (const FMasterParameter& Parameter : Recipe.Parameters)
	{
		if (Parameter.Components != 1 && Parameter.Components != 3)
		{
			Table.Problems.Add(FString::Printf(TEXT("Parameter %s has %d components; a parameter is a scalar or a colour of 3."), *Parameter.Name.ToString(), Parameter.Components));
		}
		if (Parameter.Unit.IsEmpty() || Parameter.Meaning.IsEmpty())
		{
			Table.Problems.Add(FString::Printf(TEXT("Parameter %s does not state its unit and meaning."), *Parameter.Name.ToString()));
		}
	}
	for (const FMasterCall& Call : Recipe.Calls)
	{
		ValidateCall(Call, Table);
	}
	for (const FMasterTransform& Transform : Recipe.Transforms)
	{
		Table.Check(Transform.Input, FString::Printf(TEXT("Transform %s"), *Transform.Name.ToString()));
	}
	ValidateSlab(Recipe.Slab, Table);
	return MoveTemp(Table.Problems);
}

FString PackageNameOf(const FMasterRecipe& Recipe)
{
	return Recipe.PackagePath / Recipe.AssetName.ToString();
}
}
