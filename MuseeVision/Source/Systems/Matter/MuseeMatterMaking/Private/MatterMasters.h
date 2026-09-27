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
 * - paint: a painted canvas carrying the work's own image under its varnish (Paint.ush).
 *
 * A layer working (gilt) is carried as a Substrate vertical layer over the masters that host it (white and metal): its
 * masters are M_Matter_WhiteGilt and M_Matter_MetalGilt. Every master applies the surface law (History.ush) to what it
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

		/** The layer working the masters carry. */
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

	/** Lays gilt's leaf over a recipe's Slab: the leaf's parameters, its call through the surface law, and the layered
	 *  surface. The recipe's own Slab becomes the base beneath the leaf. */
	void AddGiltLayer(FMasterRecipe& Recipe);
}
