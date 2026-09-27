#include "MatterMasterParts.h"

#include "MatterMasterParameters.h"
#include "MatterMomentCollection.h"
#include "MatterNames.h"
#include "MatterPieceData.h"

namespace Musee::Matter::MasterParts
{
namespace
{
	using namespace MasterParameter;

	void Add(FMasterRecipe& Recipe, const TCHAR* Name, const TCHAR* Group, int32 Components, const FLinearColor& Default,
		const TCHAR* Unit, const TCHAR* Meaning)
	{
		FMasterParameter& Parameter = Recipe.Parameters.AddDefaulted_GetRef();
		Parameter.Name = FName(Name);
		Parameter.Group = FName(Group);
		Parameter.Components = Components;
		Parameter.Default = Default;
		Parameter.Unit = Unit;
		Parameter.Meaning = Meaning;
	}

	const FLinearColor NeutralGrey(SlabDiffuseAlbedo, SlabDiffuseAlbedo, SlabDiffuseAlbedo);
}

void AddScalar(FMasterRecipe& Recipe, const TCHAR* Name, const TCHAR* Group, float Default, const TCHAR* Unit, const TCHAR* Meaning)
{
	Add(Recipe, Name, Group, 1, FLinearColor(Default, Default, Default), Unit, Meaning);
}

void AddColour(FMasterRecipe& Recipe, const TCHAR* Name, const TCHAR* Group, const FLinearColor& Default, const TCHAR* Unit, const TCHAR* Meaning)
{
	Add(Recipe, Name, Group, 3, Default, Unit, Meaning);
}

void AddTexture(FMasterRecipe& Recipe, const TCHAR* Name, const TCHAR* Group, const TCHAR* Texture, const TCHAR* Meaning)
{
	Add(Recipe, Name, Group, 0, FLinearColor::Black, TEXT("sRGB image"), Meaning);
	Recipe.Parameters.Last().Texture = Texture;
}

void AddPieceTableIndex(FMasterRecipe& Recipe)
{
	AddScalar(Recipe, PieceTableIndex, PieceGroup, Unset, TEXT("count"),
		TEXT("The piece's row in its stage's piece table, from its Custom Primitive Data (Studio's caster); it seeds the piece's pattern"));
	Recipe.Parameters.Last().PrimitiveDataIndex = PieceData::TableIndex;
}

FMasterPin Reads(const TCHAR* Node, int32 Components)
{
	return FMasterPin{ FName(Node), Components, FMasterSource{ FName(Node), NAME_None } };
}

FMasterPin Reads(const TCHAR* Pin, int32 Components, const TCHAR* Node, const TCHAR* Output)
{
	return FMasterPin{ FName(Pin), Components, From(Node, Output) };
}

FMasterPin Writes(const TCHAR* Pin, int32 Components)
{
	return FMasterPin{ FName(Pin), Components, {} };
}

FMasterSource From(const TCHAR* Node, const TCHAR* Output)
{
	return FMasterSource{ FName(Node), Output ? FName(Output) : NAME_None };
}

void AddHistoryInputs(FMasterRecipe& Recipe, EPores Pores)
{
	if (Pores == EPores::Open)
	{
		AddScalar(Recipe, Porosity, SubstanceGroup, Unset, TEXT("1"), TEXT("Open porosity as a volume fraction, from the substance's record"));
		AddScalar(Recipe, WettingDepthMillimeters, SubstanceGroup, Unset, TEXT("mm"), TEXT("Depth of the body whose pores govern its appearance, from the substance's record"));
	}
	AddScalar(Recipe, TextureDepthMillimeters, WorkingGroup, Unset, TEXT("mm"), TEXT("Mean texture depth of the worked face, from the working's record"));
	AddScalar(Recipe, AgeYears, HistoryGroup, Unset, TEXT("yr"), TEXT("Years since the face was worked, from the history's record"));
	AddScalar(Recipe, PassagesPerYear, HistoryGroup, Unset, TEXT("1/yr"), TEXT("Feet or hands passing over the face each year, from the history's record (Site's traffic field later)"));
	AddScalar(Recipe, SkyExposure, HistoryGroup, Unset, TEXT("1"), TEXT("Fraction of free-field precipitation that reaches the face, from the history's record (Form's facts later)"));
	AddScalar(Recipe, KeptWet, HistoryGroup, Unset, TEXT("1"), TEXT("Fraction of its water capacity the face holds whatever the weather, kept wet by water beside it"));
	AddScalar(Recipe, DepositionPerYear, HistoryGroup, Unset, TEXT("1/yr"), TEXT("First-order rate at which deposit covers bare area, from the history's record"));
	AddScalar(Recipe, WashRemovalPerYear, HistoryGroup, Unset, TEXT("1/yr"), TEXT("First-order rate at which a full rain wash clears deposit, from the history's record"));
	AddColour(Recipe, SoilAlbedo, HistoryGroup, NeutralGrey, TEXT("1"), TEXT("Diffuse albedo of the deposit the place lays down, from the history's record"));
	AddScalar(Recipe, WornRoughness, HistoryGroup, SlabRoughness, TEXT("1"), TEXT("The steady roughness abrasion carries the face toward, from the history's record"));
	AddScalar(Recipe, WornHaziness, HistoryGroup, Unset, TEXT("1"), TEXT("The steady haziness of the worn face, from the history's record"));
	AddScalar(Recipe, WearPassages, HistoryGroup, Unset, TEXT("passages"), TEXT("Passages that carry the face 63 percent of the way to its worn state (0: it never wears)"));
	Recipe.MomentReads.Add(FMasterMomentRead{ FName(MomentParameter::Wetness) });
	Recipe.MomentReads.Add(FMasterMomentRead{ FName(MomentParameter::SnowCover) });
}

TArray<FMasterPin> HistoryPins(EPores Pores, const TCHAR* TextureDepth)
{
	TArray<FMasterPin> Pins = {
		Reads(TEXT("NormalWorld"), 3, MasterInput::VertexNormalWorld, nullptr),
		Reads(SkyExposure, 1), Reads(PassagesPerYear, 1), Reads(AgeYears, 1), Reads(KeptWet, 1),
		Reads(TextureDepth, 1), Reads(WornRoughness, 1), Reads(WornHaziness, 1), Reads(WearPassages, 1),
		Reads(SoilAlbedo, 3), Reads(DepositionPerYear, 1), Reads(WashRemovalPerYear, 1),
		Reads(MomentParameter::Wetness, 1), Reads(MomentParameter::SnowCover, 1),
	};
	if (Pores == EPores::Open)
	{
		Pins.Add(Reads(Porosity, 1));
		Pins.Add(Reads(WettingDepthMillimeters, 1));
	}
	return Pins;
}

FString HistoryDeclarations(EPores Pores, const TCHAR* TextureDepth)
{
	const bool bOpen = Pores == EPores::Open;
	return FString::Printf(TEXT(R"(const FMatterPlace Place = MatterOpenPlace(NormalWorld, SkyExposure, PassagesPerYear, AgeYears);
FMatterResponse Response;
Response.Porosity = %s;
Response.WettingDepthMillimeters = %s;
Response.TextureDepthMillimeters = %s;
Response.WornRoughness = WornRoughness;
Response.WornHaziness = WornHaziness;
Response.WearPassages = WearPassages;
Response.SoilAlbedo = SoilAlbedo;
Response.DepositionPerYear = DepositionPerYear;
Response.WashRemovalPerYear = WashRemovalPerYear;
FMatterMoment Moment;
Moment.Wetness = Wetness;
Moment.SnowCover = SnowCover;
)"),
		bOpen ? Porosity : TEXT("MatterNoPores"),
		bOpen ? WettingDepthMillimeters : TEXT("MatterNoPores"),
		TextureDepth);
}

FString HistoryApplied()
{
	return TEXT("const FMatterSurface Marked = MatterMomentHistory(Surface, Moment, Place, Response, KeptWet);\n");
}

FMasterRecipe StartRecipe(const FString& AssetName, UMaterialParameterCollection& Moment, const FMasterSettings& Settings)
{
	FMasterRecipe Recipe;
	Recipe.PackagePath = GeneratedFolder;
	Recipe.AssetName = FName(*AssetName);
	Recipe.Settings = Settings;
	Recipe.Moment = &Moment;
	return Recipe;
}
}
