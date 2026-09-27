#include "MotionProfile.h"

namespace Musee::Motion
{
	TOptional<EProfile> ParseProfile(FName Name)
	{
		if (Name == TEXT("linear"))
		{
			return EProfile::Linear;
		}
		if (Name == TEXT("ease-in-out"))
		{
			return EProfile::EaseInOut;
		}
		return {};
	}

	double Covered(EProfile Profile, double T)
	{
		const double S = FMath::Clamp(T, 0.0, 1.0);
		switch (Profile)
		{
		case EProfile::EaseInOut:
			return S * S * S * (10.0 + S * (-15.0 + 6.0 * S));
		default:
			return S;
		}
	}

	double RelativeSpeed(EProfile Profile, double T)
	{
		const double S = FMath::Clamp(T, 0.0, 1.0);
		switch (Profile)
		{
		case EProfile::EaseInOut:
			return 30.0 * S * S * (1.0 - S) * (1.0 - S);
		default:
			return 1.0;
		}
	}
}
