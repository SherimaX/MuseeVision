#include "NympheasParts.h"
#include "NympheasPlan.h"

/**
 * The bronze under the glass (§3.3): the velarium's frame (the tees lying on the cloth over its seams, their ends against
 * the rim ring, and the rods they hang by) and the dome's glazing bars with the curb they bear on. The cloth and the glass
 * are paraboloids over the rim line, so the tees and bars are drawn on vertical faces, each following its rise in its own
 * plane: a tee rests on the cloth and a bar carries the glass at its uphill edge, the other edge standing off the slope
 * across it, as a rigid section does. The bars along x run whole over the seams; those along y are cut between them, their
 * ends chamfered against the bars' sides and against the curb.
 */
namespace Musee::Nympheas
{
	namespace
	{
		/** The rim line: the cove's rim, the ellipse the cloth and the glass spring from. */
		const FOvalCurve& Rim()
		{
			static const FOvalCurve Curve(Plan::RimA, Plan::RimB);
			return Curve;
		}

		/** A paraboloid over the rim line rising Rise: its height above its springing at (x, y). */
		double Paraboloid(double X, double Y, double Rise)
		{
			return Rise * (1.0 - FMath::Square(X / Plan::RimA) - FMath::Square(Y / Plan::RimB));
		}

		/** The paraboloid's slope at (x, y): its fall per metre along x and along y. */
		FVector2D SlopeOf(double X, double Y, double Rise)
		{
			return FVector2D(-2.0 * Rise * X / FMath::Square(Plan::RimA), -2.0 * Rise * Y / FMath::Square(Plan::RimB));
		}

		/** The cloth's face at (x, y). */
		double Cloth(double X, double Y)
		{
			return Plan::RimZ + Paraboloid(X, Y, Plan::VelariumRise);
		}

		/** The glass's underside at (x, y): its outer face less its thickness along its normal, measured plumb. */
		double GlassUnderside(double X, double Y)
		{
			const FVector2D Slope = SlopeOf(X, Y, Plan::DomeRise);
			return Plan::RingTop + Paraboloid(X, Y, Plan::DomeRise) - Plan::DomeGlassMeters * FMath::Sqrt(1.0 + Slope.SizeSquared());
		}

		/** The plumb height of a section Depth deep square to a path of this slope. */
		double Plumb(double Depth, double Slope)
		{
			return Depth / FMath::Sqrt(1.0 + Slope * Slope);
		}

		/** The plan axis a run lies along. */
		enum class ERunAxis : uint8
		{
			X,
			Y,
		};

		/**
		 * A section run on the vertical face through a line of the plan (y = Fixed along x, or x = Fixed along y), from Low to
		 * High along it, its path at Height and its ends chamfered on the planes whose plan normals are given. The face reaches
		 * past the cove, so every run lies on it; U is the distance from its start.
		 */
		FFormDrawing RunOnFace(const FString& Name, const TCHAR* LocalSurface, FFormProfile Section, ERunAxis Axis, double Fixed, double Low,
			double High, TFunctionRef<double(double)> Height, const FVector2D& LowCut, const FVector2D& HighCut)
		{
			const bool bAlongX = Axis == ERunAxis::X;
			const double Reach = (bAlongX ? Plan::RimA : Plan::RimB) + Plan::CoveReach;
			auto InPlan = [bAlongX, Fixed](double Along) { return bAlongX ? FVector2D(Along, Fixed) : FVector2D(Fixed, Along); };
			const int32 Steps = FMath::Max(1, FMath::CeilToInt32((High - Low) / Plan::RiseStep));
			TArray<FVector2D> Points;
			for (int32 Step = 0; Step <= Steps; ++Step)
			{
				const double Along = FMath::Lerp(Low, High, double(Step) / Steps);
				Points.Add(FVector2D(Along + Reach, Height(Along)));
			}
			FFormSweep Shape;
			Shape.Section = MoveTemp(Section);
			Shape.Face = FFormProfile::Polyline({InPlan(-Reach), InPlan(Reach)}, false);
			Shape.Path = SmoothPath(Points);
			Shape.StartCut = FVector(LowCut.X, LowCut.Y, 0.0);
			Shape.EndCut = FVector(HighCut.X, HighCut.Y, 0.0);
			return Drawn(*Name, LocalSurface, MoveTemp(Shape), nullptr);
		}

		/**
		 * Where a line of the plan (y = Fixed, or x = Fixed) meets the rim line's parallel D inside it, on the line's far (+)
		 * side: the distance along the line, and the rim's outward normal there.
		 */
		double MeetsRim(ERunAxis Axis, double Fixed, double D, FVector2D& OutNormal)
		{
			const double Quarter = UE_DOUBLE_HALF_PI;
			const double T = Axis == ERunAxis::X ? Rim().TWhereY(Fixed, -D, Fixed < 0.0 ? -Quarter : 0.0, Fixed < 0.0 ? 0.0 : Quarter)
				: Rim().TWhereX(Fixed, -D, Fixed < 0.0 ? Quarter : 0.0, Fixed < 0.0 ? UE_DOUBLE_PI : Quarter);
			OutNormal = Rim().Outward(T);
			const FVector2D Point = Rim().Parallel(T, -D);
			return Axis == ERunAxis::X ? Point.X : Point.Y;
		}

		/** The rim's normal at a line's near (−) end: the far end's, mirrored across the line. */
		FVector2D Mirrored(ERunAxis Axis, const FVector2D& Normal)
		{
			return Axis == ERunAxis::X ? FVector2D(-Normal.X, Normal.Y) : FVector2D(Normal.X, -Normal.Y);
		}

		/** A line's name: its side of the axis and its offset in centimetres. */
		FString LineName(double Fixed, const TCHAR* Negative, const TCHAR* Positive)
		{
			return FString::Printf(TEXT("%s%03d"), Fixed < 0.0 ? Negative : Positive, FMath::RoundToInt32(100.0 * FMath::Abs(Fixed)));
		}

		/** A tee's section: its flange on the cloth (A up from the cloth, B across), its web standing over the seam. */
		FFormProfile Tee()
		{
			const double Flange = 0.5 * Plan::RibFlange;
			const double Web = 0.5 * Plan::RibMetal;
			const double Metal = Plan::RibMetal;
			return Polygon({FVector2D(0.0, -Flange), FVector2D(Metal, -Flange), FVector2D(Metal, -Web), FVector2D(Plan::RibDepth, -Web),
				FVector2D(Plan::RibDepth, Web), FVector2D(Metal, Web), FVector2D(Metal, Flange), FVector2D(0.0, Flange)});
		}

		/** A bar's section: its top on the path, hanging below it (A up), BarWidth across. */
		FFormProfile BarSection()
		{
			return FFormProfile::Rectangle(FVector2D(-Plan::BarDepth, -0.5 * Plan::BarWidth), FVector2D(0.0, 0.5 * Plan::BarWidth));
		}

		/** The top of the bar along x at y: the glass's underside, less its fall across half the bar. */
		double XBarTop(double X, double Y)
		{
			return GlassUnderside(X, Y) - 0.5 * Plan::BarWidth * FMath::Abs(SlopeOf(X, Y, Plan::DomeRise).Y);
		}

		/** The rods hanging a tee from the bar above it, every RodSpacing from the axis, clear of the ring. */
		void Rods(TArray<FFormDrawing>& Out, const FString& Rib, double Y, double End, TFunctionRef<double(double)> Bed)
		{
			const int32 Count = FMath::FloorToInt32((End - Plan::RibFlange) / Plan::RodSpacing);
			const double Radius = 0.5 * Plan::RodDiameter;
			for (int32 K = -Count; K <= Count; ++K)
			{
				const double X = K * Plan::RodSpacing;
				const double Bottom = Bed(X) + Plumb(Plan::RibDepth, SlopeOf(X, Y, Plan::VelariumRise).X);
				const double Top = XBarTop(X, Y) - Plumb(Plan::BarDepth, SlopeOf(X, Y, Plan::DomeRise).X);
				FFormLathe Rod;
				Rod.Section = FFormProfile::Polyline({FVector2D(0.0, 0.0), FVector2D(Radius, 0.0), FVector2D(Radius, Top - Bottom),
					FVector2D(0.0, Top - Bottom)}, false);
				Out.Add(Drawn(*FString::Printf(TEXT("%s.rod.%02d"), *Rib, K + Count), TEXT("velarium.frame"), MoveTemp(Rod), nullptr,
					FMuseeFrame(FVector(X, Y, Bottom))));
			}
		}

		/**
		 * The velarium's tees over the seams, lying on the cloth (lifted by the cloth's fall across half the flange, so the
		 * flange's uphill edge rests on it), their ends against the rim ring's flat leg; and the rods each hangs by.
		 */
		void Ribs(TArray<FFormDrawing>& Out)
		{
			for (const double Seam : Plan::SeamYs)
			{
				for (const double Y : {-Seam, Seam})
				{
					const double Lift = 0.5 * Plan::RibFlange * FMath::Abs(SlopeOf(0.0, Y, Plan::VelariumRise).Y);
					auto Bed = [Y, Lift](double X) { return Cloth(X, Y) + Lift; };
					FVector2D Normal;
					const double End = MeetsRim(ERunAxis::X, Y, Plan::RimAngleLeg, Normal);
					const FString Rib = TEXT("velarium.frame.rib.") + LineName(Y, TEXT("n"), TEXT("s"));
					Out.Add(RunOnFace(Rib, TEXT("velarium.frame"), Tee(), ERunAxis::X, Y, -End, End, Bed, Mirrored(ERunAxis::X, Normal), Normal));
					Rods(Out, Rib, Y, End, Bed);
				}
			}
		}

		/** The bars along x over the seams, whole from the curb to the curb. */
		void BarsAlongX(TArray<FFormDrawing>& Out)
		{
			for (const double Seam : Plan::SeamYs)
			{
				for (const double Y : {-Seam, Seam})
				{
					FVector2D Normal;
					const double End = MeetsRim(ERunAxis::X, Y, Plan::BarWidth, Normal);
					auto Top = [Y](double X) { return XBarTop(X, Y); };
					Out.Add(RunOnFace(TEXT("roof.bar.x.") + LineName(Y, TEXT("n"), TEXT("s")), TEXT("roof.bars"), BarSection(), ERunAxis::X, Y, -End, End,
						Top, Mirrored(ERunAxis::X, Normal), Normal));
				}
			}
		}

		/** The seams whose bars along x a bar along y at x crosses whole: they reach past its sides, to their chamfered ends. */
		TArray<double> Crossings(double X)
		{
			TArray<double> Crossed;
			for (const double Seam : Plan::SeamYs)
			{
				for (const double Y : {-Seam, Seam})
				{
					FVector2D Normal;
					const double End = MeetsRim(ERunAxis::X, Y, Plan::BarWidth, Normal);
					// The chamfered end reaches back across half the bar by the tangent of its lean.
					const double CutBack = 0.5 * Plan::BarWidth * FMath::Abs(Normal.Y / Normal.X);
					if (FMath::Abs(X) + 0.5 * Plan::BarWidth < End - CutBack)
					{
						Crossed.Add(Y);
					}
				}
			}
			Crossed.Sort();
			return Crossed;
		}

		/** The bars along y at x = 0, ±1.20 … ±8.40, cut between the bars along x, and at each end against the curb. */
		void BarsAlongY(TArray<FFormDrawing>& Out)
		{
			const FVector2D Side(0.0, 1.0);
			for (int32 K = -Plan::BarsEachSide; K <= Plan::BarsEachSide; ++K)
			{
				const double X = K * Plan::BarSpacing;
				FVector2D Normal;
				const double End = MeetsRim(ERunAxis::Y, X, Plan::BarWidth, Normal);
				auto Top = [X](double Y) { return GlassUnderside(X, Y) - 0.5 * Plan::BarWidth * FMath::Abs(SlopeOf(X, Y, Plan::DomeRise).X); };
				const FString Line = TEXT("roof.bar.y.") + (K == 0 ? FString(TEXT("c000")) : LineName(X, TEXT("w"), TEXT("e")));
				double Low = -End;
				FVector2D LowCut = Mirrored(ERunAxis::Y, Normal);
				int32 Segment = 0;
				for (const double Crossed : Crossings(X))
				{
					Out.Add(RunOnFace(FString::Printf(TEXT("%s.%d"), *Line, Segment++), TEXT("roof.bars"), BarSection(), ERunAxis::Y, X, Low,
						Crossed - 0.5 * Plan::BarWidth, Top, LowCut, Side));
					Low = Crossed + 0.5 * Plan::BarWidth;
					LowCut = Side;
				}
				Out.Add(RunOnFace(FString::Printf(TEXT("%s.%d"), *Line, Segment), TEXT("roof.bars"), BarSection(), ERunAxis::Y, X, Low, End, Top,
					LowCut, Normal));
			}
		}

		/**
		 * The curb the glass's edge and the bars bear on: a bronze bar of the bars' section run round the rim line against
		 * the ring's and the upstand's inner face, its top at the glass's underside where the glass falls steepest round the
		 * rim (across the short axis), so the edge bears on it all round.
		 */
		void Curb(TArray<FFormDrawing>& Out)
		{
			const double Steepest = 2.0 * Plan::DomeRise / Plan::RimB;
			FFormSweep Shape;
			Shape.Section = FFormProfile::Rectangle(FVector2D(0.0, -Plan::BarDepth), FVector2D(Plan::BarWidth, 0.0));
			Shape.Path = FFormProfile::Ellipse(FVector2D::ZeroVector, Plan::RimA, Plan::RimB, 180.0, true);
			Shape.ElevationMeters = Plan::RingTop - Plan::DomeGlassMeters * FMath::Sqrt(1.0 + Steepest * Steepest);
			Out.Add(Drawn(TEXT("roof.curb"), TEXT("roof.bars"), MoveTemp(Shape), nullptr));
		}
	}

	void DrawRoofFrame(TArray<FFormDrawing>& Out)
	{
		Ribs(Out);
		BarsAlongX(Out);
		BarsAlongY(Out);
		Curb(Out);
	}
}
