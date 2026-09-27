#pragma once

#include "CoreMinimal.h"
#include "MuseeFrame.h"
#include "MuseeKey.h"

#include "SitePort.generated.h"

/** What a port joins: the ways a visitor passes from one stage to the next. */
UENUM()
enum class ESitePortKind : uint8
{
	/** A door in a wall, on one level. */
	Doorway,
	/** A stair between levels, running through a well. */
	Stairwell,
	/** A lift between levels, running through a well. */
	Shaft,
	/** A gate to the grounds. */
	Gate,
};

/**
 * The prism a vertical connection runs through, in its stage's frame. The serving side owns what is in it (the stair,
 * the lift car); the receiving side reserves it as a void and builds nothing inside it.
 */
USTRUCT()
struct MUSEESITE_API FSiteWell
{
	GENERATED_BODY()

	/** True on the side that owns the well's contents. */
	UPROPERTY()
	bool bServing = false;

	/** The prism's lower and upper corners, metres. */
	UPROPERTY(meta = (Units = "Meters"))
	FVector MinMeters = FVector::ZeroVector;

	UPROPERTY(meta = (Units = "Meters"))
	FVector MaxMeters = FVector::ZeroVector;
};

/**
 * A port: the one place a stage meets another. Its frame stands on the stage's outer face at the opening's centreline
 * and threshold level, with x facing out of the stage and z up. Its class (Data/Site/PortClasses.json) sets the clear
 * opening and what closes it, so two ports that mate agree by construction. The stage builds only the rough opening;
 * the joint between two mated ports, or the blind on an unmated one, is Site's.
 */
USTRUCT()
struct MUSEESITE_API FSitePort
{
	GENERATED_BODY()

	/** The port's key, under its stage: nympheas.east. */
	UPROPERTY()
	FMuseeKey Key;

	UPROPERTY()
	ESitePortKind Kind = ESitePortKind::Doorway;

	/** Its class in Data/Site/PortClasses.json: doorway.3000x4800.arched. */
	UPROPERTY()
	FMuseeKey Class;

	/** On the outer face, at the threshold, x out of the stage and z up, in the stage's frame. */
	UPROPERTY()
	FMuseeFrame Frame;

	/** The depth of the wall the opening passes through, metres. */
	UPROPERTY(meta = (Units = "Meters"))
	double WallDepthMeters = 0.0;

	/** The radius of a cylindrical outer face (a drum), metres; zero for a flat face. */
	UPROPERTY(meta = (Units = "Meters"))
	double FaceRadiusMeters = 0.0;

	/** The port it is designed to meet (salon.west), as the stage's own design says; the site plan decides. */
	UPROPERTY()
	FMuseeKey Mates;

	/** Whether the port has a well (a stairwell or a shaft), and the well. */
	UPROPERTY()
	bool bHasWell = false;

	UPROPERTY()
	FSiteWell Well;

	/** The direction the port faces, out of its stage, in the stage's frame. */
	[[nodiscard]] FVector Facing() const { return Frame.DirectionToParent(FVector::ForwardVector); }
};
