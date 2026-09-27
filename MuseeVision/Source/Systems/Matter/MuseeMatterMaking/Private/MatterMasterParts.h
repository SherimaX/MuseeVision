#pragma once

#include "CoreMinimal.h"
#include "MatterMasterRecipe.h"

/**
 * The parts every master recipe is built from: its parameters, the reads of the museum Moment, and the surface law's
 * block (History.ush), which every master applies to its surface. A master's own file adds its face and its surface.
 */
namespace Musee::Matter::MasterParts
{
	/** Matter's shading library files, as a call includes them. */
	inline constexpr const TCHAR* LibraryFrame = TEXT("/Project/Matter/Frame.ush");
	inline constexpr const TCHAR* LibraryOptics = TEXT("/Project/Matter/Optics.ush");
	inline constexpr const TCHAR* LibraryHistory = TEXT("/Project/Matter/History.ush");
	inline constexpr const TCHAR* LibraryStone = TEXT("/Project/Matter/Stone.ush");
	inline constexpr const TCHAR* LibraryWhite = TEXT("/Project/Matter/White.ush");
	inline constexpr const TCHAR* LibraryMetal = TEXT("/Project/Matter/Metal.ush");
	inline constexpr const TCHAR* LibraryGlass = TEXT("/Project/Matter/Glass.ush");
	inline constexpr const TCHAR* LibraryMedium = TEXT("/Project/Matter/Medium.ush");
	inline constexpr const TCHAR* LibraryPaint = TEXT("/Project/Matter/Paint.ush");

	/** The parameter groups an instance shows, by the record each group's values come from. */
	inline constexpr const TCHAR* SubstanceGroup = TEXT("Substance");
	inline constexpr const TCHAR* WorkingGroup = TEXT("Working");
	inline constexpr const TCHAR* HistoryGroup = TEXT("History");
	inline constexpr const TCHAR* PieceGroup = TEXT("Piece");

	/** An uninstanced master shows Substrate's own neutral Slab (MaterialExpressionSubstrate.h): its diffuse albedo, its
	 *  roughness, and the index whose normal-incidence reflectance is its F0 of 0.04. */
	inline constexpr float SlabDiffuseAlbedo = 0.18f;
	inline constexpr float SlabRoughness = 0.5f;
	inline constexpr float SlabRefractiveIndex = 1.5f;
	inline constexpr float SlabF0 = 0.04f;
	/** Every other input starts at zero: no structure, no age, no exposure, no deposit. */
	inline constexpr float Unset = 0.0f;
	/** Structure as fine across a bedding as along it. */
	inline constexpr float Unstretched = 1.0f;

	/** The texture a work's image parameter starts from: the engine's own default texture. */
	inline constexpr const TCHAR* EngineDefaultTexture = TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture");

	void AddScalar(FMasterRecipe& Recipe, const TCHAR* Name, const TCHAR* Group, float Default, const TCHAR* Unit, const TCHAR* Meaning);
	void AddColour(FMasterRecipe& Recipe, const TCHAR* Name, const TCHAR* Group, const FLinearColor& Default, const TCHAR* Unit, const TCHAR* Meaning);
	void AddTexture(FMasterRecipe& Recipe, const TCHAR* Name, const TCHAR* Group, const TCHAR* Texture, const TCHAR* Meaning);
	/** The piece's table index, from its Custom Primitive Data. */
	void AddPieceTableIndex(FMasterRecipe& Recipe);

	/** A call input named after the parameter or node it reads. */
	FMasterPin Reads(const TCHAR* Node, int32 Components);
	/** A call input named Pin that reads Node's output Output (none for its first). */
	FMasterPin Reads(const TCHAR* Pin, int32 Components, const TCHAR* Node, const TCHAR* Output);
	/** A call's named output. */
	FMasterPin Writes(const TCHAR* Pin, int32 Components);
	/** A source: Node's output Output (none for its first). */
	FMasterSource From(const TCHAR* Node, const TCHAR* Output = nullptr);

	/** Whether a body's pores take up water (its record gives its porosity and wetting depth), or it is sealed. */
	enum class EPores : uint8 { Open, Sealed };

	/** The history block's parameters, and the Moment's Wetness and SnowCover, with the texture depth the block reads
	 *  named TextureDepth. Add once per recipe; a second block reading another texture depth adds that parameter. */
	void AddHistoryInputs(FMasterRecipe& Recipe, EPores Pores);

	/** The inputs a call applying the surface law reads, with its texture depth from the parameter TextureDepth. */
	TArray<FMasterPin> HistoryPins(EPores Pores, const TCHAR* TextureDepth);

	/** The HLSL that declares the law's Place, Response and Moment from those inputs. */
	FString HistoryDeclarations(EPores Pores, const TCHAR* TextureDepth);

	/** The HLSL that applies the law to the surface named Surface, into the surface named Marked. */
	FString HistoryApplied();

	/** The recipe of a master in /Game/Generated/Matter, reading the Moment from Moment. */
	FMasterRecipe StartRecipe(const FString& AssetName, UMaterialParameterCollection& Moment, const FMasterSettings& Settings);
}
