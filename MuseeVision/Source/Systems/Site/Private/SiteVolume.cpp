#include "SiteVolume.h"

#include "SiteFindings.h"
#include "SitePlan.h"

/**
 * Overlap is measured by sweeping a line across both polygons. Between two heights where neither polygon has a corner and
 * no edge of one crosses an edge of the other, the edges each line cuts keep their order, so the stretch inside both
 * changes linearly and is largest at one end: reading it just inside both ends of every such band finds the largest.
 */
TArray<FSitePrism> FSiteFootprint::PrismsOrOutline() const
{
	if (!Prisms.IsEmpty())
	{
		return Prisms;
	}
	FSitePrism Whole;
	Whole.OutlineMeters = OutlineMeters;
	Whole.BottomMeters = BottomMeters;
	Whole.TopMeters = TopMeters;
	return {Whole};
}

namespace Musee::Site
{
	namespace
	{
		/** How far inside a band its ends are read, as a fraction of its height: clear of the corners at its ends. */
		constexpr double BandInset = 1.0e-6;

		/** Bands thinner than this, metres, are rounding between two equal heights. */
		constexpr double ThinnestBandMeters = 1.0e-12;

		/** Where a polygon's edges cross the line at height Y (no corner lies on it), in order along it. */
		TArray<double> Crossings(TConstArrayView<FVector2D> Polygon, double Y)
		{
			TArray<double> Along;
			for (int32 Index = 0; Index < Polygon.Num(); ++Index)
			{
				const FVector2D& A = Polygon[Index];
				const FVector2D& B = Polygon[(Index + 1) % Polygon.Num()];
				if ((A.Y <= Y) != (B.Y <= Y))
				{
					Along.Add(A.X + (Y - A.Y) * (B.X - A.X) / (B.Y - A.Y));
				}
			}
			Along.Sort();
			return Along;
		}

		/** The longest stretch of the line at height Y inside both polygons, by the even-odd rule. */
		double SharedAt(TConstArrayView<FVector2D> A, TConstArrayView<FVector2D> B, double Y)
		{
			const TArray<double> InA = Crossings(A, Y);
			const TArray<double> InB = Crossings(B, Y);
			double Longest = 0.0;
			for (int32 I = 0; I + 1 < InA.Num(); I += 2)
			{
				for (int32 J = 0; J + 1 < InB.Num(); J += 2)
				{
					Longest = FMath::Max(Longest, FMath::Min(InA[I + 1], InB[J + 1]) - FMath::Max(InA[I], InB[J]));
				}
			}
			return Longest;
		}

		/** The height where two segments cross within both, if they do (parallel segments never do). */
		TOptional<double> CrossingHeight(const FVector2D& P0, const FVector2D& P1, const FVector2D& Q0, const FVector2D& Q1)
		{
			const FVector2D R = P1 - P0;
			const FVector2D S = Q1 - Q0;
			const double Denominator = FVector2D::CrossProduct(R, S);
			if (FMath::IsNearlyZero(Denominator))
			{
				return {};
			}
			const double T = FVector2D::CrossProduct(Q0 - P0, S) / Denominator;
			const double U = FVector2D::CrossProduct(Q0 - P0, R) / Denominator;
			if (T <= 0.0 || T >= 1.0 || U <= 0.0 || U >= 1.0)
			{
				return {};
			}
			return P0.Y + T * R.Y;
		}

		/** Every height where the order along the sweep may change: each corner, and each crossing of an edge of A with one of B. */
		TArray<double> Breaks(TConstArrayView<FVector2D> A, TConstArrayView<FVector2D> B)
		{
			TArray<double> Heights;
			for (const FVector2D& Corner : A) { Heights.Add(Corner.Y); }
			for (const FVector2D& Corner : B) { Heights.Add(Corner.Y); }
			for (int32 I = 0; I < A.Num(); ++I)
			{
				for (int32 J = 0; J < B.Num(); ++J)
				{
					if (const TOptional<double> Y = CrossingHeight(A[I], A[(I + 1) % A.Num()], B[J], B[(J + 1) % B.Num()]))
					{
						Heights.Add(*Y);
					}
				}
			}
			Heights.Sort();
			return Heights;
		}

		TArray<FVector2D> Moved(TConstArrayView<FVector2D> Polygon, const FVector2D& By)
		{
			TArray<FVector2D> Out;
			for (const FVector2D& Point : Polygon) { Out.Add(Point + By); }
			return Out;
		}

		/** How far two placed stages' prisms overlap: the least of the plan's and the height's overlap, at the worst pair. */
		double PrismOverlap(const FSiteFootprint& A, const FVector& AtA, const FSiteFootprint& B, const FVector& AtB)
		{
			double Worst = 0.0;
			for (const FSitePrism& PA : A.PrismsOrOutline())
			{
				for (const FSitePrism& PB : B.PrismsOrOutline())
				{
					const double Height = FMath::Min(PA.TopMeters + AtA.Z, PB.TopMeters + AtB.Z) - FMath::Max(PA.BottomMeters + AtA.Z, PB.BottomMeters + AtB.Z);
					if (Height > OverlapToleranceMeters)
					{
						const double Plan = OverlapMeters(Moved(PA.OutlineMeters, FVector2D(AtA)), Moved(PB.OutlineMeters, FVector2D(AtB)));
						Worst = FMath::Max(Worst, FMath::Min(Plan, Height));
					}
				}
			}
			return Worst;
		}

		/** Whether both stages are turned about one axis once placed. */
		bool Coaxial(const FSiteFootprint& A, const FVector& AtA, const FSiteFootprint& B, const FVector& AtB)
		{
			return A.TurnedProfileMeters.Num() >= 3 && B.TurnedProfileMeters.Num() >= 3
				&& FVector2D::Distance(A.AxisMeters + FVector2D(AtA), B.AxisMeters + FVector2D(AtB)) <= OverlapToleranceMeters;
		}
	}

	double OverlapMeters(TConstArrayView<FVector2D> A, TConstArrayView<FVector2D> B)
	{
		if (A.Num() < 3 || B.Num() < 3)
		{
			return 0.0;
		}
		const TArray<double> Heights = Breaks(A, B);
		double Longest = 0.0;
		for (int32 Band = 0; Band + 1 < Heights.Num(); ++Band)
		{
			const double Low = Heights[Band];
			const double High = Heights[Band + 1];
			if (High - Low <= ThinnestBandMeters)
			{
				continue;
			}
			for (const double At : {BandInset, 0.5, 1.0 - BandInset})
			{
				Longest = FMath::Max(Longest, SharedAt(A, B, FMath::Lerp(Low, High, At)));
			}
		}
		return Longest;
	}

	TArray<FMuseeFinding> CheckOverlaps(const TMap<FMuseeKey, FSiteFootprint>& Footprints, const FSiteLayout& Layout)
	{
		TArray<FMuseeKey> Stages;
		for (const TPair<FMuseeKey, FSiteFootprint>& Entry : Footprints)
		{
			if (Entry.Value.Kind == ESiteFootprintKind::Building && Layout.TranslationsMeters.Contains(Entry.Key)) { Stages.Add(Entry.Key); }
		}
		Stages.Sort([](const FMuseeKey& A, const FMuseeKey& B) { return A.ToString() < B.ToString(); });
		TArray<FMuseeFinding> Findings;
		for (int32 I = 0; I < Stages.Num(); ++I)
		{
			for (int32 J = I + 1; J < Stages.Num(); ++J)
			{
				const FSiteFootprint& A = Footprints[Stages[I]];
				const FSiteFootprint& B = Footprints[Stages[J]];
				const FVector& AtA = Layout.TranslationsMeters[Stages[I]];
				const FVector& AtB = Layout.TranslationsMeters[Stages[J]];
				double Overlap = PrismOverlap(A, AtA, B, AtB);
				if (Overlap > OverlapToleranceMeters && Coaxial(A, AtA, B, AtB))
				{
					Overlap = OverlapMeters(Moved(A.TurnedProfileMeters, FVector2D(0.0, AtA.Z)), Moved(B.TurnedProfileMeters, FVector2D(0.0, AtB.Z)));
				}
				if (Overlap > OverlapToleranceMeters)
				{
					Findings.Add(SiteFinding(TEXT("Site.NoOverlap"), FName(*Stages[J].ToString()), FString::Printf(
						TEXT("the volumes of %s and %s overlap by %.4f m"), *Stages[I].ToString(), *Stages[J].ToString(), Overlap),
						TEXT("overlap, m"), Overlap, 0.0, OverlapToleranceMeters, Stages[J]));
				}
			}
		}
		return Findings;
	}
}
