#include "HallOfLightEnds.h"
#include "HallOfLightParts.h"
#include "HallOfLightPlan.h"

/**
 * The vault (sec. 3.2, 3.3): a shallow barrel of pearl steel and glass, R 5.30 about (y 0, z 2.20), bearing on the eaves
 * at +-56.83 deg: eighteen arches, seven purlin lines, a cross of flat bars in every panel between arches, a collar on
 * each lining, and one hot-bent pane per panel, 8 across and 19 along. Every member is drawn by rule about the vault's
 * axis at R 5.30 + b.
 *
 * What lies on the barrel is drawn in its development (Form's ground of revolution about the vault's axis): U round it
 * from the crown towards the north, V along the hall, W out of it. A cross's flat bars run straight from node to node in
 * the development, so on the vault they are the helices a flat bar bent over its panel makes; each collar is one flat bent
 * round its lining's curve under the glass's end; and the end bays' panes and the purlins' end lengths are cut to the
 * lining and the collar along those curves.
 */
namespace Musee::HallOfLight
{
	namespace
	{
		/** A bisection this fine (a micrometre) finds where a bar's end clears what it stops against. */
		constexpr double SetbackToleranceMeters = 1.0e-6;

		FName VaultSteel() { return Surface(TEXT("vault.steel")); }

		/**
		 * One plate of an arch: an annulus about the vault's axis between two radii, its feet cut level on the eaves'
		 * top, pushed along the hall to the plate's width.
		 */
		FFormProfile ArchPlate(const Plan::FPlate& Plate)
		{
			const FVector2D Axis(0.0, Plan::VaultAxisZ);
			const double Foot = Plan::EavesTop - Plan::VaultAxisZ;
			const double Inner = Plan::VaultRadius + Plate.FromB;
			const double Outer = Plan::VaultRadius + Plate.ToB;
			const double InnerFoot = FMath::RadiansToDegrees(FMath::Acos(Foot / Inner));
			const double OuterFoot = FMath::RadiansToDegrees(FMath::Acos(Foot / Outer));
			// In the plate's plane (y, z) an angle from +y towards +z is 90 deg less the vault angle from the crown.
			FFormProfile Outline = FFormProfile::From(FVector2D(Outer * FMath::Sin(FMath::DegreesToRadians(OuterFoot)), Plan::EavesTop));
			Outline.ArcAbout(Axis, Outer, Outer, 90.0 - OuterFoot, 90.0 + OuterFoot, EFormJoint::Crisp);
			Outline.LineTo(FVector2D(-Inner * FMath::Sin(FMath::DegreesToRadians(InnerFoot)), Plan::EavesTop), EFormJoint::Crisp);
			Outline.ArcAbout(Axis, Inner, Inner, 90.0 + InnerFoot, 90.0 - InnerFoot, EFormJoint::Crisp);
			return Outline.Close();
		}

		void Arches(TArray<FFormDrawing>& Out)
		{
			const TPair<const TCHAR*, Plan::FPlate> Plates[] = {{TEXT("flange"), Plan::ArchFlange}, {TEXT("web"), Plan::ArchWeb}, {TEXT("cap"), Plan::ArchCap}};
			for (int32 Arch = 0; Arch < Plan::Arches; ++Arch)
			{
				for (const TPair<const TCHAR*, Plan::FPlate>& Plate : Plates)
				{
					FFormExtrusion Shape;
					Shape.Outline = ArchPlate(Plate.Value);
					Shape.DepthMeters = 2.0 * Plate.Value.Half;
					Out.Add(Drawn(FString::Printf(TEXT("arch.%d.%s"), Arch, Plate.Key), VaultSteel(), MoveTemp(Shape), nullptr,
						AlongHall(ArchX(Arch) - Plate.Value.Half)));
				}
			}
		}

		/** The vault's cylinder at a radius as a ground, in the vault axis's frame (x up, y north, z east along the hall). */
		FFormGround Barrel(double Radius)
		{
			return FFormGround::Revolved(FFormProfile::Polyline({FVector2D(Radius, 0.0), FVector2D(Radius, 1.0)}, false), Radius);
		}

		/** A point of the vault, its angle (radians from the crown, positive south) and x, in the barrel's development at a radius. */
		FVector2D OnBarrel(double Theta, double X, double Radius)
		{
			return FVector2D(-Radius * Theta, X);
		}

		/** A shape drawn in the barrel's development at a radius, stood on the vault's axis. */
		template <typename TShape>
		FFormDrawing DrawnOnBarrel(const FString& Name, FName InSurface, TShape&& Shape, double Radius)
		{
			FFormDrawing Drawing = Drawn(Name, InSurface, Forward<TShape>(Shape), nullptr, VaultAxis());
			Drawing.Ground = Barrel(Radius);
			return Drawing;
		}

		/** The least distance from Least to Most at which Clear holds, Clear holding from some distance on. */
		double Setback(TFunctionRef<bool(double)> Clear, double Least, double Most)
		{
			if (Clear(Least))
			{
				return Least;
			}
			double Low = Least;
			double High = Most;
			while (High - Low > SetbackToleranceMeters)
			{
				const double Mid = 0.5 * (Low + High);
				(Clear(Mid) ? High : Low) = Mid;
			}
			return High;
		}

		/**
		 * A flat bar of a cross, straight in the barrel's development between two nodes: its line, the half of its width
		 * across the line, the radius its inner face lies on (which its development is drawn at) and its depth out from it.
		 */
		struct FBarrelBar
		{
			FVector2D From;
			FVector2D To;
			FVector2D Along;
			FVector2D Across;
			double Length = 0.0;
			double Half = 0.0;
			double Radius = 0.0;
			double Depth = 0.0;

			FBarrelBar(const FVector2D& InFrom, const FVector2D& InTo, double InHalf, double InRadius, double InDepth)
				: From(InFrom), To(InTo), Half(InHalf), Radius(InRadius), Depth(InDepth)
			{
				Length = FVector2D::Distance(From, To);
				Along = (To - From) / Length;
				Across = FVector2D(-Along.Y, Along.X);
			}

			FVector2D At(double Distance) const { return From + Along * Distance; }

			/** The two edges of its width a distance along it. */
			TArray<FVector2D> Edges(double Distance) const { return {At(Distance) - Across * Half, At(Distance) + Across * Half}; }

			/** The bar from Start along it to Length less End, swept on the barrel. */
			FFormDrawing Drawing(const FString& Name, double Start, double End) const
			{
				FFormSweep Shape;
				Shape.Section = FFormProfile::Rectangle(FVector2D(-Half, 0.0), FVector2D(Half, Depth));
				Shape.Path = FFormProfile::Polyline({At(Start), At(Length - End)}, false);
				return DrawnOnBarrel(Name, VaultSteel(), MoveTemp(Shape), Radius);
			}

			FBarrelBar Reversed() const { return FBarrelBar(To, From, Half, Radius, Depth); }
		};

		/**
		 * Where a bar's end stops short of its node, at least the sheet's 0.025: clear of the arch's web, and of the purlin's
		 * side or, on the eaves' line, above the eaves' top.
		 */
		double ShortOfNode(const FBarrelBar& Bar, const FVector2D& Node, bool bEaves)
		{
			auto Clear = [&](double Distance)
			{
				for (const FVector2D& Edge : Bar.Edges(Distance))
				{
					const bool bOffWeb = FMath::Abs(Edge.Y - Node.Y) >= Plan::ArchWeb.Half;
					bool bOffLine = FMath::Abs(Edge.X - Node.X) >= Plan::PurlinHalf;
					if (bEaves)
					{
						const double Theta = -Edge.X / Bar.Radius;
						bOffLine = Plan::VaultAxisZ + Bar.Radius * FMath::Cos(Theta) >= Plan::EavesTop;
					}
					if (!bOffWeb || !bOffLine)
					{
						return false;
					}
				}
				return true;
			};
			return Setback(Clear, Plan::BarShortOfNode, 0.5 * Bar.Length);
		}

		/** Where half of bar B stops short of the crossing, clear of bar A's side, which runs through. */
		double ShortOfCrossing(const FBarrelBar& Half, const FBarrelBar& Through)
		{
			const FVector2D Middle = Through.At(0.5 * Through.Length);
			auto Clear = [&](double Distance)
			{
				for (const FVector2D& Edge : Half.Edges(Half.Length - Distance))
				{
					if (FMath::Abs(FVector2D::DotProduct(Edge - Middle, Through.Across)) < Through.Half)
					{
						return false;
					}
				}
				return true;
			};
			return Setback(Clear, 0.0, 0.5 * Half.Length);
		}

		/** A cross in one panel: bar A from the lower-left node to the upper-right, bar B the other way, cut at A. */
		void Cross(TArray<FFormDrawing>& Out, int32 Bay, int32 Panel, const TArray<double>& Lines)
		{
			const double X0 = ArchX(Bay);
			const double X1 = ArchX(Bay + 1);
			const double T0 = Lines[Panel];
			const double T1 = Lines[Panel + 1];
			const bool bEaves0 = Panel == 0;
			const bool bEaves1 = Panel + 2 == Lines.Num();
			const double RA = Plan::VaultRadius + Plan::BarA.FromB;
			const double RB = Plan::VaultRadius + Plan::BarB.FromB;
			const FBarrelBar A(OnBarrel(T0, X0, RA), OnBarrel(T1, X1, RA), Plan::BarA.Half, RA, Plan::BarA.ToB - Plan::BarA.FromB);
			const FBarrelBar B(OnBarrel(T1, X0, RB), OnBarrel(T0, X1, RB), Plan::BarB.Half, RB, Plan::BarB.ToB - Plan::BarB.FromB);
			const FString Name = FString::Printf(TEXT("cross.%d.%d"), Bay, Panel);
			Out.Add(A.Drawing(Name + TEXT(".a"), ShortOfNode(A, A.From, bEaves0), ShortOfNode(A.Reversed(), A.To, bEaves1)));

			// B in two halves, each from its node to A's side; A's line drawn at B's radius, where B meets it.
			const FBarrelBar Through(OnBarrel(T0, X0, RB), OnBarrel(T1, X1, RB), Plan::BarA.Half, RB, 0.0);
			const FVector2D Crossing = B.At(0.5 * B.Length);
			const FBarrelBar First(B.From, Crossing, Plan::BarB.Half, RB, B.Depth);
			const FBarrelBar Second(B.To, Crossing, Plan::BarB.Half, RB, B.Depth);
			Out.Add(First.Drawing(Name + TEXT(".b-1"), ShortOfNode(First, First.From, bEaves1), ShortOfCrossing(First, Through)));
			Out.Add(Second.Drawing(Name + TEXT(".b-2"), ShortOfNode(Second, Second.From, bEaves0), ShortOfCrossing(Second, Through)));
		}

		void Crosses(TArray<FFormDrawing>& Out)
		{
			const TArray<double> Lines = VaultLines();
			for (int32 Bay = 0; Bay + 1 < Plan::Arches; ++Bay)
			{
				for (int32 Panel = 0; Panel + 1 < Lines.Num(); ++Panel)
				{
					Cross(Out, Bay, Panel, Lines);
				}
			}
		}

		/**
		 * The collar's line at an end at a vault angle, along the hall: its middle, or its side towards the hall, taken at the
		 * collar's middle radius, its side on the lining's inner face. Its width is measured square to its own line in the
		 * development, which leans with the lining's curve.
		 */
		double CollarX(EHallEnd End, double Theta, bool bHallSide)
		{
			const FEndFace Face = InnerFace(End);
			const double Middle = Plan::VaultRadius + 0.5 * (Plan::CollarFromB + Plan::CollarToB);
			const double Y = Middle * FMath::Sin(Theta);
			// The face's run along the hall per metre round the vault: its slope in plan times how fast y turns round.
			const double Slope = FMath::Abs(Y) / FMath::Max(FMath::Sqrt(FMath::Max(0.0, Face.Radius * Face.Radius - Y * Y)), UE_DOUBLE_SMALL_NUMBER)
				* FMath::Abs(FMath::Cos(Theta));
			const double Width = (bHallSide ? Plan::CollarAlong : 0.5 * Plan::CollarAlong) * FMath::Sqrt(1.0 + Slope * Slope);
			return Face.X(Y) + Face.Inward() * Width;
		}

		/** The angles a curve round the barrel is followed at: from one angle to another at the linings' step, both included. */
		TArray<double> RoundBarrel(double From, double To, double Radius)
		{
			const int32 Steps = FMath::Max(1, FMath::CeilToInt32(FMath::Abs(To - From) * Radius / Plan::LiningTopStep));
			TArray<double> Angles;
			for (int32 Step = 0; Step <= Steps; ++Step)
			{
				Angles.Add(FMath::Lerp(From, To, double(Step) / Steps));
			}
			return Angles;
		}

		/**
		 * The collars, one flat bent round each lining under the vault glass's end, its side on the lining's face, from eaves
		 * to eaves where its inner face stands on the eaves' top.
		 */
		void Collars(TArray<FFormDrawing>& Out)
		{
			const double Radius = Plan::VaultRadius + Plan::CollarFromB;
			const double Foot = FMath::Acos((Plan::EavesTop - Plan::VaultAxisZ) / Radius);
			for (const EHallEnd End : {EHallEnd::West, EHallEnd::East})
			{
				TArray<FVector2D> Line;
				for (const double Theta : RoundBarrel(-Foot, Foot, Radius))
				{
					Line.Add(OnBarrel(Theta, CollarX(End, Theta, false), Radius));
				}
				FFormSweep Shape;
				Shape.Section = FFormProfile::Rectangle(FVector2D(-0.5 * Plan::CollarAlong, 0.0), FVector2D(0.5 * Plan::CollarAlong, Plan::CollarToB - Plan::CollarFromB));
				Shape.Path = FFormProfile::Polyline(Line, false);
				Out.Add(DrawnOnBarrel(End == EHallEnd::West ? TEXT("collar.west") : TEXT("collar.east"), VaultSteel(), MoveTemp(Shape), Radius));
			}
		}

		/**
		 * The purlins, in lengths between the arch webs; at each end a length from the collar's side towards the hall, cut to
		 * it along its curve, to the first or last arch.
		 */
		void Purlins(TArray<FFormDrawing>& Out)
		{
			const TArray<double> Lines = VaultLines();
			const double Inner = Plan::VaultRadius + Plan::PurlinFromB;
			const double Outer = Plan::VaultRadius + Plan::PurlinToB;
			for (int32 Line = 1; Line + 1 < Lines.Num(); ++Line)
			{
				const double Theta = Lines[Line];
				const FVector Across(0.0, FMath::Cos(Theta), -FMath::Sin(Theta));
				const FVector Radial(0.0, FMath::Sin(Theta), FMath::Cos(Theta));
				for (int32 Length = 1; Length < Plan::Arches; ++Length)
				{
					const double X0 = ArchX(Length - 1) + Plan::ArchWeb.Half;
					const double X1 = ArchX(Length) - Plan::ArchWeb.Half;
					FFormExtrusion Shape;
					Shape.Outline = FFormProfile::Rectangle(FVector2D(-Plan::PurlinHalf, Inner), FVector2D(Plan::PurlinHalf, Outer));
					Shape.DepthMeters = X1 - X0;
					Out.Add(Drawn(FString::Printf(TEXT("purlin.%d.%d"), Line, Length), VaultSteel(), MoveTemp(Shape), nullptr,
						FrameOfAxes(FVector(X0, 0.0, Plan::VaultAxisZ), Across, Radial)));
				}
				const double Side = Plan::PurlinHalf / Inner;
				for (const EHallEnd End : {EHallEnd::West, EHallEnd::East})
				{
					const bool bWest = End == EHallEnd::West;
					const double Arch = bWest ? ArchX(0) - Plan::ArchWeb.Half : ArchX(Plan::Arches - 1) + Plan::ArchWeb.Half;
					const double A = Theta - Side;
					const double B = Theta + Side;
					FFormExtrusion Shape;
					Shape.Outline = FFormProfile::Polyline({OnBarrel(A, CollarX(End, A, true), Inner), OnBarrel(B, CollarX(End, B, true), Inner),
						OnBarrel(B, Arch, Inner), OnBarrel(A, Arch, Inner)}, true);
					Shape.DepthMeters = Outer - Inner;
					Out.Add(DrawnOnBarrel(FString::Printf(TEXT("purlin.%d.%d"), Line, bWest ? 0 : Plan::Arches), VaultSteel(), MoveTemp(Shape), Inner));
				}
			}
		}

		/**
		 * The vault's panes, one per panel, hot-bent to R 5.30 and centred on it: gasketed against the arch webs and the
		 * purlins either side, standing on the eaves' top at their feet; in the end bays from the lining's face, where the
		 * glass's end rests on the collar, along its curve to the first or last arch.
		 */
		void Panes(TArray<FFormDrawing>& Out)
		{
			const TArray<double> Lines = VaultLines();
			const double Half = 0.5 * Plan::GlassMeters;
			const double Inner = Plan::VaultRadius - Half;
			const double Outer = Plan::VaultRadius + Half;
			const double Foot = FMath::Acos((Plan::EavesTop - Plan::VaultAxisZ) / Inner);
			const double AtPurlin = (Plan::PurlinHalf + Plan::GasketMeters) / Plan::VaultRadius;
			const double AtWeb = Plan::ArchWeb.Half + Plan::GasketMeters;
			const int32 Bays = Plan::Arches + 1;
			for (int32 Panel = 0; Panel + 1 < Lines.Num(); ++Panel)
			{
				const double From = Panel == 0 ? -Foot : Lines[Panel] + AtPurlin;
				const double To = Panel + 2 == Lines.Num() ? Foot : Lines[Panel + 1] - AtPurlin;
				for (int32 Bay = 0; Bay < Bays; ++Bay)
				{
					const FString Name = FString::Printf(TEXT("vault.glass.%d.%d"), Bay, Panel);
					FFormDrawing Pane;
					if (Bay == 0 || Bay + 1 == Bays)
					{
						const bool bWest = Bay == 0;
						const FEndFace Face = InnerFace(bWest ? EHallEnd::West : EHallEnd::East);
						const double Arch = bWest ? ArchX(0) - AtWeb : ArchX(Plan::Arches - 1) + AtWeb;
						TArray<FVector2D> Outline;
						for (const double Theta : RoundBarrel(From, To, Inner))
						{
							Outline.Add(OnBarrel(Theta, Face.X(Plan::VaultRadius * FMath::Sin(Theta)), Inner));
						}
						Outline.Add(OnBarrel(To, Arch, Inner));
						Outline.Add(OnBarrel(From, Arch, Inner));
						FFormExtrusion Shape;
						Shape.Outline = FFormProfile::Polyline(Outline, true);
						Shape.DepthMeters = Plan::GlassMeters;
						Pane = DrawnOnBarrel(Name, Surface(TEXT("vault.glass")), MoveTemp(Shape), Inner);
					}
					else
					{
						FFormLathe Shape;
						Shape.Section = FFormProfile::Rectangle(FVector2D(Inner, ArchX(Bay - 1) + AtWeb), FVector2D(Outer, ArchX(Bay) - AtWeb));
						// The axis's frame turns from the crown towards the north: a vault angle is its negative.
						Shape.FromDegrees = -FMath::RadiansToDegrees(To);
						Shape.ToDegrees = -FMath::RadiansToDegrees(From);
						Pane = Drawn(Name, Surface(TEXT("vault.glass")), MoveTemp(Shape), nullptr, VaultAxis());
					}
					Pane.bNanite = false;
					Out.Add(MoveTemp(Pane));
				}
			}
		}
	}

	void DrawVault(TArray<FFormDrawing>& Out)
	{
		Arches(Out);
		Purlins(Out);
		Crosses(Out);
		Collars(Out);
		Panes(Out);
	}
}
