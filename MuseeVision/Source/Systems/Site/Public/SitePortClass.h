#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"
#include "SitePort.h"
#include "Templates/ValueOrError.h"

#include "SitePortClass.generated.h"

/**
 * A class of port: the standard opening two stages agree on, like a door schedule's door type. It sets the clear
 * opening (width, head, and whether the head is a semicircular arch) and what closes it when the port is joined. Read
 * from Data/Site/PortClasses.json.
 */
USTRUCT()
struct MUSEESITE_API FSitePortClass
{
	GENERATED_BODY()

	/** doorway.3000x4800.arched: kind, clear width by crown in millimetres, head. */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Key;

	UPROPERTY(meta = (MuseeRequired))
	ESitePortKind Kind = ESitePortKind::Doorway;

	/** Between the jambs, metres. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double ClearWidthMeters = 0.0;

	/** The head above the threshold: the lintel's soffit, or the springing of an arched head, metres. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double HeadMeters = 0.0;

	/** A semicircular head of half the clear width's radius, springing at HeadMeters. */
	UPROPERTY()
	bool Arched = false;

	/** What closes a joined port of this class: open-portal, pair-of-doors, gate, light-lock, landing-door. */
	UPROPERTY(meta = (MuseeRequired))
	FName Closure;

	/** Where the opening comes from: the as-built door it keeps. */
	UPROPERTY()
	FString Source;

	/** The crown of the clear opening above the threshold, metres. */
	[[nodiscard]] double CrownMeters() const { return HeadMeters + (Arched ? 0.5 * ClearWidthMeters : 0.0); }
};

namespace Musee::Site
{
	/** The port classes' file, under Data. */
	inline constexpr const TCHAR* PortClassesRecord = TEXT("Site/PortClasses.json");

	/** The port class of this key from a record file (the museum's by default), or why there is none. */
	[[nodiscard]] MUSEESITE_API TValueOrError<FSitePortClass, FString> FindPortClass(const FMuseeKey& Key, const FString& Path = PortClassesRecord);
}
