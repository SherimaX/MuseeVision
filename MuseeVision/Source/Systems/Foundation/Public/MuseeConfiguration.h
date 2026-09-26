#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"
#include "MuseePose.h"

#include "MuseeConfiguration.generated.h"

/**
 * One state of every mechanism in scope: a named set of poses. It is the one currency for anything checked "in every
 * mechanism state". Motion enumerates the configurations of a stage, Site composes them across seams, and Studio
 * hands each one to the checks that must hold in all of them (a shell sealed, water contained, a way out).
 *
 * A body with no pose here stands in its drawn place.
 */
USTRUCT()
struct MUSEEFOUNDATION_API FMuseeConfiguration
{
	GENERATED_BODY()

	/** The configuration's name, which says the state it is (`rotunda.cylinder-raised`). */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Name;

	/** One pose per body placed, each body at most once. */
	UPROPERTY()
	TArray<FMuseePose> Poses;

	/** The pose of a body, or null if the body stands in its drawn place. */
	[[nodiscard]] const FMuseePose* Find(const FMuseeKey& Body) const;

	/** Places a body: replaces its pose if it has one, adds it otherwise. */
	void Set(const FMuseePose& Pose);
};
