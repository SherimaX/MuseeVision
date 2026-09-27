#pragma once

#include "ArtHang.h"
#include "ArtWork.h"
#include "CoreMinimal.h"
#include "MuseeFinding.h"
#include "MuseeStageDresser.h"

namespace Musee::ArtMaking
{
	/** The kind of stage anchor a canvas hangs on (the stage contract): a hanging wall. */
	inline const FName WallAnchorKind(TEXT("Wall"));

	/**
	 * The kind of stage anchor a stained-glass panel is set in (the stage contract, D-014): a glazed light, its origin at
	 * the panel's lower-left seen from inside, x along the light, y out of the glass towards the viewer, z up.
	 */
	inline const FName GlazingAnchorKind(TEXT("Glazing"));

	/** A finding of Art's about a stage's hanging plan. */
	[[nodiscard]] FMuseeFinding ArtFinding(const FMuseeStageContext& Stage, const FString& Summary);

	/**
	 * The stage's hanging plan, if it has one: false when the stage keeps no Hang.json, or when it doesn't read or
	 * names another stage (a finding).
	 */
	[[nodiscard]] bool ReadStageHang(const FMuseeStageContext& Stage, FArtHang& OutHang, TArray<FMuseeFinding>& OutFindings);

	/** What the plan's entries are checked against, read once: Care.json's classes and reproductions. */
	struct FCareKeys
	{
		TArray<FName> Classes;
		TArray<FName> Reproductions;
	};

	/** Care.json's classes and reproductions, or why it can't be read. */
	[[nodiscard]] TValueOrError<FCareKeys, FString> ReadCareKeys();

	/**
	 * Whether an entry resolves: to a work its catalogue holds (read with its reproduction's credit), whether Art makes
	 * it or the stage builds it (`"built": "form"`, the one value `built` takes); with a care class and a reproduction
	 * Care.json knows, and a well-formed original's size where it is made after one. The work, or why not.
	 */
	[[nodiscard]] TValueOrError<FArtWork, FString> ResolveEntry(const FArtHangEntry& Entry, const FCareKeys& Care);

	/** How a layer is finished: the palette entry it derives from, and the working and history it is named with. */
	struct FLayerFinish
	{
		FName Palette;
		FName Working;
		FName History;
	};

	/**
	 * One object of a work the stage makes: its entry in the plan; where its left edge falls in the work's one image, or
	 * its own image; the palette key of the layer it shows, and how that layer is finished.
	 */
	struct FCanvasPlan
	{
		const FArtHangEntry* Entry = nullptr;
		double ImageOffsetMeters = 0.0;
		FString OwnImagePath;
		FName Layer;
		FLayerFinish Finish;
	};

	/**
	 * A work the stage makes: the work (with its image), its objects in order, and the one image's size. A work whose
	 * objects each show their own image (a series of canvases, a band of panels) has no one image.
	 */
	struct FWorkPlan
	{
		FArtWork Work;
		TArray<FCanvasPlan> Canvases;
		FVector2D ImageMeters = FVector2D::ZeroVector;
		bool bOwnImages = false;

		[[nodiscard]] bool IsSplit() const { return Canvases.Num() > 1; }
	};

	/**
	 * Every work of a hanging plan that Art can make, checked whole: each object's entry (it resolves; its anchor, size,
	 * support, frame and glass surfaces in the stage's finish schedule), the work's objects (each index once; sharing
	 * one image at one height, or each showing its own) and its images; and the oil paint's finish. Works the stage
	 * builds are left to it. What can't be made is a finding and is left out; the works point into Hang, which must
	 * outlive them. False, with a finding, when the paint has no finish.
	 */
	[[nodiscard]] bool PlanWorks(const FMuseeStageContext& Stage, const FArtHang& Hang, TArray<FWorkPlan>& OutWorks, TArray<FMuseeFinding>& OutFindings);
}
