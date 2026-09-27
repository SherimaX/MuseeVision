#include "AlbionParts.h"

/**
 * The lancets (§3.1, §3.2, §3.5, §3.6), one in each bay of each side wall: on the glass line, the light's leaded glass
 * (the outline grown 0.01 into its groove), glazed tonight in plain diamond quarries whose lead cames are one lattice
 * (the designs wait for their catalogue ids, §13; the Glazing anchors are reserved for them); the protective pane 0.05
 * outside; the wrought saddle bars 0.015 inside; and outside, the sill weathered from the glass to the face over its drip
 * block.
 */
namespace Musee::Albion
{
	namespace
	{
		/** The lancet's clear light as a convex polygon, anticlockwise, its head's arcs divided finely for clipping. */
		constexpr int32 HeadSteps = 32;

		TArray<FVector2D> LightPolygon(const FArch& A)
		{
			TArray<FVector2D> Points = {FVector2D(A.Centre - A.Half, A.Sill), FVector2D(A.Centre + A.Half, A.Sill), FVector2D(A.Centre + A.Half, A.Springing)};
			const double Rise = A.Crown() - A.Springing;
			for (int32 Step = 1; Step < HeadSteps; ++Step)
			{
				const double Z = A.Springing + Rise * Step / HeadSteps;
				Points.Add(FVector2D(A.Centre + A.HalfAt(Z), Z));
			}
			Points.Add(FVector2D(A.Centre, A.Crown()));
			for (int32 Step = HeadSteps - 1; Step >= 1; --Step)
			{
				const double Z = A.Springing + Rise * Step / HeadSteps;
				Points.Add(FVector2D(A.Centre - A.HalfAt(Z), Z));
			}
			Points.Add(FVector2D(A.Centre - A.Half, A.Springing));
			return Points;
		}

		/** The part of a polygon on the left of the line A to B, moved in from it by Inset (Sutherland and Hodgman). */
		TArray<FVector2D> KeepLeft(const TArray<FVector2D>& Points, const FVector2D& A, const FVector2D& B, double Inset)
		{
			const FVector2D Edge = (B - A).GetSafeNormal();
			auto Side = [&](const FVector2D& P) { return FVector2D::CrossProduct(Edge, P - A) - Inset; };
			TArray<FVector2D> Kept;
			for (int32 Index = 0; Index < Points.Num(); ++Index)
			{
				const FVector2D& P = Points[Index];
				const FVector2D& Q = Points[(Index + 1) % Points.Num()];
				const double SP = Side(P);
				const double SQ = Side(Q);
				if (SP >= 0.0)
				{
					Kept.Add(P);
				}
				if ((SP >= 0.0) != (SQ >= 0.0))
				{
					Kept.Add(P + (Q - P) * (SP / (SP - SQ)));
				}
			}
			return Kept;
		}

		/** A polygon clipped to a convex one (anticlockwise), then moved in from every edge of the result by Inset. */
		TArray<FVector2D> ClipAndInset(TArray<FVector2D> Shape, const TArray<FVector2D>& Convex, double Inset)
		{
			for (int32 Index = 0; Index < Convex.Num() && Shape.Num() >= 3; ++Index)
			{
				Shape = KeepLeft(Shape, Convex[Index], Convex[(Index + 1) % Convex.Num()], 0.0);
			}
			const TArray<FVector2D> Clipped = Shape;
			for (int32 Index = 0; Index < Clipped.Num() && Shape.Num() >= 3; ++Index)
			{
				Shape = KeepLeft(Shape, Clipped[Index], Clipped[(Index + 1) % Clipped.Num()], Inset);
			}
			return Shape;
		}

		double Area(const TArray<FVector2D>& Points)
		{
			double Twice = 0.0;
			for (int32 Index = 0; Index < Points.Num(); ++Index)
			{
				Twice += FVector2D::CrossProduct(Points[Index], Points[(Index + 1) % Points.Num()]);
			}
			return 0.5 * FMath::Abs(Twice);
		}

		/** The smallest glass a quarry may show between its cames: a centimetre square; smaller, the lead runs solid. */
		constexpr double SmallestQuarry = 1.0e-4;

		/**
		 * The quarries' windows in the came lattice: diamonds 0.13 wide and 0.18 high, set on the lancet's axis from its
		 * sill, clipped to the light and each moved in by half a came.
		 */
		TArray<FFormProfile> QuarryWindows(const FArch& Light)
		{
			const TArray<FVector2D> Convex = LightPolygon(Light);
			const double W = Plan::QuarryWidth;
			const double H = Plan::QuarryHigh;
			const int32 Across = FMath::CeilToInt32(Light.Half / W) + 1;
			const int32 Up = FMath::CeilToInt32((Light.Crown() - Light.Sill) / H) + 1;
			TArray<FFormProfile> Windows;
			for (int32 Row = 0; Row <= 2 * Up; ++Row)
			{
				const double Z = Light.Sill + 0.5 * H * Row;
				const double Shift = (Row % 2) * 0.5 * W;
				for (int32 Column = -Across; Column <= Across; ++Column)
				{
					const double U = Light.Centre + W * Column + Shift;
					const TArray<FVector2D> Diamond = {FVector2D(U, Z - 0.5 * H), FVector2D(U + 0.5 * W, Z), FVector2D(U, Z + 0.5 * H), FVector2D(U - 0.5 * W, Z)};
					const TArray<FVector2D> Window = ClipAndInset(Diamond, Convex, 0.5 * Plan::CameMeters);
					if (Window.Num() >= 3 && Area(Window) >= SmallestQuarry)
					{
						Windows.Add(FFormProfile::Polygon(Window));
					}
				}
			}
			return Windows;
		}

		/** One lancet's glass, lattice, guard, saddle bars, outer sill and drip, in bay 1 to 6 of the west or east wall. */
		void OneLancet(bool bWest, int32 Bay, TArray<FFormDrawing>& Out)
		{
			const double S = bWest ? -1.0 : 1.0;
			const FString Name = FString::Printf(TEXT("lancet.%s%d"), bWest ? TEXT("w") : TEXT("e"), Bay);
			const FArch Light = Lancet(Bay);
			const FArch Glazed = Light.Grown(Plan::LancetGlassGrow);
			const double Line = S * Plan::SideGlassX;
			// AcrossX pushes east: each sheet from its west face.
			auto Sheet = [&](double Middle, double Thick) { return AcrossX(Middle - 0.5 * Thick); };

			FFormExtrusion Leaded;
			Leaded.Outline = Glazed.Outline();
			Leaded.DepthMeters = Plan::LeadedGlassMeters;
			Out.Add(Glass(Drawn(Name + TEXT(".glass"), TEXT("lancet.glass"), MoveTemp(Leaded), Overhead, Sheet(Line, Plan::LeadedGlassMeters))));

			FFormExtrusion Lattice;
			Lattice.Outline = Glazed.Outline();
			Lattice.Holes = QuarryWindows(Light);
			Lattice.DepthMeters = Plan::CameMeters;
			Out.Add(Drawn(Name + TEXT(".came"), TEXT("came"), MoveTemp(Lattice), Overhead, Sheet(Line, Plan::CameMeters)));

			// The protective pane stands on the weathered sill, 0.05 outside the glass.
			const double Slope = (Plan::OuterSillAtGlass - Plan::LancetSill) / (Plan::SideOuterX - Plan::SideGlassX);
			FArch Guarded = Glazed;
			Guarded.Sill = Plan::OuterSillAtGlass - Slope * Plan::GuardOffset;
			FFormExtrusion Guard;
			Guard.Outline = Guarded.Outline();
			Guard.DepthMeters = Plan::LaminatedMeters;
			const double GuardAt = Line + S * (Plan::GuardOffset + 0.5 * Plan::LaminatedMeters);
			Out.Add(Glass(Drawn(Name + TEXT(".guard"), TEXT("guard-glazing"), MoveTemp(Guard), Overhead, Sheet(GuardAt, Plan::LaminatedMeters))));

			const double BarX = Line - S * Plan::SaddleInside;
			const double Reach = Light.Half + Plan::SaddleReach;
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::SaddleZ)); ++Index)
			{
				const double Z = Plan::SaddleZ[Index];
				const FVector Half(0.5 * Plan::SaddleBar, 0.0, 0.5 * Plan::SaddleBar);
				Out.Add(Box(FString::Printf(TEXT("%s.saddle.%d"), *Name, Index + 1), TEXT("iron.wrought"),
					FVector(BarX, Light.Centre - Reach, Z) - Half, FVector(BarX, Light.Centre + Reach, Z) + Half, Overhead));
			}

			const double Face = S * Plan::SideOuterX;
			FFormExtrusion Sill;
			Sill.Outline = Section({FVector2D(Line, Plan::LancetSill), FVector2D(Face, Plan::LancetSill), FVector2D(Line, Plan::OuterSillAtGlass)});
			Sill.DepthMeters = 2.0 * Light.Half;
			Out.Add(Drawn(Name + TEXT(".sill"), TEXT("ext.dressing"), MoveTemp(Sill), Overhead, AcrossY(Light.Centre + Light.Half)));
			const double Tip = Face + S * Plan::DripProud;
			Out.Add(Box(Name + TEXT(".drip"), TEXT("ext.dressing"), FVector(FMath::Min(Face, Tip), Light.Centre - Plan::DripHalf, Plan::DripFoot),
				FVector(FMath::Max(Face, Tip), Light.Centre + Plan::DripHalf, Plan::LancetSill), Overhead));
		}
	}

	void DrawLancets(TArray<FFormDrawing>& Out)
	{
		for (const bool bWest : {true, false})
		{
			for (int32 Bay = 1; Bay <= Plan::Lines - 1; ++Bay)
			{
				OneLancet(bWest, Bay, Out);
			}
		}
	}
}
