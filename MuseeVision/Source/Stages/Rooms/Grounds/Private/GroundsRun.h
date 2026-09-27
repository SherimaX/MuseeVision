#pragma once

#include "CoreMinimal.h"
#include "FormProfile.h"

/**
 * A mason's run in plan: the centre line a line of stones is set along (a kerb, a coping, the strip's edge), made of
 * straight legs and arcs about a centre, open or closed. Its stones are set out leg by leg, each leg divided evenly
 * into stones as near as it comes to their nominal length; a joint falls on every corner, and the stones meeting there
 * are mitred on the corner's bisecting plane, so they meet exactly and the section keeps its shape round the turn.
 */
namespace Musee::Grounds
{
	/** One leg of a run: a straight line, or an arc about a centre from one plan angle to another (either way round). */
	struct FRunLeg
	{
		FVector2D Start = FVector2D::ZeroVector;
		FVector2D End = FVector2D::ZeroVector;
		bool bArc = false;
		FVector2D Centre = FVector2D::ZeroVector;
		double Radius = 0.0;
		double FromDegrees = 0.0;
		double ToDegrees = 0.0;

		[[nodiscard]] double Length() const;

		/** The point a distance along the leg from its start. */
		[[nodiscard]] FVector2D PointAt(double Distance) const;

		/** The unit direction of travel a distance along the leg. */
		[[nodiscard]] FVector2D DirectionAt(double Distance) const;

		/** Carries a profile standing at the point Distance0 along to the point Distance1. */
		void Extend(FFormProfile& Path, double Distance0, double Distance1) const;
	};

	/** A stone of a run: the stretch of its centre line it covers, and its ends' cut planes (zero where square). */
	struct FRunStone
	{
		FFormProfile Path;
		FVector StartCut = FVector::ZeroVector;
		FVector EndCut = FVector::ZeroVector;
	};

	class FRun
	{
	public:
		/** A run starting at a point. */
		[[nodiscard]] static FRun From(const FVector2D& Point);

		/** A run through points, straight between them, closed back to the first if asked. */
		[[nodiscard]] static FRun Through(TConstArrayView<FVector2D> Points, bool bLoop);

		/** A straight leg from the run's end to a point. */
		FRun& LineTo(const FVector2D& Point);

		/**
		 * An arc about a centre from the run's end (which lies on it) to a plan angle, degrees, turning the way asked: with
		 * the plan angle increasing (east towards south) or decreasing.
		 */
		FRun& ArcTo(const FVector2D& Centre, double ToDegrees, bool bIncreasing);

		/** A closed run returns to its start with a straight leg, if it isn't there already. */
		FRun& Close();

		[[nodiscard]] const TArray<FRunLeg>& Legs() const { return RunLegs; }
		[[nodiscard]] bool IsClosed() const { return bClosed; }
		[[nodiscard]] double Length() const;
		[[nodiscard]] FVector2D Start() const { return RunStart; }
		[[nodiscard]] FVector2D End() const;

		/** The whole run as a plan path. */
		[[nodiscard]] FFormProfile Path() const;

		/** The run's stones, leg by leg, each leg divided evenly into stones as near as it comes to Nominal long. */
		[[nodiscard]] TArray<FRunStone> Stones(double Nominal) const;

	private:
		/** The mitre plane's normal where leg Index starts (the corner with the leg before it), or zero where square. */
		FVector MitreBefore(int32 Index) const;

		FVector2D RunStart = FVector2D::ZeroVector;
		TArray<FRunLeg> RunLegs;
		bool bClosed = false;
	};
}
