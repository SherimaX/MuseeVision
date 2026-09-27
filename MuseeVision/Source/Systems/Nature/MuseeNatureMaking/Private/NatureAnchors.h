#pragma once

#include "CoreMinimal.h"

/** The anchor kinds Nature reads from a stage (Docs/Stages contract), and the one it follows onto a moving body. */
namespace Musee::NatureMaking::AnchorKind
{
	inline const FName Water(TEXT("Water"));
	inline const FName Bed(TEXT("Bed"));
	inline const FName Body(TEXT("Body"));
}
