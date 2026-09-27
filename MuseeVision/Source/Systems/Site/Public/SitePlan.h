#pragma once

#include "CoreMinimal.h"
#include "MuseeFinding.h"
#include "MuseeKey.h"
#include "SitePort.h"
#include "Templates/ValueOrError.h"

#include "SitePlan.generated.h"

/** Two ports that meet, and the passage between their faces (zero where the walls touch). */
USTRUCT()
struct MUSEESITE_API FSiteMating
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey A;

	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey B;

	/** From A's face to B's, along A's facing, metres. */
	UPROPERTY(meta = (Units = "Meters"))
	double PassageMeters = 0.0;

	UPROPERTY()
	FString Note;
};

/** Where the visitor arrives: a point on a stage's floor and the way they face, in that stage's frame. */
USTRUCT()
struct MUSEESITE_API FSiteArrival
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Stage;

	/** (x east, y south, z up) in the stage's frame, metres: the floor under the visitor's feet. */
	UPROPERTY(meta = (MuseeRequired))
	TArray<double> PointMeters;

	/** The way the visitor faces, degrees from east towards south (180 faces west). */
	UPROPERTY(meta = (Units = "Degrees"))
	double FacingDegrees = 0.0;
};

/**
 * The site plan (Data/Site/SitePlan.json): the one record of how the museum's stages join. The anchor stage stands at
 * the museum's origin; every other stage is placed by translation alone, through the matings, so each keeps its true
 * north and the sun comes from the real direction.
 */
USTRUCT()
struct MUSEESITE_API FSitePlan
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Anchor;

	UPROPERTY(meta = (MuseeRequired))
	FSiteArrival Arrival;

	UPROPERTY()
	TArray<FSiteMating> Matings;
};

/** Where the plan puts the stages it can place: each stage's translation, the ports mated, and the arrival. */
struct FSiteLayout
{
	/** Where each placed stage's origin stands in the museum's frame, metres. */
	TMap<FMuseeKey, FVector> TranslationsMeters;

	/** The ports joined to a present neighbour; every other port is blind. */
	TSet<FMuseeKey> MatedPorts;

	/** The arrival in the museum's frame: the point (metres) and the facing; set when its stage is placed. */
	bool bHasArrival = false;
	FVector ArrivalMeters = FVector::ZeroVector;
	double ArrivalFacingDegrees = 0.0;
};

namespace Musee::Site
{
	/** The site plan's file, under Data. */
	inline constexpr const TCHAR* PlanRecord = TEXT("Site/SitePlan.json");

	/** Two facings are opposite when they turn from each other by less than this: a hundredth of a degree. */
	inline constexpr double FacingToleranceDegrees = 0.01;

	/** A loop of matings closes when it returns within one millimetre (Docs/Architecture.md, Site.LoopsClose). */
	inline constexpr double LoopToleranceMeters = 0.001;

	/** The site plan from a record file (the museum's by default). */
	[[nodiscard]] MUSEESITE_API TValueOrError<FSitePlan, FString> ReadPlan(const FString& Path = PlanRecord);

	/** The stage a port belongs to: its key one scope out. */
	[[nodiscard]] MUSEESITE_API FMuseeKey StageOfPort(const FMuseeKey& Port);

	/**
	 * Places the stages by translation alone, walking the matings out from the anchor. A mating whose stages are both
	 * present joins them; one naming a stage that isn't built yet is left for later, and its present port stays blind.
	 * Ports that disagree in kind or class, or don't face each other, are findings (Site.PortsMate); a loop that returns
	 * more than a millimetre off is a finding (Site.LoopsClose).
	 */
	[[nodiscard]] MUSEESITE_API FSiteLayout Solve(const FSitePlan& Plan, const TMap<FMuseeKey, TArray<FSitePort>>& PortsByStage,
		TArray<FMuseeFinding>& OutFindings);
}
