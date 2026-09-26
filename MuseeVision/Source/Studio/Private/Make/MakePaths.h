#pragma once

#include "CoreMinimal.h"

namespace Musee::Studio
{
	/**
	 * Where a make writes, and the names it gives what it writes. Every made asset lives under one root folder:
	 * /Game/Generated for the museum. Names are stable, so a remake replaces a package rather than adding one.
	 */
	struct FMakePaths
	{
		FString Root;

		/** A stage's mesh: <Root>/Stages/<Stage>/SM_<Stage>_<Piece>. */
		FString StageMesh(const FString& Stage, const FString& Piece) const
		{
			return FString::Printf(TEXT("%s/Stages/%s/SM_%s_%s"), *Root, *Stage, *Stage, *Piece);
		}

		/** A stage's level: <Root>/Stages/<Stage>/L_<Stage>. */
		FString StageLevel(const FString& Stage) const
		{
			return FString::Printf(TEXT("%s/Stages/%s/L_%s"), *Root, *Stage, *Stage);
		}

		/** The museum's level: <Root>/Site/L_Museum. */
		FString MuseumLevel() const
		{
			return FString::Printf(TEXT("%s/Site/L_Museum"), *Root);
		}
	};
}
