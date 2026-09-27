#pragma once

#include "CoreMinimal.h"
#include "MotionDrive.h"
#include "MotionFollowing.h"
#include "MotionRide.h"
#include "MotionStep.h"
#include "MotionStop.h"
#include "MotionReturn.h"
#include "MotionUseText.h"
#include "MuseeKey.h"

#include "MotionMechanismSpec.generated.h"

/**
 * One mechanism of a stage, as its Mechanisms.json specifies it (Docs/Stages contract): the body it moves, the joint
 * it moves along, how far and how fast, the control the visitor uses, the open and close sequences, the rides a lift
 * offers at its stops, and the guard that keeps the body off the visitor.
 *
 * Joints: `prismatic` slides the body along Axis by Travel metres; `revolute` turns it about Axis (through the body's
 * origin, its pivot) by Travel degrees. The axis is a direction in the stage's frame. The travel runs from `closed` (0)
 * to `open` (1); a lift names its landings between as stops. The body is drawn at its `drawn` state (closed when it
 * names none), starts at its `home` (where it is drawn when it names none), and its close sequence leaves it there.
 *
 * Timing: a short move gives `time` (for the full travel) and `profile`: `linear`, or `ease-in-out`, the minimum-jerk
 * move (Flash and Hogan 1985). A long drive gives `drive` in their place: speed, acceleration and jerk limits, with
 * zones. Guards: `none`; `body-in-travel`, which holds the drive while the visitor's body is in its next travel slice
 * and resumes when it is clear (a lift); `body-in-travel-reverse`, which sends the body back to the state it left (a
 * door).
 *
 * A mechanism may publish its position on a channel, and may instead follow another's channel (guard rails that rise
 * with the pond they guard, a lift's door leaves), in which case it has no control, no sequences and no rides.
 *
 * Mechanisms of one `group` share a space, as the Reserve's sliding screens share the aisle: at most one stands away
 * from closed at a time, and opening one first runs the others' close. A mechanism `start`s where the museum opens it
 * (its home by default). One that `returns` goes home by itself when it is left open with nobody near.
 */
USTRUCT()
struct MUSEEMOTION_API FMotionMechanismSpec
{
	GENERATED_BODY()

	/** The mechanism's key (`nympheas.pond-lift`); its actuations and captions are published under it. */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Key;

	/** `prismatic` or `revolute`. */
	UPROPERTY(meta = (MuseeRequired))
	FName Joint;

	/** The Body anchor the mechanism moves; the stage's level carries a scene component tagged with it. */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Body;

	/** The joint's axis in the stage's frame, as three numbers; normalised when read. */
	UPROPERTY(meta = (MuseeRequired))
	TArray<double> Axis;

	/** The joint's travel from closed to open: metres for a prismatic joint, degrees for a revolute one. */
	UPROPERTY(meta = (MuseeRequired))
	float Travel = 0.0f;

	/** How long the full travel takes, for a mechanism timed by its profile (none with a drive). */
	UPROPERTY(meta = (Units = "Seconds"))
	float Time = 0.0f;

	/** The move's profile, `linear` or `ease-in-out`, for a mechanism timed by it (none with a drive). */
	UPROPERTY()
	FName Profile;

	/** A long drive's limits, in place of Time and Profile. */
	UPROPERTY()
	FMotionDrive Drive;

	/** The named places the body rests at on its travel. */
	UPROPERTY()
	TArray<FMotionStop> Stops;

	/** The state the stage draws the body in: a stop, `open`, or `closed` (when none is named). */
	UPROPERTY()
	FName Drawn;

	/** Where its close sequence leaves it, and where it starts unless it names a start; where it is drawn when none is named. */
	UPROPERTY()
	FName Home;

	/** The Use anchor of the control the visitor uses, or the none key for a mechanism nobody works by hand. */
	UPROPERTY()
	FMuseeKey Control;

	/** The steps that open the mechanism, in order. */
	UPROPERTY()
	TArray<FMotionStep> Open;

	/** The steps that close it, in order. */
	UPROPERTY()
	TArray<FMotionStep> Close;

	/** The further uses of the body, each offered at one stop. */
	UPROPERTY()
	TArray<FMotionRide> Rides;

	/** `none`, `body-in-travel` or `body-in-travel-reverse`. */
	UPROPERTY()
	FName Guard;

	/** The gap the guard keeps between the moving body and the visitor. */
	UPROPERTY(meta = (Units = "Meters"))
	float Clearance = 0.0f;

	/** The channel the mechanism publishes its position on, as a share of its travel from 0 to 1 (none: it publishes none). */
	UPROPERTY()
	FMuseeKey Publishes;

	/** The channel the mechanism follows instead of a control; a following mechanism has no control, sequences or rides. */
	UPROPERTY()
	FMotionFollowing Follows;

	/** What the control offers away from where the open sequence ends; the joint's verb when absent (its verb's key is none). */
	UPROPERTY()
	FMotionUseText OpenUse;

	/** What the control offers where the open sequence ends; the joint's verb when absent (its verb's key is none). */
	UPROPERTY()
	FMotionUseText CloseUse;

	/** The group whose mechanisms share one space, at most one of them away from closed; none for a mechanism alone. */
	UPROPERTY()
	FMuseeKey Group;

	/** The state it stands in when the museum opens: `closed`, `open` or a stop; its home when none is named. */
	UPROPERTY()
	FName Start;

	/** How it goes home by itself when it is left; unset for a mechanism that waits to be used. */
	UPROPERTY()
	FMotionReturn Returns;

	/** The designer's note: the machine's sources and intent. */
	UPROPERTY()
	FString Note;

	/** Where a named state lies on the travel, from 0 to 1: `closed`, `open` or a stop; nothing for any other name. */
	[[nodiscard]] TOptional<double> PositionOf(FName State) const;

	/** Where the stage draws the body, as a share of the travel. */
	[[nodiscard]] double DrawnPosition() const;

	/** Where its close sequence leaves it, as a share of the travel: its home, or where it is drawn. */
	[[nodiscard]] double HomePosition() const;

	/** Where it stands when the museum opens, as a share of the travel: its start, or its home. */
	[[nodiscard]] double StartPosition() const;

	/** Gives every text that has a source but no key the key its place implies: `<mechanism>.open-use.verb`,
	 *  `<mechanism>.ride-2.caption` (the contract: a plain string is the source, keyed by the mechanism and the field). */
	void KeyTexts();
};
