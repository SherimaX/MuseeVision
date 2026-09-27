#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"

#include "MotionFollowing.generated.h"

/**
 * How one thing follows another's channel, as a cam follows its shaft: guard rails that rise with the pond they guard,
 * a lift's door leaves with their control point, a light that dims while a work performs, a mast column that grows as
 * the car climbs. The follower's value is linear from Out[0] at the channel's value In[0] to Out[1] at In[1], clamped
 * outside that range. Out is in the follower's own units: a mechanism's travel (metres or degrees), a derived
 * channel's value (0 to 1), a length (metres).
 */
USTRUCT()
struct MUSEEMOTION_API FMotionFollowing
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Channel;

	/** The channel's values [c0, c1]. */
	UPROPERTY(meta = (MuseeRequired))
	TArray<double> In;

	/** The follower's values at each of them [v0, v1]. */
	UPROPERTY(meta = (MuseeRequired))
	TArray<double> Out;

	/** Whether anything is followed. */
	[[nodiscard]] bool IsSet() const { return !Channel.IsNone(); }

	/** Whether the map is sound: two different channel values, and two follower values. */
	[[nodiscard]] bool IsSound() const { return In.Num() == 2 && Out.Num() == 2 && In[0] != In[1]; }

	/** The follower's value at a channel value. */
	[[nodiscard]] double At(double Value) const
	{
		if (!IsSound())
		{
			return 0.0;
		}
		const double Share = FMath::Clamp((Value - In[0]) / (In[1] - In[0]), 0.0, 1.0);
		return FMath::Lerp(Out[0], Out[1], Share);
	}
};
