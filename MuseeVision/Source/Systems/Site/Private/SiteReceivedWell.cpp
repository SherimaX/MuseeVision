#include "SiteReceivedWell.h"

#include "SiteFindings.h"
#include "SitePlan.h"

/**
 * A triangle enters the well when some point of it, between the well's bottom and top, stands nearer the axis than the
 * circle and outside every notch. The triangle is cut to the well's height, leaving a convex polygon in plan; the nearest
 * point of a convex polygon to the axis says whether it reaches inside a circle; and a notch is taken out by cutting the
 * polygon into the convex pieces beside it, behind the axis, and within it, where only the part nearer than the notch's
 * inner radius is in the well.
 */
namespace Musee::Site
{
	namespace
	{
		/** A polygon's corners in plan, about the well's axis, metres. */
		using FPlanPolygon = TArray<FVector2D, TInlineAllocator<12>>;

		/** The part of a convex polygon where Normal . P <= Offset. */
		FPlanPolygon Clip(const FPlanPolygon& Polygon, const FVector2D& Normal, double Offset)
		{
			FPlanPolygon Kept;
			for (int32 Index = 0; Index < Polygon.Num(); ++Index)
			{
				const FVector2D& A = Polygon[Index];
				const FVector2D& B = Polygon[(Index + 1) % Polygon.Num()];
				const double DA = FVector2D::DotProduct(Normal, A) - Offset;
				const double DB = FVector2D::DotProduct(Normal, B) - Offset;
				if (DA <= 0.0) { Kept.Add(A); }
				if ((DA < 0.0) != (DB < 0.0) && DA != DB) { Kept.Add(A + (B - A) * (DA / (DA - DB))); }
			}
			return Kept;
		}

		/**
		 * How near a convex polygon comes to the axis, metres: nothing when the axis lies inside it (on the same side of
		 * every edge), else its nearest edge. A polygon collapsed to a segment or a point (a wall's face seen from above) has
		 * no inside, even on a line through the axis, so only its edges count.
		 */
		double NearestToAxis(const FPlanPolygon& Polygon)
		{
			if (Polygon.IsEmpty())
			{
				return TNumericLimits<double>::Max();
			}
			bool bAround = Polygon.Num() >= 3;
			double Sign = 0.0;
			double Nearest = TNumericLimits<double>::Max();
			for (int32 Index = 0; Index < Polygon.Num(); ++Index)
			{
				const FVector2D& A = Polygon[Index];
				const FVector2D& B = Polygon[(Index + 1) % Polygon.Num()];
				const double Side = FVector2D::CrossProduct(B - A, -A);
				bAround &= Sign == 0.0 || Side == 0.0 || (Side > 0.0) == (Sign > 0.0);
				Sign = Side != 0.0 ? Side : Sign;
				const double Length = (B - A).SizeSquared();
				const double T = Length > 0.0 ? FMath::Clamp(FVector2D::DotProduct(-A, B - A) / Length, 0.0, 1.0) : 0.0;
				Nearest = FMath::Min(Nearest, (A + (B - A) * T).Size());
			}
			return bAround && Sign != 0.0 ? 0.0 : Nearest;
		}

		/** The direction of a bearing in plan: x east, y south, so north is -y. */
		FVector2D OfBearing(double Degrees)
		{
			const double Radians = FMath::DegreesToRadians(Degrees);
			return FVector2D(FMath::Sin(Radians), -FMath::Cos(Radians));
		}

		/** Whether a convex polygon reaches inside the class's circle outside the notches from the Notch-th on. */
		bool Enters(const FPlanPolygon& Polygon, const FSitePortClass& Class, int32 Notch)
		{
			if (NearestToAxis(Polygon) >= Class.WellRadiusMeters - LoopToleranceMeters)
			{
				return false;
			}
			if (Notch == Class.WellNotches.Num())
			{
				return true;
			}
			const FSiteWellNotch& Cut = Class.WellNotches[Notch];
			const FVector2D Along = OfBearing(Cut.BearingDegrees);
			const FVector2D Across(-Along.Y, Along.X);
			const double Half = Cut.HalfWidthMeters + LoopToleranceMeters;
			if (Enters(Clip(Polygon, Across, -Half), Class, Notch + 1) || Enters(Clip(Polygon, -Across, -Half), Class, Notch + 1))
			{
				return true;
			}
			const FPlanPolygon Within = Clip(Clip(Polygon, Across, Half), -Across, Half);
			if (Enters(Clip(Within, Along, 0.0), Class, Notch + 1))
			{
				return true;
			}
			return NearestToAxis(Clip(Within, -Along, 0.0)) < Cut.InnerRadiusMeters - LoopToleranceMeters;
		}

		/** A triangle cut to the well's height, in plan about the axis; empty when it lies wholly above or below. */
		FPlanPolygon InWellHeight(const FVector* Corners, const FSiteWell& Well, const FVector2D& Axis)
		{
			TArray<FVector, TInlineAllocator<12>> Polygon = {Corners[0], Corners[1], Corners[2]};
			for (const double Sign : {1.0, -1.0})
			{
				// Keep Sign * z <= Limit: below the top, then above the bottom, each a tolerance inside.
				const double Limit = Sign > 0.0 ? Well.MaxMeters.Z - LoopToleranceMeters : -(Well.MinMeters.Z + LoopToleranceMeters);
				TArray<FVector, TInlineAllocator<12>> Kept;
				for (int32 Index = 0; Index < Polygon.Num(); ++Index)
				{
					const FVector& A = Polygon[Index];
					const FVector& B = Polygon[(Index + 1) % Polygon.Num()];
					const double DA = Sign * A.Z - Limit;
					const double DB = Sign * B.Z - Limit;
					if (DA <= 0.0) { Kept.Add(A); }
					if ((DA < 0.0) != (DB < 0.0) && DA != DB) { Kept.Add(A + (B - A) * (DA / (DA - DB))); }
				}
				Polygon = MoveTemp(Kept);
			}
			FPlanPolygon Plan;
			for (const FVector& Corner : Polygon) { Plan.Add(FVector2D(Corner) - Axis); }
			return Plan;
		}

		/** How near the axis a piece comes inside the well's section, metres, or a negative number when it stays out. */
		double Reach(const FSiteWellPiece& Piece, const FSiteWell& Well, const FSitePortClass& Class, const FVector2D& Axis)
		{
			const FBox Bounds(Well.MinMeters, Well.MaxMeters);
			double Nearest = -1.0;
			for (int32 Corner = 0; Corner + 2 < Piece.TrianglesMeters.Num(); Corner += 3)
			{
				const FVector* Corners = &Piece.TrianglesMeters[Corner];
				if (!FBox(Corners, 3).Intersect(Bounds))
				{
					continue;
				}
				const FPlanPolygon Plan = InWellHeight(Corners, Well, Axis);
				if (Enters(Plan, Class, 0))
				{
					const double Here = NearestToAxis(Plan);
					Nearest = Nearest < 0.0 ? Here : FMath::Min(Nearest, Here);
				}
			}
			return Nearest;
		}
	}

	FName PartOf(const FSiteWell& Well, FName Piece)
	{
		const FString Name = Piece.ToString();
		for (const FSiteWellPart& Part : Well.Parts)
		{
			if (Name.StartsWith(Part.PiecePrefix, ESearchCase::CaseSensitive))
			{
				return Part.Part;
			}
		}
		return NAME_None;
	}

	TArray<FMuseeFinding> CheckReceivedWell(const FSitePort& Port, const FSitePortClass& Class, TConstArrayView<FSiteWellPiece> Pieces)
	{
		TArray<FMuseeFinding> Findings;
		const FName Drawer(*StageOfPort(Port.Key).ToString());
		for (const FSiteWellPart& Part : Port.Well.Parts)
		{
			if (!Class.WellAdmits.Contains(Part.Part))
			{
				Findings.Add(SiteFinding(TEXT("Site.RoomStaysInside"), Drawer, FString::Printf(TEXT("the well of %s admits no %s (%s*)"),
					*Port.Key.ToString(), *Part.Part.ToString(), *Part.PiecePrefix), TEXT("parts the class doesn't admit"), 1.0, 0.0, 0.0, Port.Key));
			}
		}
		if (!Port.bHasWell || Port.Well.bServing || Class.WellRadiusMeters <= 0.0)
		{
			return Findings;
		}
		const FVector2D Axis = 0.5 * (FVector2D(Port.Well.MinMeters) + FVector2D(Port.Well.MaxMeters));
		for (const FSiteWellPiece& Piece : Pieces)
		{
			if (!PartOf(Port.Well, Piece.Name).IsNone())
			{
				continue;
			}
			const double Nearest = Reach(Piece, Port.Well, Class, Axis);
			if (Nearest >= 0.0)
			{
				Findings.Add(SiteFinding(TEXT("Site.RoomStaysInside"), Drawer, FString::Printf(
					TEXT("piece %s reaches into the well %s receives, %.3f m from its axis, and is no part its class admits"), *Piece.Name.ToString(),
					*Port.Key.ToString(), Nearest), TEXT("nearest to the well's axis, m"), Nearest, Class.WellRadiusMeters, TNumericLimits<double>::Max(),
					Port.Key));
			}
		}
		return Findings;
	}
}
