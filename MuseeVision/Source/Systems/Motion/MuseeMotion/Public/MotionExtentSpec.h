#pragma once

#include "CoreMinimal.h"
#include "MotionFollowing.h"
#include "MotionTelescopeStage.h"
#include "MotionTurns.h"
#include "MuseeKey.h"

#include "MotionExtentSpec.generated.h"

/**
 * A piece of a machine whose length changes as the machine runs, drawn by rule and never at one length
 * (Mechanisms.json `extents`): the Rotunda's telescopic rams, whose stages slide out of one another as the drum rises,
 * and the lift masts that grow turn by turn out of their housings as the Élan car climbs. Its end is anchored at a
 * point (At) on a body or on the stage; it runs from there along Axis for a length that follows a channel. Motion
 * draws its parts with Form's drawing, and moves them: a telescope's stages slide out largest first, each fully before
 * the next, as a single-acting telescopic cylinder's do (the largest piston area lifts first at one pressure), and
 * a column of turns shows as many whole turns as its length needs.
 */
USTRUCT()
struct MUSEEMOTION_API FMotionExtentSpec
{
	GENERATED_BODY()

	/** The piece's key (`rotunda.sun-clock.ram.1`); its parts are named under it. */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Key;

	/** The finish schedule's surface its parts are finished in. */
	UPROPERTY(meta = (MuseeRequired))
	FName Surface;

	/** The Body anchor its anchored end rides on, or the none key for the stage itself. */
	UPROPERTY()
	FMuseeKey Body;

	/** The anchored end, in metres in the body's frame (or the stage's). */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	FVector At = FVector::ZeroVector;

	/** The direction the length runs in from the anchored end, in the same frame; normalised when read. */
	UPROPERTY(meta = (MuseeRequired))
	FVector Axis = FVector::UpVector;

	/** The length, in metres, as it follows a channel. */
	UPROPERTY(meta = (MuseeRequired))
	FMotionFollowing Length;

	/** A telescope's moving stages, largest first; empty for a column of turns. */
	UPROPERTY()
	TArray<FMotionTelescopeStage> Telescope;

	/** A column of turns; unset for a telescope. */
	UPROPERTY()
	FMotionTurns Turns;

	/** The designer's note: the machine's sources. */
	UPROPERTY()
	FString Note;

	/** The telescope's length with every stage home: the shorter of the lengths it follows between. */
	[[nodiscard]] double ClosedLength() const;

	/** How far a telescope's stage stands out of the stage before it at a length: largest first, each in turn. */
	[[nodiscard]] double StageExtension(int32 Stage, double Length) const;

	/** How far a telescope's stage stands from where it is drawn (home) at a length: its own and those before it. */
	[[nodiscard]] double StageOffset(int32 Stage, double Length) const;

	/** How many turns a column shows at a length: every turn it needs, the last still forming in its housing. */
	[[nodiscard]] int32 TurnsAt(double Length) const;

	/** The piece a telescope's stage is drawn as. */
	[[nodiscard]] FName StagePiece(int32 Stage) const;

	/** The piece one turn of a column is drawn as. */
	[[nodiscard]] FName TurnPiece() const;
};
