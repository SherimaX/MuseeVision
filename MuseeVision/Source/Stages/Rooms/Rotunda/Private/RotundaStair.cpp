#include "RotundaParts.h"
#include "RotundaPlan.h"

/**
 * The stair (§3.8), after Momo and Bramante: one wide helical flight round an open well Ø 4.40, descending anticlockwise
 * in plan (φ falling) from the raised drum's west door, by the south and east, to the landing, whose portal opens north.
 * Forty risers of 0.150: the top landing, eighteen treads, the mid landing, nineteen treads, and the last riser onto the
 * landing floor.
 *
 * Each tread is one stone, turned between its nosing lines, with its nosing swept along its riser. Form sweeps only
 * along paths in plan, so the helicoid soffit and the string's raked top are drawn as their nearest honest forms: each
 * tread's stone runs down to the soffit's line at its nosing, stepping as a stair of solid blocks does, and the string is
 * laid in one block per tread, its top 0.12 over the pitch line at the tread's middle.
 */
namespace Musee::Rotunda
{
	namespace
	{
		/** The nosing line of tread K (1 to 39), its front; Front(0) is the top landing's back edge. */
		double Front(int32 K)
		{
			if (K <= 0)
			{
				return Plan::TopLandingBackDeg;
			}
			if (K < Plan::MidLandingTread)
			{
				return Plan::FirstNosingDeg - (K - 1) * Plan::TreadDeg;
			}
			return Front(Plan::MidLandingTread - 1) - Plan::MidLandingDeg - (K - Plan::MidLandingTread) * Plan::TreadDeg;
		}

		double TreadTop(int32 K) { return -Plan::Riser * K; }

		/** A tread's stone runs down to the soffit's line under its nosing, and no lower than the landing it stands on. */
		double TreadBottom(int32 K) { return FMath::Max(TreadTop(K) - Plan::SoffitUnderPitch, Plan::LandingZ); }

		bool IsLanding(int32 K) { return K == 1 || K == Plan::MidLandingTread; }

		/** The string's top over tread K: flush with the top landing, 0.12 over the landings' level, else over the pitch line at mid-going. */
		double StringTop(int32 K)
		{
			if (K == 1)
			{
				return TreadTop(K);
			}
			const double Pitch = TreadTop(K) + (IsLanding(K) ? 0.0 : 0.5 * Plan::Riser);
			return FMath::Min(Pitch + Plan::StringOverPitch, Plan::StringCeiling);
		}

		/** The string runs half a degree past the last riser. */
		constexpr double StringPastDeg = 0.5;

		void Tread(TArray<FFormDrawing>& Out, int32 K)
		{
			const double From = Front(K);
			const double To = Front(K - 1);
			Out.Add(Drawn(FString::Printf(TEXT("stair.tread.%02d"), K), TEXT("stair.tread"), Ring(Plan::TreadInner - Plan::TreadInto,
				Plan::TreadOuter + Plan::TreadInto, TreadBottom(K), TreadTop(K), From, To), Walkable));

			// The nosing over the riser below, swept inwards along the riser line so its section faces forward, down the flight.
			FFormSweep Nosing;
			Nosing.Section = Profile(Plan::Nosing, true);
			Nosing.Path = FFormProfile::Polyline({Polar(Plan::TreadOuter, From), Polar(Plan::TreadInner, From)}, false);
			Nosing.ElevationMeters = TreadTop(K);
			Out.Add(Drawn(FString::Printf(TEXT("stair.tread.%02d.nosing"), K), TEXT("stair.tread"), MoveTemp(Nosing), nullptr));

			const bool bLast = K == Plan::Risers - 1;
			Out.Add(Drawn(FString::Printf(TEXT("stair.string.%02d"), K), TEXT("stair.string"), Ring(Plan::StringInner, Plan::TreadInner,
				TreadBottom(K), StringTop(K), bLast ? From - StringPastDeg : From, To), Solid));
		}
	}

	void DrawStair(TArray<FFormDrawing>& Out)
	{
		for (int32 K = 1; K < Plan::Risers; ++K)
		{
			Tread(Out, K);
		}
	}
}
