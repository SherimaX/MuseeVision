#pragma once

#include "ArtHang.h"
#include "ArtWork.h"
#include "CoreMinimal.h"
#include "MuseeFinding.h"
#include "MuseeStageDresser.h"

namespace Musee::ArtMaking
{
	/** A finding of Art's about a stage's hanging plan. */
	[[nodiscard]] FMuseeFinding ArtFinding(const FMuseeStageContext& Stage, const FString& Summary);

	/**
	 * The stage's hanging plan, if it has one: false when the stage keeps no Hang.json, or when it doesn't read or
	 * names another stage (a finding).
	 */
	[[nodiscard]] bool ReadStageHang(const FMuseeStageContext& Stage, FArtHang& OutHang, TArray<FMuseeFinding>& OutFindings);

	/** One canvas of a work the stage makes: its entry in the plan, and where its left edge falls in the work's image. */
	struct FCanvasPlan
	{
		const FArtHangEntry* Entry = nullptr;
		double ImageOffsetMeters = 0.0;
	};

	/** A work the stage makes: the work (with its image), its canvases in order along the image, and the image's size. */
	struct FWorkPlan
	{
		FArtWork Work;
		TArray<FCanvasPlan> Canvases;
		FVector2D ImageMeters = FVector2D::ZeroVector;

		[[nodiscard]] bool IsSplit() const { return Canvases.Num() > 1; }
	};

	/** The working and history every work's paint is resolved with: its palette entry's one finish. */
	struct FPaintFinish
	{
		FName Working;
		FName History;
	};

	/**
	 * Every work of a hanging plan that Art can make, checked whole: each canvas's entry (its wall, size, support, frame
	 * surface in the stage's finish schedule, care class, id), the work's canvases (numbered 0 on, once each, of one
	 * height), its catalogue entry and its image; and the paint's finish. What can't be made is a finding and is left
	 * out; the works point into Hang, which must outlive them. False, with a finding, when the paint has no finish.
	 */
	[[nodiscard]] bool PlanWorks(const FMuseeStageContext& Stage, const FArtHang& Hang, TArray<FWorkPlan>& OutWorks, FPaintFinish& OutPaint,
		TArray<FMuseeFinding>& OutFindings);
}
