#pragma once

#include "CoreMinimal.h"
#include "Templates/ValueOrError.h"

#include "ArtSupport.generated.h"

/**
 * What a work is made on, as Art makes it (`Data/Art/Supports.json`): the palette finish its own image is laid in (the
 * oil paint of a canvas or a panel, the paper of a print, the silver of a daguerreotype, the lustre over a dish's glaze,
 * the ink on a scroll's silk or paper, a tapestry's weave), the finish of the faces that carry no image (a dish's glazed
 * underside), and the thickness of a sheet a work is made on. A support made as a kind of object of its own (paper
 * painted and mounted as a hanging scroll) has its own entry for that kind.
 */
USTRUCT()
struct FArtSupport
{
	GENERATED_BODY()

	/** The hanging plan's support (`canvas`, `paper`, `silvered copper plate`, `earthenware`). */
	UPROPERTY(meta = (MuseeRequired))
	FName Support;

	/** The kind of object (the plan's `kind`) this entry is for; none for the support made as any other. */
	UPROPERTY()
	FName Kind;

	/** The palette entry, working and history of the finish the work's own image is laid in. */
	UPROPERTY(meta = (MuseeRequired))
	FName Palette;

	UPROPERTY(meta = (MuseeRequired))
	FName Working;

	UPROPERTY(meta = (MuseeRequired))
	FName History;

	/** The same entry's finish of the faces that carry no image, where the support has such faces; none otherwise. */
	UPROPERTY()
	FName GroundWorking;

	UPROPERTY()
	FName GroundHistory;

	/** A sheet's thickness (a print, a plate); zero for a support that is not a sheet. */
	UPROPERTY(meta = (Units = "Meters"))
	double ThicknessMeters = 0.0;
};

/** Every support Art makes, as its record lists them. */
USTRUCT()
struct FArtSupports
{
	GENERATED_BODY()

	/** The record, under Data. */
	static constexpr const TCHAR* Record = TEXT("Art/Supports.json");

	UPROPERTY(meta = (MuseeRequired))
	TArray<FArtSupport> Supports;
};

namespace Musee::ArtMaking
{
	/** The supports Art makes, read once; an error when the record doesn't read. */
	[[nodiscard]] const TValueOrError<FArtSupports, FString>& ReadSupports();

	/** The support Art makes of this name, as made as no kind of its own, or null. */
	[[nodiscard]] const FArtSupport* FindSupport(FName Support);

	/** The support Art makes of this name made as this kind: its entry for the kind, else its entry for any; or null. */
	[[nodiscard]] const FArtSupport* FindSupport(FName Support, FName Kind);
}
