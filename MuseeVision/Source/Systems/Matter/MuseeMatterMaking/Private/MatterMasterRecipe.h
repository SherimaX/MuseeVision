#pragma once

#include "CoreMinimal.h"
#include "Hash/Blake3.h"
#include "Materials/MaterialExpressionTransform.h"

/**
 * A master recipe: the complete description of one of Matter's master materials, as data.
 *
 * A master is a thin graph. Its parameters are the inputs every instance sets (a substance's optics, a working's
 * texture, the state its history is in); its calls are Custom expressions that call into Matter's shading library
 * (/Project/Matter/*.ush), where every law and every number lives; and it ends in one Substrate Slab. The recipe holds
 * no physics of its own: a call's code only calls the library, and Validate refuses a number in it.
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
	inline constexpr int32 MasterBuilderVersion = 1;

	/** The engine's surface inputs a recipe may name as a source, by these node names. */
	namespace MasterInput
	{
		/** The position in the mesh's own axes, in engine units (a piece's mesh is cast in the piece's frame). */
		inline constexpr const TCHAR* LocalPosition = TEXT("LocalPosition");
		/** The vertex normal in world axes: the face's geometric orientation, for rain, snow and exposure. */
		inline constexpr const TCHAR* VertexNormalWorld = TEXT("VertexNormalWorld");
	}

	/** Where a pin takes its value: a node of the recipe (a parameter, a call, a transform or an engine input) and,
	 *  for a call, one of its named outputs (none for the call's return value). */
	struct FMasterSource
	{
		FName Node;
		FName Output;

		bool IsSet() const { return !Node.IsNone(); }
	};

	/** An input every instance of the master sets: a scalar or a three-component colour or vector. */
	struct FMasterParameter
	{
		FName Name;
		FName Group;
		int32 Components = 1;
		/** The value an instance starts from; a scalar's is in R. */
		FLinearColor Default = FLinearColor::Black;
		/** The unit the name states ("mm", "m/s", "1/yr", "yr", "1" for a fraction or a ratio). */
		FString Unit;
		/** What it is, physically, and who supplies it. */
		FString Meaning;
	};

	/** One input or output of a call. An input takes its value from Source; an output is assigned by name in Code. */
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

	/** The pins of the Substrate Slab a master ends in. Haziness is the weight of the Slab's second, wider specular
	 *  lobe about Roughness; the builder resolves it through Substrate's Haziness-To-Secondary-Roughness. */
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
	};

	/** One master material, completely described. */
	struct FMasterRecipe
	{
		/** The long package path of the folder the master is made in, e.g. /Game/Generated/Matter. */
		FString PackagePath;
		/** The asset's name, M_Matter_<Family>. */
		FName AssetName;
		/** Whether the master is used on Nanite meshes (every opaque piece is). */
		bool bUsedWithNanite = false;
		TArray<FMasterParameter> Parameters;
		TArray<FMasterCall> Calls;
		TArray<FMasterTransform> Transforms;
		FMasterSlab Slab;
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
	 *  valid width, and that no call holds a number. */
	TArray<FString> Validate(const FMasterRecipe& Recipe);

	/** The long package name of the recipe's master: PackagePath/AssetName. */
	FString PackageNameOf(const FMasterRecipe& Recipe);
}
