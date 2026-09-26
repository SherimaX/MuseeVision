#pragma once

#include "CoreMinimal.h"

namespace Musee::Studio
{
	/**
	 * The museum measures in metres; the engine measures in centimetres. This is Studio's one crossing between them:
	 * 100 centimetres to the metre, by the SI definition of the centimetre.
	 */
	inline constexpr double EngineUnitsPerMeter = 100.0;

	/** A position or offset in metres, in engine units. */
	inline FVector EngineFromMeters(const FVector& Meters)
	{
		return Meters * EngineUnitsPerMeter;
	}

	/** A length in engine units, in metres. */
	inline double MetersFromEngine(double EngineLength)
	{
		return EngineLength / EngineUnitsPerMeter;
	}
}
