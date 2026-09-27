#pragma once

#include "CoreMinimal.h"

class UObject;
class UPrimitiveComponent;
class AActor;

/**
 * What the visitor's gaze finds: the thing a hit belongs to, by the Foundation contracts it implements. A thing is found
 * on the component hit, on the components it hangs from, or on its actor, so an owner may make a usable volume, a
 * usable piece or a usable actor alike. A usable answers the Visibility trace, by blocking it or by overlapping it.
 */
namespace Musee::Visitor
{
	/** The usable thing (IMuseeUsable) a hit belongs to, or null. */
	UObject* FindUsable(UPrimitiveComponent* Hit, AActor* Actor);

	/** The captioned thing (IMuseeCaptioned) a hit belongs to, or null. */
	UObject* FindCaptioned(UPrimitiveComponent* Hit, AActor* Actor);
}
