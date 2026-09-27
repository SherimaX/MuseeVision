#pragma once

#include "CoreMinimal.h"
#include "MotionStep.h"
#include "MotionUseText.h"
#include "MuseeKey.h"

#include "MotionMechanismSpec.generated.h"

/**
 * One mechanism of a stage, as its Mechanisms.json specifies it (Docs/Stages contract): the body it moves, the joint
 * it moves along, how far and how fast, the control the visitor uses, the open and close sequences, and the guard
 * that keeps the body off the visitor.
 *
 * Joints: `prismatic` slides the body along Axis by Travel metres; `revolute` turns it about Axis (through the body's
 * origin) by Travel degrees. The axis is a direction in the stage's frame. Profiles: `linear`, or `ease-in-out`, the
 * minimum-jerk move (Flash and Hogan 1985), which starts and ends at rest with no step in acceleration. Guards: `none`;
 * `body-in-travel`, which holds the drive while the visitor's body is in its next travel slice and resumes when it is
 * clear (a lift); `body-in-travel-reverse`, which sends the body back to the state it left (a door).
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

	/** How long the full travel takes. */
	UPROPERTY(meta = (MuseeRequired, Units = "Seconds"))
	float Time = 0.0f;

	/** The move's profile: `linear` or `ease-in-out`. */
	UPROPERTY(meta = (MuseeRequired))
	FName Profile;

	/** The Use anchor of the control the visitor uses, or the none key for a mechanism nobody works by hand. */
	UPROPERTY()
	FMuseeKey Control;

	/** The steps that open the mechanism, in order. */
	UPROPERTY()
	TArray<FMotionStep> Open;

	/** The steps that close it, in order. */
	UPROPERTY()
	TArray<FMotionStep> Close;

	/** `none`, `body-in-travel` or `body-in-travel-reverse`. */
	UPROPERTY()
	FName Guard;

	/** The gap the guard keeps between the moving body and the visitor. */
	UPROPERTY(meta = (Units = "Meters"))
	float Clearance = 0.0f;

	/** What the control offers while the mechanism is closed; the joint's verb when absent (its verb's key is none). */
	UPROPERTY()
	FMotionUseText OpenUse;

	/** What the control offers while the mechanism is open; the joint's verb when absent (its verb's key is none). */
	UPROPERTY()
	FMotionUseText CloseUse;
};
