#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"
#include "SiteIris.h"
#include "SitePort.h"
#include "Templates/ValueOrError.h"

#include "SitePortClass.generated.h"

/**
 * A notch in a well's section: a radial slot cut back from the well's circle, in which a part of the receiving stage runs
 * outside the well (the guide rails' blades in iris.4600's well). It spans a width across its bearing's ray, from an inner
 * radius out to the circle.
 */
USTRUCT()
struct MUSEESITE_API FSiteWellNotch
{
	GENERATED_BODY()

	/** The ray it lies on, degrees clockwise from north (0 north, 90 east), in the museum's true-north frame. */
	UPROPERTY(meta = (MuseeRequired, Units = "Degrees"))
	double BearingDegrees = 0.0;

	/** Half its width across the ray, metres. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double HalfWidthMeters = 0.0;

	/** Where it starts in from the well's circle, metres from the axis. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double InnerRadiusMeters = 0.0;
};

/**
 * A class of port: the standard opening two stages agree on, like a door schedule's door type. It sets the clear
 * opening (width, head, and whether the head is a semicircular arch), the rough opening each stage builds round it (the
 * clear opening grown by the allowance the joint's lining fills), the room each stage keeps clear round it on its face for
 * the joint's surround, the envelope within which the joint's masonry closes any gap between two faces, the bronze strip
 * on the mating plane, and what closes the port when it is joined. A vertical port's class names its well's section (a
 * clear radius less its notches), the parts a receiving stage may build in it, and the iris that closes it. Read from
 * Data/Site/PortClasses.json.
 */
USTRUCT()
struct MUSEESITE_API FSitePortClass
{
	GENERATED_BODY()

	/** doorway.3000x4800.arched: kind, clear width by crown in millimetres, head; iris.4600: the well's clear diameter. */
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

	/** What closes a joined port of this class: open-portal, pair-of-doors, gate, light-lock, landing-door, iris. */
	UPROPERTY(meta = (MuseeRequired))
	FName Closure;

	/**
	 * The rough opening's allowance, metres: each stage builds the clear opening grown by this at the jambs, the head and
	 * under the sill, and the joint's lining and threshold fill it. Zero where the stages finish their own passage.
	 */
	UPROPERTY(meta = (Units = "Meters"))
	double AllowanceMeters = 0.0;

	/** How far past the clear opening each stage keeps its face clear for the joint's surround, metres. */
	UPROPERTY(meta = (Units = "Meters"))
	double SurroundMeters = 0.0;

	/**
	 * The collar envelope, where the joint's masonry closes the gap between two faces that don't touch (the wedges between
	 * a flat port face and a drum, a passage between two stages): half its width across the port, and its bottom and top,
	 * metres from the threshold. A zero width: no masonry.
	 */
	UPROPERTY(meta = (Units = "Meters"))
	double CollarHalfWidthMeters = 0.0;

	UPROPERTY(meta = (Units = "Meters"))
	double CollarBottomMeters = 0.0;

	UPROPERTY(meta = (Units = "Meters"))
	double CollarTopMeters = 0.0;

	/** The bronze strip's width on the mating plane, across the threshold, metres; zero for none. */
	UPROPERTY(meta = (Units = "Meters"))
	double StripMeters = 0.0;

	/** A vertical port's well: its clear radius about the port's axis, metres. */
	UPROPERTY(meta = (Units = "Meters"))
	double WellRadiusMeters = 0.0;

	/** The well's section is its circle less these notches. */
	UPROPERTY()
	TArray<FSiteWellNotch> WellNotches;

	/**
	 * The parts a stage that receives the well may build inside its section (rail, iris, mast, dog, sill): everything else
	 * is kept out of the well, which the serving stage's car runs through.
	 */
	UPROPERTY()
	TArray<FName> WellAdmits;

	/** The iris that closes a port of the iris closure. */
	UPROPERTY()
	FSiteIris Iris;

	/** Where the opening comes from: the as-built door it keeps. */
	UPROPERTY()
	FString Source;

	/** The crown of the clear opening above the threshold, metres. */
	[[nodiscard]] double CrownMeters() const { return HeadMeters + (Arched ? 0.5 * ClearWidthMeters : 0.0); }

	/** Whether ports of this class are closed by an iris in their plane. */
	[[nodiscard]] bool IsIris() const { return Closure == FName(TEXT("iris")); }
};

namespace Musee::Site
{
	/** The port classes' file, under Data. */
	inline constexpr const TCHAR* PortClassesRecord = TEXT("Site/PortClasses.json");

	/** The port class of this key from a record file (the museum's by default), or why there is none. */
	[[nodiscard]] MUSEESITE_API TValueOrError<FSitePortClass, FString> FindPortClass(const FMuseeKey& Key, const FString& Path = PortClassesRecord);
}
