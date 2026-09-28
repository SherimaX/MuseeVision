#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"

#include "LightingRecords.generated.h"

// A stage's lighting schedule, Data/Stages/<stage>/Lighting.json, as the stage contract writes it (metres, in the
// stage's frame).

USTRUCT()
struct FLightScheduleDaylight
{
	GENERATED_BODY()

	/** The Glazed or Open opening that admits daylight. */
	UPROPERTY(meta = (MuseeRequired)) FString Opening;
	/** Its glazing, a palette key. */
	UPROPERTY() FString Glazing;
	/** Whose weather falls on it: `museum`, `giverny` or `london` (Decision D-005). */
	UPROPERTY(meta = (MuseeRequired)) FString Weather;
	/** Its daylight control from the catalogue, or `none`. */
	UPROPERTY() FString Control;
	UPROPERTY() FString Note;
};

/**
 * A zone of a luminous surface laid round the stage's vertical axis through its origin (the Atrium's canopy): the ring
 * between two radii, over the bearings from one to the other, clockwise from north (-y). A full ring or disc runs
 * [0, 360].
 */
USTRUCT()
struct FLightScheduleZone
{
	GENERATED_BODY()

	/** The inner and outer radii, in metres. */
	UPROPERTY() TArray<double> R;
	/** The bearings it runs from and to, in degrees clockwise from north; it may pass north ([345, 15]). */
	UPROPERTY() TArray<double> Bearing;
};

/**
 * The inside of a sphere about the luminaire's seat, facing its centre (the Sphere's image surface): its radius, and the
 * radius of the round opening at its foot, about the vertical through its centre (the Sphere's neck), none when zero.
 */
USTRUCT()
struct FLightScheduleSphere
{
	GENERATED_BODY()

	UPROPERTY(meta = (Units = "Meters")) double Radius = 0.0;
	UPROPERTY(meta = (Units = "Meters")) double Opening = 0.0;
	/**
	 * The stage's surface whose glass the panels shine through (the Sphere's `sphere.image`), when they have one: the
	 * sphere's radius is then the glass's inner face, and the panels' emitters stand behind it by the pane's thickness,
	 * which Matter's record of the surface's working gives. None: the picture is shown on the sphere itself.
	 */
	UPROPERTY() FString Behind;

	/** Whether the entry gives a sphere at all. */
	[[nodiscard]] bool IsGiven() const { return Radius != 0.0 || Opening != 0.0; }
};

USTRUCT()
struct FLightScheduleLuminaire
{
	GENERATED_BODY()

	/** The luminaire's name within the stage (`rim`). */
	UPROPERTY(meta = (MuseeRequired)) FString Id;
	/** Its catalogue key. */
	UPROPERTY(meta = (MuseeRequired)) FString Type;
	/** The key of the Mount anchor it sits on; a luminaire has either a mount or a point, never both. */
	UPROPERTY() FString Mount;
	/** Where it sits when it has no Mount anchor: a point [x, y, z] in the stage's frame, in metres. */
	UPROPERTY() TArray<double> Point;
	/**
	 * The point [x, y, z] its light is aimed at, in the stage's frame, in metres. Required at a point; on a mount, its
	 * light leaves along the mount's x axis when absent.
	 */
	UPROPERTY() TArray<double> Aim;
	/** `always`, `dusk` or `presence`. */
	UPROPERTY(meta = (MuseeRequired)) FString Operation;
	/** The channel (0 to 1) that dims it. */
	UPROPERTY() FString Follows;
	/** The moving body it rides on. */
	UPROPERTY() FString Body;
	/** The length laid, for a linear type (a strip, a cove line, a slot). */
	UPROPERTY(meta = (Units = "Meters")) double Length = 0.0;
	/**
	 * For a linear type, the direction [x, y, z] its run is laid in, in the stage's frame (a cove strip's tangent). Without
	 * it, a run at a point lies along the stage's x axis, and one on a mount along the mount's x axis.
	 */
	UPROPERTY() TArray<double> Along;
	/**
	 * For an areal type (a luminous ceiling's modules, edge-lit panes), the size [width, height] of the face it lights, in
	 * metres: its width along the stage's x axis and its height along its y when the face is level (in general, across
	 * and up the face). A luminaire has either an extent or a zone.
	 */
	UPROPERTY() TArray<double> Extent;
	/** For an areal type, the zone of a curved or round field it lights, in place of an extent. */
	UPROPERTY() FLightScheduleZone Zone;
	/**
	 * For an areal type, the sphere about its seat whose inside it lights, in place of an extent or a zone: a face that
	 * encloses the luminaire, its light converging on the seat, so its aim is its seat or none.
	 */
	UPROPERTY() FLightScheduleSphere Sphere;
	/** The piece it is set flush in (a step light's kerb or stone), by its surface key. */
	UPROPERTY() FMuseeKey Host;
	/** The surface its light is for (the walk a step light grazes), by its surface key. */
	UPROPERTY() FMuseeKey Surface;
	UPROPERTY() FString Note;
};

/** When a target holds: while a mechanism stands at one of its stops (the Cube's car on its floor). */
USTRUCT()
struct FLightScheduleWhen
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired)) FMuseeKey Mechanism;
	UPROPERTY(meta = (MuseeRequired)) FString Stop;
};

USTRUCT()
struct FLightScheduleTarget
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired)) FString Surface;
	UPROPERTY() FString Class;
	/** The target range [least, most], in lux; either end may be open (`[50, null]`, at least 50 lx). */
	UPROPERTY(meta = (MuseeRange)) TArray<double> Lux;
	UPROPERTY() FString Source;
	/** Checked only while its mechanism stands at its stop; always, without it. */
	UPROPERTY() FLightScheduleWhen When;
	UPROPERTY() FString Note;
};

USTRUCT()
struct FLightScheduleDesign
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired)) FString Brighter;
	UPROPERTY(meta = (MuseeRequired)) FString Darker;
	/** The range [least, most] of the brighter surface's luminance over the darker's; either end may be open. */
	UPROPERTY(meta = (MuseeRange)) TArray<double> Ratio;
	UPROPERTY() FString Note;
};

/** A stage's lighting schedule. */
USTRUCT()
struct FLightSchedule
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired)) FString Stage;
	UPROPERTY() FString Notes;
	UPROPERTY() TArray<FLightScheduleDaylight> Daylight;
	UPROPERTY() TArray<FLightScheduleLuminaire> Luminaires;
	UPROPERTY() TArray<FLightScheduleTarget> Targets;
	UPROPERTY() TArray<FLightScheduleDesign> Design;
};
