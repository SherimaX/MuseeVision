#pragma once

#include "CoreMinimal.h"

#include "LightOperation.generated.h"

/**
 * How a luminaire is switched through the day and night, as a modern gallery's controls do it (Decision D-006: the
 * museum is always open). The numbers each way uses are in Data/Light/Operation.json.
 */
UENUM()
enum class ELightOperation : uint8
{
	/** Always on. */
	Always,
	/** On from sunset to sunrise, as an astronomical time switch runs architectural light: on while the museum's sun is down. */
	Dusk,
	/** On while a presence detector beside it sees the visitor, and off when it has seen nobody for its hold time. */
	Presence,
};
