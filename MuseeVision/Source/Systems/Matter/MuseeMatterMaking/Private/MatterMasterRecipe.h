#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Hash/Blake3.h"
#include "Materials/MaterialExpressionSubstrate.h"
#include "Materials/MaterialExpressionTransform.h"

class UMaterialParameterCollection;

/**
 * A master recipe: the complete description of one of Matter's master materials, as data.
 *
 * A master is a thin graph. Its parameters are the inputs every instance sets (a substance's optics, a working's
 * texture, the state its history is in); it reads the museum Moment's weather from Time's parameter collection; its
 * calls are Custom expressions that call into Matter's shading library (/Project/Matter/*.ush), where every law and
 * every number lives; and it ends in a Substrate surface: one Slab, a Slab under a vertical layer (a gilt leaf), or a
 * Single Layer Water. The recipe holds no physics of its own: a call's code only calls the library, and Validate refuses
 * a number in it.
 *
 * The recipe is also the master's identity. Its canonical text, with the builder's version, is hashed (BLAKE3), and
 * every identifier the made material carries (its StateId, which keys its shaders in the DDC, and the guids of its
 * expressions and its package) derives from that hash. The same recipe therefore makes the same package, byte for
 * byte, and a changed recipe changes the identity and recompiles.
 */
namespace Musee::Matter
{
	/** The version of the graph builder's output. It is part of every master's identity: raise it whenever the builder
	 *  would make a different material from the same recipe. */
	inline constexpr int32 MasterBuilderVersion = 2;

	/** The engine's surface inputs a recipe may name as a source, by these node names. */
	namespace MasterInput
	{
		/** The position in the mesh's own axes, in engine units (a piece's mesh is cast in the piece's frame). */
		inline constexpr const TCHAR* LocalPosition = TEXT("LocalPosition");
		/** The vertex normal in world axes: the face's geometric orientation, for rain, snow and exposure. */
		inline constexpr const TCHAR* VertexNormalWorld = TEXT("VertexNormalWorld");
		/** The mesh's bounds in its own axes, in engine units; its outputs are the engine's (Min, Max, Extents). */
		inline constexpr const TCHAR* LocalBounds = TEXT("LocalBounds");
	}

	/** Where a pin takes its value: a node of the recipe (a parameter, a Moment read, a call, a transform, a utility or
	 *  an engine input) and, for a node with several outputs, one of them by name (none for the first). */
	struct FMasterSource
	{
		FName Node;
		FName Output;

		bool IsSet() const { return !Node.IsNone(); }
	};

	/** An input every instance of the master sets: a scalar, a three-component colour or vector, or a texture. */
	struct FMasterParameter
	{
		FName Name;
		FName Group;
		/** 1 for a scalar, 3 for a colour or vector; 0 for a texture. */
		int32 Components = 1;
		/** The value an instance starts from; a scalar's is in R. */
		FLinearColor Default = FLinearColor::Black;
		/** For a texture, the object path of the texture an instance starts from. */
		FString Texture;
		/** For a scalar each primitive supplies (its Custom Primitive Data), the slot; INDEX_NONE for an instance's. */
		int32 PrimitiveDataIndex = INDEX_NONE;
		/** The unit the name states ("mm", "1/yr", "yr", "1" for a fraction or a ratio). */
		FString Unit;
		/** What it is, physically, and who supplies it. */
		FString Meaning;

		bool IsTexture() const { return Components == 0; }
	};

	/** A scalar the master reads from Time's parameter collection (the museum Moment), by the collection's name. */
	struct FMasterMomentRead
	{
		FName Name;
	};

	/** One input or output of a call. An input takes its value from Source; an output is assigned by name in Code. A
	 *  texture input (Components 0) passes a texture object, which Code samples with <Name>Sampler. */
	struct FMasterPin
	{
		FName Name;
		int32 Components = 1;
		FMasterSource Source;
	};

	/** A call into Matter's shading library: one Custom expression that includes library files and calls their
	 *  functions. Code is the HLSL body; it declares no physics and holds no numbers. */
	struct FMasterCall
	{
		FName Name;
		TArray<FString> Includes;
		FString Code;
		TArray<FMasterPin> Inputs;
		int32 ReturnComponents = 1;
		TArray<FMasterPin> Outputs;
	};

	/** A change of axes of a vector, such as a relief gradient from the piece's axes to the face's tangent axes. */
	struct FMasterTransform
	{
		FName Name;
		FMasterSource Input;
		EMaterialVectorCoordTransformSource From = TRANSFORMSOURCE_Local;
		EMaterialVectorCoordTransform To = TRANSFORM_Tangent;
	};

	/** Substrate's Transmittance-To-MeanFreePath: a transmittance colour at normal incidence becomes the mean free path
	 *  a Slab of the default thickness takes (its outputs are the engine's: MFP, Thickness). */
	struct FMasterTransmittance
	{
		FName Name;
		FMasterSource Transmittance;
	};

	/** The pins of a Substrate Slab. Haziness is the weight of the Slab's second, wider specular lobe about Roughness;
	 *  the builder resolves it through Substrate's Haziness-To-Secondary-Roughness. */
	struct FMasterSlab
	{
		FMasterSource DiffuseAlbedo;
		FMasterSource F0;
		FMasterSource F90;
		FMasterSource Roughness;
		FMasterSource Anisotropy;
		FMasterSource Haziness;
		FMasterSource Normal;
		FMasterSource Tangent;
		FMasterSource SSSMFP;
		/** How light travels inside the slab: none for an opaque body, diffusion for marble, a simple volume for glass. */
		EMaterialSubSurfaceType SubSurface = MSS_None;
	};

	/** The pins of Substrate's Single Layer Water: a water surface over the volume of water beneath it. */
	struct FMasterWater
	{
		FMasterSource Specular;
		FMasterSource Roughness;
		FMasterSource Normal;
		FMasterSource WaterAlbedo;
		FMasterSource WaterExtinction;
	};

	/** What a master's front material is. */
	enum class EMasterSurface : uint8
	{
		/** One Slab. */
		Slab,
		/** A Slab (the layer: gilt's leaf) over the Slab beneath it, the layer weighted by its coverage. */
		LayeredSlab,
		/** Single Layer Water. */
		Water,
	};

	/** The material's own settings a master fixes. */
	struct FMasterSettings
	{
		EBlendMode BlendMode = BLEND_Opaque;
		/** Whether the master is used on Nanite meshes (every opaque piece is; glass and water are not). */
		bool bUsedWithNanite = false;
		/** Translucent glass is lit per pixel at its surface and does not bend the view: a pane's two faces are
		 *  parallel, so what is seen through it is displaced, not refracted. */
		bool bSurfaceTranslucency = false;
	};

	/** One master material, completely described. */
	struct FMasterRecipe
	{
		/** The long package path of the folder the master is made in, e.g. /Game/Generated/Matter. */
		FString PackagePath;
		/** The asset's name, M_Matter_<Master>. */
		FName AssetName;
		FMasterSettings Settings;
		/** Time's parameter collection the Moment reads come from; required when the recipe reads any. */
		UMaterialParameterCollection* Moment = nullptr;
		TArray<FMasterParameter> Parameters;
		TArray<FMasterMomentRead> MomentReads;
		TArray<FMasterCall> Calls;
		TArray<FMasterTransform> Transforms;
		TArray<FMasterTransmittance> Transmittances;
		EMasterSurface Surface = EMasterSurface::Slab;
		/** The Slab: the whole surface, or the base beneath a layer. */
		FMasterSlab Slab;
		/** The layer's Slab and its coverage, for a layered surface. */
		FMasterSlab Layer;
		FMasterSource LayerCoverage;
		FMasterWater Water;
	};

	/** A master's identity: the hash of its recipe and the builder's version, and the identifiers derived from it. */
	struct FMasterIdentity
	{
		FBlake3Hash InputHash;

		/** A guid for one role in the made package ("Material.StateId", "Expression.Call_History"...), derived from the
		 *  input hash, so each role has its own identifier and all of them change together with the recipe. */
		FGuid Derive(FStringView Role) const;

		/** The material's StateId: it keys the material's shaders in the derived-data cache. */
		FGuid StateId() const;
	};

	/** The recipe's canonical text: every field, in a fixed order and format. */
	FString CanonicalText(const FMasterRecipe& Recipe);

	/** The recipe's identity: BLAKE3 of the builder's version and the canonical text. */
	FMasterIdentity IdentityOf(const FMasterRecipe& Recipe);

	/** The recipe's problems, each a sentence naming the node at fault; empty when the recipe can be built. It checks
	 *  that every name is unique, every source resolves, every include lies in Matter's library, every pin has a
	 *  valid width, that the Moment is given when it is read, that the surface's pins are set, and that no call holds
	 *  a number. */
	TArray<FString> Validate(const FMasterRecipe& Recipe);

	/** The long package name of the recipe's master: PackagePath/AssetName. */
	FString PackageNameOf(const FMasterRecipe& Recipe);

	/** The Slab's pins, each with its name, in a fixed order. */
	TArray<TPair<const TCHAR*, const FMasterSource*>> SlabPins(const FMasterSlab& Slab);

	/** Single Layer Water's pins, each with its name, in a fixed order. */
	TArray<TPair<const TCHAR*, const FMasterSource*>> WaterPins(const FMasterWater& Water);
}
