#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"

#include "MotionFollowing.generated.h"

/**
 * How a mechanism follows another's channel instead of a control, as guard rails follow the height of the pond they
 * guard: its travel is linear from Out[0] at the channel's value In[0] to Out[1] at In[1], clamped outside that range.
 * Out is in the joint's travel units (metres or degrees).
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

	/** The travel at each of them [t0, t1]. */
	UPROPERTY(meta = (MuseeRequired))
	TArray<double> Out;

	/** Whether the mechanism follows a channel. */
	[[nodiscard]] bool IsSet() const { return !Channel.IsNone(); }

	/** The travel the mechanism follows to at a channel value. */
	[[nodiscard]] double TravelAt(double Value) const
	{
		if (In.Num() != 2 || Out.Num() != 2 || In[0] == In[1])
		{
			return 0.0;
		}
		const double Share = FMath::Clamp((Value - In[0]) / (In[1] - In[0]), 0.0, 1.0);
		return FMath::Lerp(Out[0], Out[1], Share);
	}
};
