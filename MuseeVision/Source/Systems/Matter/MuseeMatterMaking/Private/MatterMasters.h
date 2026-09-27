#pragma once

#include "CoreMinimal.h"
#include "MatterMasterRecipe.h"
#include "Templates/ValueOrError.h"

class UMaterialParameterCollection;

/**
 * Matter's master materials: one per Substrate shading topology, each justified by a physical difference, never by a
 * room (D-015). A substance's typed record names its master.
 *
 * - stone: an opaque dielectric of bedded grains (travertine), its face drawn from its bedding (Stone.ush);
 * - marble: the same, translucent: light diffuses millimetres into calcite before it leaves (Medium.ush);
 * - white: an opaque, porous, unbedded body (lime plaster, cast plaster, limewash; White.ush);
 * - metal: a conductor with its measured reflection, a brushed grain, and a patina or tarnish film (Metal.ush);
 * - glass: a thin transmitting pane, absorbing along its thickness (Glass.ush);
 * - water: a still body of water seen through its surface, as Single Layer Water (Medium.ush);
 * - paint: a painted canvas carrying the work's own image under its varnish (Paint.ush);
 * - wood: an opaque fibrous body with growth rings and rays, its grain along each board, under an oil or wax that soaks
 *   into it (Wood.ush);
 * - cloth: an opaque textile, its dye in its fibres and the sheen of its standing fibre ends on a fuzz layer (Cloth.ush);
 * - sheet: a thin sheet that scatters the light it passes, lit from both faces (a linen velarium, an opal pane;
 *   Sheet.ush);
 * - living: a plant's thin tissue, its species' colour from its vertices, lit from both faces (Living.ush).
 *
 * A painted or hard-chromed steel needs no master of its own: paint is a film the working lays over the metal at full
 * cover (worn through where it is worn), and chromium is a metal with its own measured reflection (Metal.ush).
 *
 * A layer working (gilt) is carried as a Substrate vertical layer over the masters that host it (white, metal and wood:
 * leaf laid on gesso, on bronze, and on oak or lacquered pine sealed and sized): its masters are M_Matter_WhiteGilt,
 * M_Matter_MetalGilt and M_Matter_WoodGilt. Every master applies the surface law (History.ush) to what it
 * shows, reading the museum Moment's weather from MPC_Moment.
 */
namespace Musee::Matter
{
	namespace Masters
	{
		inline constexpr const TCHAR* Stone = TEXT("stone");
		inline constexpr const TCHAR* Marble = TEXT("marble");
		inline constexpr const TCHAR* White = TEXT("white");
		inline constexpr const TCHAR* Metal = TEXT("metal");
		inline constexpr const TCHAR* Glass = TEXT("glass");
		inline constexpr const TCHAR* Water = TEXT("water");
		inline constexpr const TCHAR* Paint = TEXT("paint");
		inline constexpr const TCHAR* Wood = TEXT("wood");
		inline constexpr const TCHAR* Cloth = TEXT("cloth");
		inline constexpr const TCHAR* Sheet = TEXT("sheet");
		inline constexpr const TCHAR* Living = TEXT("living");

		/** The layer working the masters carry: gilt and its variants (gilt.gold-leaf.oil, gilt.gold-leaf.water), whose
		 *  key's first segment names the layer. */
		inline constexpr const TCHAR* Gilt = TEXT("gilt");
	}

	/** One master: its key and the layer working it carries (none for a plain master). */
	struct FMasterKind
	{
		FName Master;
		FName Layer;

		bool operator==(const FMasterKind& Other) const { return Master == Other.Master && Layer == Other.Layer; }
	};

	/** Every master the make step makes, in a fixed order. */
	TArray<FMasterKind> AllMasters();

	/** The recipe of a master, reading the Moment from Moment; an error for a master or a layer Matter does not make. */
	TValueOrError<FMasterRecipe, FString> MasterRecipe(const FMasterKind& Kind, UMaterialParameterCollection& Moment);

	/** The recipes of the stone, marble and white masters. */
	FMasterRecipe StoneRecipe(UMaterialParameterCollection& Moment);
	FMasterRecipe MarbleRecipe(UMaterialParameterCollection& Moment);
	FMasterRecipe WhiteRecipe(UMaterialParameterCollection& Moment, bool bGilt);

	/** The recipe of the metal master. */
	FMasterRecipe MetalRecipe(UMaterialParameterCollection& Moment, bool bGilt);

	/** The recipes of the glass, water and paint masters. */
	FMasterRecipe GlassRecipe(UMaterialParameterCollection& Moment);
	FMasterRecipe WaterRecipe(UMaterialParameterCollection& Moment);
	FMasterRecipe PaintRecipe(UMaterialParameterCollection& Moment);

	/** The recipes of the wood (with gilt's leaf laid over it, or not), cloth, sheet and living masters. */
	FMasterRecipe WoodRecipe(UMaterialParameterCollection& Moment, bool bGilt);
	FMasterRecipe ClothRecipe(UMaterialParameterCollection& Moment);
	FMasterRecipe SheetRecipe(UMaterialParameterCollection& Moment);
	FMasterRecipe LivingRecipe(UMaterialParameterCollection& Moment);

	/** Lays gilt's leaf over a recipe's Slab: the leaf's parameters, its call through the surface law, and the layered
	 *  surface. The recipe's own Slab becomes the base beneath the leaf. */
	void AddGiltLayer(FMasterRecipe& Recipe);
}
