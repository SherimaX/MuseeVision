#pragma once

#include "CoreMinimal.h"

/**
 * Chenghuai's tunable numbers as data, for live geometry rounds: a game started with -Live reads
 * Saved/Live/chenghuai.json ({"tail.Length": 0.7, "hall.Main hall.EaveFront": 1.2, …}) and rebuilds the wing within a
 * moment of a change (UMuseeLiveGeometry). Without -Live, and for every key the file doesn't name, the number is the
 * default written in the code, so the cooked wing is exactly as before. A value that is right goes back into the code
 * as its default, as a look.json value goes back into its script.
 *
 * Every key the build asked for, with its default, is listed in Saved/Live/chenghuai_keys.json after a build.
 */
namespace ChenghuaiTune
{
	/** Reads Saved/Live/chenghuai.json (only with -Live; otherwise every number keeps its default). */
	MUSEEVISION_API void Load();

	/** The number for Key: the file's value, else Default. Records the key and its default. */
	MUSEEVISION_API double Num(const FString& Key, double Default);

	/** Writes the keys asked for since the last Load, with their defaults and current values; returns the file's keys
	 *  that no build asked for (misspelt, or not tunable). */
	MUSEEVISION_API TArray<FString> WriteKeys();

	/** Saved/Live/chenghuai.json. */
	MUSEEVISION_API FString Path();

	/** Bumped by every Load: caches of tuned values (the rockery's peaks and hollows) rebuild when it moves. */
	MUSEEVISION_API int32 Generation();
}
