#include "RotundaParts.h"
#include "RotundaPlan.h"

/**
 * The eye (§3.5, D-004): the hall's one opening to the sky, a disc r 2.50 in the dome's curb. Under its laminated
 * low-iron glass a welded pearl-steel lattice spans it: a ring against the curb and 21 bars in three directions at
 * 60°, all three middle bars crossing at the centre in the bronze node, the sun clock's nodus.
 */
namespace Musee::Rotunda
{
	namespace
	{
		/** A half-plane of the lattice's plan: the points P with Normal · P at least Offset. */
		struct FHalfPlane
		{
			FVector2D Normal;
			double Offset;
		};

		/** A convex polygon cut by a half-plane (Sutherland and Hodgman), keeping its winding. */
		TArray<FVector2D> Cut(const TArray<FVector2D>& Polygon, const FHalfPlane& Plane)
		{
			TArray<FVector2D> Kept;
			for (int32 Index = 0; Index < Polygon.Num(); ++Index)
			{
				const FVector2D& A = Polygon[Index];
				const FVector2D& B = Polygon[(Index + 1) % Polygon.Num()];
				const double DA = FVector2D::DotProduct(Plane.Normal, A) - Plane.Offset;
				const double DB = FVector2D::DotProduct(Plane.Normal, B) - Plane.Offset;
				if (DA >= 0.0)
				{
					Kept.Add(A);
				}
				if ((DA >= 0.0) != (DB >= 0.0))
				{
					Kept.Add(A + (B - A) * (DA / (DA - DB)));
				}
			}
			return Kept;
		}

		double SignedArea(const TArray<FVector2D>& Polygon)
		{
			double Area = 0.0;
			for (int32 Index = 0; Index < Polygon.Num(); ++Index)
			{
				Area += FVector2D::CrossProduct(Polygon[Index], Polygon[(Index + 1) % Polygon.Num()]);
			}
			return 0.5 * Area;
		}

		/** Where a segment from A to B crosses the circle of a radius about the origin, as fractions along it, ascending. */
		TArray<double> Crossings(const FVector2D& A, const FVector2D& B, double Radius)
		{
			const FVector2D D = B - A;
			const double QA = D.SizeSquared();
			const double QB = 2.0 * FVector2D::DotProduct(A, D);
			const double QC = A.SizeSquared() - Radius * Radius;
			const double Discriminant = QB * QB - 4.0 * QA * QC;
			TArray<double> Out;
			if (QA <= 0.0 || Discriminant <= 0.0)
			{
				return Out;
			}
			const double Root = FMath::Sqrt(Discriminant);
			for (const double S : {(-QB - Root) / (2.0 * QA), (-QB + Root) / (2.0 * QA)})
			{
				if (S > 0.0 && S < 1.0)
				{
					Out.Add(S);
				}
			}
			return Out;
		}

		double AngleDeg(const FVector2D& P) { return FMath::RadiansToDegrees(FMath::Atan2(P.Y, P.X)); }

		/**
		 * A convex polygon (wound with positive area) clipped to the disc of a radius about the origin, as a closed
		 * profile whose stretches outside the disc follow its circle; unset when less than SmallestArea of it lies inside.
		 */
		TOptional<FFormProfile> InsideDisc(const TArray<FVector2D>& Polygon, double Radius, double SmallestArea)
		{
			struct FMark
			{
				FVector2D Point;
				bool bExit;
			};
			TArray<FMark> Marks;
			for (int32 Index = 0; Index < Polygon.Num(); ++Index)
			{
				const FVector2D& A = Polygon[Index];
				const FVector2D& B = Polygon[(Index + 1) % Polygon.Num()];
				if (A.Size() < Radius)
				{
					Marks.Add({A, false});
				}
				for (const double S : Crossings(A, B, Radius))
				{
					const FVector2D P = A + (B - A) * S;
					// Leaving the disc when the segment runs outwards there.
					Marks.Add({P, FVector2D::DotProduct(P, B - A) > 0.0});
				}
			}
			if (Marks.IsEmpty())
			{
				return {};
			}
			// The region's area: the polygon through the marks, and the circle's segment beyond each exit's chord.
			double Area = 0.0;
			for (int32 Index = 0; Index < Marks.Num(); ++Index)
			{
				const FMark& Here = Marks[Index];
				const FVector2D& Next = Marks[(Index + 1) % Marks.Num()].Point;
				Area += 0.5 * FVector2D::CrossProduct(Here.Point, Next);
				if (Here.bExit)
				{
					double Angle = FMath::Atan2(Next.Y, Next.X) - FMath::Atan2(Here.Point.Y, Here.Point.X);
					Angle = Angle <= 0.0 ? Angle + 2.0 * UE_DOUBLE_PI : Angle;
					Area += 0.5 * Radius * Radius * (Angle - FMath::Sin(Angle));
				}
			}
			if (Area < SmallestArea)
			{
				return {};
			}
			// Start just after an exit, so every exit's arc runs to the next mark, an entry.
			int32 First = 0;
			for (int32 Index = 0; Index < Marks.Num(); ++Index)
			{
				if (Marks[Index].bExit)
				{
					First = (Index + 1) % Marks.Num();
					break;
				}
			}
			FFormProfile Out = FFormProfile::From(Marks[First].Point);
			for (int32 Step = 0; Step < Marks.Num(); ++Step)
			{
				const FMark& Here = Marks[(First + Step) % Marks.Num()];
				if (Step > 0)
				{
					Out.LineTo(Here.Point, EFormJoint::Crisp);
				}
				if (Here.bExit)
				{
					const FVector2D& Next = Marks[(First + Step + 1) % Marks.Num()].Point;
					const double From = AngleDeg(Here.Point);
					double To = AngleDeg(Next);
					while (To <= From)
					{
						To += 360.0;
					}
					Out.ArcAbout(FVector2D::ZeroVector, Radius, Radius, From, To, EFormJoint::Crisp);
				}
			}
			Out.Close();
			return TOptional<FFormProfile>(MoveTemp(Out));
		}

		/**
		 * The openings between the bars, inside the ring's inner face: each is where one gap of every family of bars
		 * meets the others, clipped to the ring. Openings smaller than a square centimetre are left solid, as the
		 * welder's fillet would fill them.
		 */
		TArray<FFormProfile> LatticeOpenings()
		{
			constexpr double SmallestOpening = 1.0e-4;
			const double Beyond = 2.0 * Plan::LatticeRingOuter;
			const double Half = 0.5 * Plan::BarWidth;
			// Each family's gaps: below its first bar, between each pair, above its last.
			TArray<TPair<double, double>> Gaps;
			Gaps.Add({-Beyond, -Plan::BarsEachSide * Plan::BarPitch - Half});
			for (int32 K = -Plan::BarsEachSide; K < Plan::BarsEachSide; ++K)
			{
				Gaps.Add({K * Plan::BarPitch + Half, (K + 1) * Plan::BarPitch - Half});
			}
			Gaps.Add({Plan::BarsEachSide * Plan::BarPitch + Half, Beyond});
			TArray<FVector2D> Normals;
			for (const double Deg : Plan::BarDegrees)
			{
				Normals.Add(Polar(1.0, Deg + 90.0));
			}
			TArray<FFormProfile> Out;
			for (const TPair<double, double>& G0 : Gaps)
			{
				for (const TPair<double, double>& G1 : Gaps)
				{
					for (const TPair<double, double>& G2 : Gaps)
					{
						TArray<FVector2D> Cell = {FVector2D(-Beyond, -Beyond), FVector2D(Beyond, -Beyond), FVector2D(Beyond, Beyond), FVector2D(-Beyond, Beyond)};
						const TPair<double, double>* Bounds[3] = {&G0, &G1, &G2};
						for (int32 Family = 0; Family < 3 && Cell.Num() >= 3; ++Family)
						{
							Cell = Cut(Cell, {Normals[Family], Bounds[Family]->Key});
							Cell = Cut(Cell, {-Normals[Family], -Bounds[Family]->Value});
						}
						if (Cell.Num() < 3 || SignedArea(Cell) < SmallestOpening)
						{
							continue;
						}
						TOptional<FFormProfile> Hole = InsideDisc(Cell, Plan::LatticeRingInner, SmallestOpening);
						if (Hole.IsSet())
						{
							Out.Add(MoveTemp(Hole.GetValue()));
						}
					}
				}
			}
			return Out;
		}
	}

	void DrawEye(TArray<FFormDrawing>& Out)
	{
		Out.Add(Drawn(TEXT("eye.lattice.ring"), TEXT("eye.lattice"), Ring(Plan::LatticeRingInner, Plan::LatticeRingOuter, Plan::LatticeRingBottom,
			Plan::LatticeRingTop), nullptr));
		Out.Add(Drawn(TEXT("eye.lattice.bars"), TEXT("eye.lattice"), Slab(Circle(Plan::BarsReach), LatticeOpenings(), Plan::BarTop - Plan::BarBottom),
			nullptr, Raised(Plan::BarBottom)));

		FFormDrawing Glass = Drawn(Pieces::EyeGlass, TEXT("eye.glass"), Slab(Circle(Plan::GlassRadius), {}, Plan::GlassMeters), Solid,
			Raised(Plan::GlassBottom));
		Glass.bNanite = false;
		Out.Add(MoveTemp(Glass));

		FFormLathe Node;
		Node.Section = FFormProfile::From(FVector2D(0.0, -Plan::NodeRadius));
		Node.Section.ArcAbout(FVector2D::ZeroVector, Plan::NodeRadius, Plan::NodeRadius, -90.0, 90.0);
		Out.Add(Drawn(TEXT("eye.node"), TEXT("eye.node"), MoveTemp(Node), nullptr, Raised(Plan::NodeZ)));
	}
}
