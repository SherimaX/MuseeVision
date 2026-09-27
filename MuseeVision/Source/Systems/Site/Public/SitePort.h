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

/** Pieces a stage that receives a well builds inside it, as the part of the port's class they are: neck.iris. as iris. */
USTRUCT()
struct MUSEESITE_API FSiteWellPart
{
	GENERATED_BODY()

	/** The start of the pieces' names: every piece whose name begins with it is this part. */
	UPROPERTY()
	FString PiecePrefix;

	/** The part of the class's well it is (FSitePortClass::WellAdmits): rail, iris, mast, dog, sill. */
	UPROPERTY()
	FName Part;
};

/**
 * The prism a vertical connection runs through, in its stage's frame. The serving side owns what is in it (the stair,
 * the lift car); the receiving side reserves it as a void, shaped as its port class's well section, and builds inside it
 * only the parts the class admits.
 */
USTRUCT()
struct MUSEESITE_API FSiteWell
{
	GENERATED_BODY()

	/** True on the side that owns the well's contents. */
	UPROPERTY()
	bool bServing = false;

	/** The prism's lower and upper corners, metres: a round well's bounds. */
	UPROPERTY(meta = (Units = "Meters"))
	FVector MinMeters = FVector::ZeroVector;

	UPROPERTY(meta = (Units = "Meters"))
	FVector MaxMeters = FVector::ZeroVector;

	/** A round well's radius about the vertical line through the prism's centre, metres; zero for a square prism. */
	UPROPERTY(meta = (Units = "Meters"))
	double RadiusMeters = 0.0;

	/**
	 * On the receiving side, the pieces the stage builds inside the well's section, and the part of the class each is; the
	 * first whose prefix a piece's name begins with names it. A piece in the well that no part names is kept out.
	 */
	UPROPERTY()
	TArray<FSiteWellPart> Parts;
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

	/**
	 * The radius of a cylindrical outer face, metres, whose axis is vertical on the port's centreline: positive when the
	 * axis stands behind the face, inside the stage (a drum's outer face, bulging out), negative when it stands ahead of
	 * it (a face wrapped round a neighbour's drum); zero for a flat face. The wall's inner face is the concentric cylinder
	 * WallDepthMeters behind it, unless it is flat.
	 */
	UPROPERTY(meta = (Units = "Meters"))
	double FaceRadiusMeters = 0.0;

	/** Whether the wall's inner face is the plane WallDepthMeters behind the port's frame (a flat room face behind a curved port face). */
	UPROPERTY()
	bool bFlatInnerFace = false;

	/**
	 * Whether the stage draws its own surround round this port on its inner face (an archivolt, an architrave), so the
	 * joint draws none there.
	 */
	UPROPERTY()
	bool bOwnSurround = false;

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
