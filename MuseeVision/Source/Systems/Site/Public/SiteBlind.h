#pragma once

#include "CoreMinimal.h"
#include "FormDrawing.h"
#include "SitePort.h"
#include "SitePortClass.h"
#include "Templates/ValueOrError.h"

#include "SiteBlind.generated.h"

/**
 * What stops an unmated port, so a stage made alone is whole and a door waits where a room will join. A doorway's blind
 * is a closed pair of cast bronze leaves meeting on the port's centreline, set back in the rough opening. A vertical
 * port facing down, whose iris would be the absent stage's below, is closed by its class's iris shut on the mating plane.
 * Read from Data/Site/Blinds.json; the bronze and the iris's pearl steel are the palette's (Data/Matter/Palette.json).
 */
USTRUCT()
struct MUSEESITE_API FSiteBlindDoors
{
	GENERATED_BODY()

	/** The leaves' material: a palette key, its working and its history. */
	UPROPERTY(meta = (MuseeRequired))
	FName Material;

	UPROPERTY(meta = (MuseeRequired))
	FName Working;

	UPROPERTY(meta = (MuseeRequired))
	FName History;

	/** A leaf's thickness, metres. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double LeafMeters = 0.0;

	/** The gap round each leaf: at the jambs, the head, the floor and between the leaves, metres. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double ClearanceMeters = 0.0;

	/** How far the leaves' outer face stands back from the port's face, metres. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double SetBackMeters = 0.0;

	/** The shut iris's blades: a palette key, its working and its history. */
	UPROPERTY(meta = (MuseeRequired))
	FName IrisMaterial;

	UPROPERTY(meta = (MuseeRequired))
	FName IrisWorking;

	UPROPERTY(meta = (MuseeRequired))
	FName IrisHistory;

	UPROPERTY()
	FString Source;
};

namespace Musee::Site
{
	/** The blind doors' file, under Data. */
	inline constexpr const TCHAR* BlindsRecord = TEXT("Site/Blinds.json");

	/** The blind doors' record (the museum's by default). */
	[[nodiscard]] MUSEESITE_API TValueOrError<FSiteBlindDoors, FString> ReadBlindDoors(const FString& Path = BlindsRecord);

	/**
	 * The blind of an unmated port, drawn in Form's language in the stage's frame: a doorway's two leaves, named
	 * blind.<port>.left and blind.<port>.right; or, for a vertical port facing down, its class's iris shut on the mating
	 * plane, its blades named blind.<port>.iris.<k>. A vertical port facing up is shut by its own stage's iris and needs
	 * none. False, with the reason, when the class's opening can't hold the leaves or its iris isn't a stack.
	 */
	[[nodiscard]] MUSEESITE_API bool Blind(const FSitePort& Port, const FSitePortClass& Class, const FSiteBlindDoors& Doors,
		TArray<FFormDrawing>& OutDrawings, FString& OutWhy);
}
