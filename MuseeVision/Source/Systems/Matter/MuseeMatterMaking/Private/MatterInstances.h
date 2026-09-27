#pragma once

#include "CoreMinimal.h"
#include "MatterHistory.h"
#include "MatterMasters.h"
#include "MatterPaletteEntry.h"
#include "MatterSubstance.h"
#include "MatterWorking.h"
#include "MuseeDependencySet.h"
#include "MuseeKey.h"
#include "Templates/ValueOrError.h"

class UMaterialInstanceConstant;
class UMaterialInterface;

/**
 * Composing a material instance from a material spec: a palette key, a working and a history (D-015, "Matter defines").
 *
 * The spec's records are read (the palette entry, its substance's typed record, the working's, and the history regime
 * of the master it is shaded by), the instance is parented to that master, and every parameter the master declares is
 * written from one record field, in the unit the master states. A record that lacks a number the master needs, gives it
 * in another unit, or a master parameter no record writes, is a problem: there is no default and no fallback. The
 * instance's identifiers are derived from what was written, so the same records make the same instance.
 */
namespace Musee::Matter
{
	/** The version of the instance composer's output: raise it whenever it would compose an instance differently. */
	inline constexpr int32 InstanceComposerVersion = 2;

	/** What a material is: a palette key, a working and a history regime. */
	struct FMatterSpec
	{
		FMuseeKey Palette;
		FMuseeKey Working;
		FMuseeKey History;

		/** palette/working/history, for messages and tags. */
		FString ToString() const;
	};

	/** Everything a spec's records say, and the master that shades it. */
	struct FSpecRecords
	{
		FMatterPaletteEntry Entry;
		FMatterSubstance Substance;
		FMatterWorking Working;
		/** For a layer working (gilt and its variants), the working of the host face beneath it. */
		TOptional<FMatterWorking> Host;
		FMatterHistory History;
		FMasterKind Master;
	};

	/** Reads a spec's records; an error names the missing record or key. */
	TValueOrError<FSpecRecords, FString> ReadSpec(const FMatterSpec& Spec);

	/** What composing an instance did. */
	struct FInstanceComposed
	{
		FMasterKind Master;
		/** The records read, each with the hash of its content. */
		FMuseeDependencySet Inputs;
		/** Every parameter written, as name=value text, in a fixed order: the instance's identity. */
		FString Written;
		TArray<FString> Problems;

		bool Succeeded() const { return Problems.IsEmpty(); }
	};

	/** Composes Instance from Spec's records, parented to the master FindMaster gives for the spec's master kind. */
	FInstanceComposed ComposeInstance(const FMatterSpec& Spec, UMaterialInstanceConstant& Instance,
		TFunctionRef<UMaterialInterface*(const FMasterKind&)> FindMaster);

	/** Finishes a composed instance: applies its parameters and gives it the identifiers derived from what it was
	 *  composed with (Composed.Written). Call again after writing more parameters into it. */
	void StampInstance(const FInstanceComposed& Composed, const FMatterSpec& Spec, UMaterialInstanceConstant& Instance);

	/** An identifier for one role of a composed instance ("Instance.ParameterStateId"...), derived from what it was
	 *  composed with. */
	FGuid InstanceGuid(const FInstanceComposed& Composed, const FMatterSpec& Spec, FStringView Role);

	/** The layer a working lays, from its key's first segment (gilt.gold-leaf.oil lays gilt), or none for a face
	 *  working. */
	FName LayerOf(const FMatterWorking& Working);
}
