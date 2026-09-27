#include "HallOfLightEnds.h"
#include "HallOfLightParts.h"
#include "HallOfLightPlan.h"

/**
 * The vault (sec. 3.2, 3.3): a shallow barrel of pearl steel and glass, R 5.30 about (y 0, z 2.20), bearing on the eaves
 * at +-56.83 deg: eighteen arches, seven purlin lines, a cross of flat bars in every panel between arches, a collar on
 * each lining, and one hot-bent pane per panel, 8 across and 19 along. Every member is drawn by rule about the vault's
 * axis at R 5.30 + b.
 *
 * Form draws no sweep along a path out of plan (KIT-BACKLOG), so the crosses are straight bars node to node and each
 * collar is a chain of straight flats between the purlins' nodes on its lining; and it can't cut a turned pane or a
 * run to a curved face, so the end bays' panes and the purlins' end lengths stop square where they clear the collar
 * and the lining. The sheet's first pass lists all of these as Form's gaps.
 */
namespace Musee::HallOfLight
{
	namespace
	{
		/** A bisection this fine (a micrometre) finds where a bar's end clears what it stops against. */
		constexpr double SetbackToleranceMeters = 1.0e-6;
		/** A collar's flat is checked against its lining at this many points along it. */
		constexpr int32 CollarSamples = 32;

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

		/**
		 * A straight bar of rectangular section between two points of its centreline: across it HalfAcross either side, and
		 * up it (towards the vault's outside, square to its length in the plane of the vault's radius at its middle)
		 * HalfUp either side.
		 */
		struct FBar
		{
			FVector From;
			FVector To;
			FVector Along;
			FVector Up;
			FVector Across;
			double Length = 0.0;
			double HalfAcross = 0.0;
			double HalfUp = 0.0;

			FBar(const FVector& InFrom, const FVector& InTo, double MidTheta, double InHalfAcross, double InHalfUp)
				: From(InFrom), To(InTo), HalfAcross(InHalfAcross), HalfUp(InHalfUp)
			{
				Length = FVector::Distance(From, To);
				Along = (To - From) / Length;
				const FVector Radial(0.0, FMath::Sin(MidTheta), FMath::Cos(MidTheta));
				Up = (Radial - FVector::DotProduct(Radial, Along) * Along).GetSafeNormal();
				Across = FVector::CrossProduct(Up, Along);
			}

			/** The four corners of the bar's end face a distance along it from its start. */
			TArray<FVector> Corners(double Distance) const
			{
				TArray<FVector> Out;
				for (const double A : {-HalfAcross, HalfAcross})
				{
					for (const double B : {-HalfUp, HalfUp})
					{
						Out.Add(From + Along * Distance + Across * A + Up * B);
					}
				}
				return Out;
			}

			/** The bar from Start along it to Length less End, as a drawing. */
			FFormDrawing Drawing(const FString& Name, FName InSurface, double Start, double End) const
			{
				FFormExtrusion Shape;
				Shape.Outline = FFormProfile::Rectangle(FVector2D(-HalfAcross, -HalfUp), FVector2D(HalfAcross, HalfUp));
				Shape.DepthMeters = Length - Start - End;
				return Drawn(Name, InSurface, MoveTemp(Shape), nullptr, FrameOfAxes(From + Along * Start, Across, Up));
			}

			/** The same bar the other way round. */
			FBar Reversed(double MidTheta) const
			{
				return FBar(To, From, MidTheta, HalfAcross, HalfUp);
			}
		};

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

		/** A point's vault angle and its distance round the vault from a line at angle Theta. */
		double RoundFrom(const FVector& Point, double Theta)
		{
			const double Angle = FMath::Atan2(Point.Y, Point.Z - Plan::VaultAxisZ);
			const double Radius = FVector2D(Point.Y, Point.Z - Plan::VaultAxisZ).Size();
			return FMath::Abs(Angle - Theta) * Radius;
		}

		/**
		 * Where a bar's end stops short of a node, at least the sheet's 0.025: clear of the arch's web, and of the purlin
		 * or, on the eaves' line, above the eaves' top.
		 */
		double ShortOfNode(const FBar& Bar, double NodeX, double NodeTheta, bool bEaves)
		{
			auto Clear = [&](double Distance)
			{
				for (const FVector& Corner : Bar.Corners(Distance))
				{
					const bool bOffWeb = FMath::Abs(Corner.X - NodeX) >= Plan::ArchWeb.Half;
					const bool bOffLine = bEaves ? Corner.Z >= Plan::EavesTop : RoundFrom(Corner, NodeTheta) >= Plan::PurlinHalf;
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
		double ShortOfCrossing(const FBar& Half, const FBar& Through)
		{
			const FVector Middle = 0.5 * (Through.From + Through.To);
			auto Clear = [&](double Distance)
			{
				for (const FVector& Corner : Half.Corners(Half.Length - Distance))
				{
					if (FMath::Abs(FVector::DotProduct(Corner - Middle, Through.Across)) < Through.HalfAcross)
					{
						return false;
					}
				}
				return true;
			};
			return Setback(Clear, 0.0, 0.5 * Half.Length);
		}

		/** The centreline point of a cross's bar at a node: on the vault at the bar's middle depth. */
		FVector CrossNode(double X, double Theta, const Plan::FPlate& Bar)
		{
			return VaultPoint(X, Theta, Plan::VaultRadius + 0.5 * (Bar.FromB + Bar.ToB));
		}

		/** A cross in one panel: bar A from the lower-left node to the upper-right, bar B the other way, cut at A. */
		void Cross(TArray<FFormDrawing>& Out, int32 Bay, int32 Panel, const TArray<double>& Lines)
		{
			const double X0 = ArchX(Bay);
			const double X1 = ArchX(Bay + 1);
			const double T0 = Lines[Panel];
			const double T1 = Lines[Panel + 1];
			const double Mid = 0.5 * (T0 + T1);
			const bool bEaves0 = Panel == 0;
			const bool bEaves1 = Panel + 2 == Lines.Num();
			const double UpA = 0.5 * (Plan::BarA.ToB - Plan::BarA.FromB);
			const double UpB = 0.5 * (Plan::BarB.ToB - Plan::BarB.FromB);
			const FBar A(CrossNode(X0, T0, Plan::BarA), CrossNode(X1, T1, Plan::BarA), Mid, Plan::BarA.Half, UpA);
			const FBar B(CrossNode(X0, T1, Plan::BarB), CrossNode(X1, T0, Plan::BarB), Mid, Plan::BarB.Half, UpB);
			const FString Name = FString::Printf(TEXT("cross.%d.%d"), Bay, Panel);
			const double AStart = ShortOfNode(A, X0, T0, bEaves0);
			const double AEnd = ShortOfNode(A.Reversed(Mid), X1, T1, bEaves1);
			Out.Add(A.Drawing(Name + TEXT(".a"), VaultSteel(), AStart, AEnd));

			// B in two halves, each from its node to A's side.
			const FVector Crossing = 0.5 * (B.From + B.To);
			const FBar First(B.From, Crossing, Mid, Plan::BarB.Half, UpB);
			const FBar Second(B.To, Crossing, Mid, Plan::BarB.Half, UpB);
			Out.Add(First.Drawing(Name + TEXT(".b-1"), VaultSteel(), ShortOfNode(First, X0, T1, bEaves1), ShortOfCrossing(First, A)));
			Out.Add(Second.Drawing(Name + TEXT(".b-2"), VaultSteel(), ShortOfNode(Second, X1, T0, bEaves0), ShortOfCrossing(Second, A)));
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

		/** A collar's flat between two lines on a lining, set in from the lining so no part of it runs into the stone. */
		FBar CollarFlat(EHallEnd End, double T0, double T1)
		{
			const FEndFace Face = InnerFace(End);
			const double Middle = Plan::VaultRadius + 0.5 * (Plan::CollarFromB + Plan::CollarToB);
			auto Node = [&](double Theta, double In)
			{
				const FVector Point = VaultPoint(0.0, Theta, Middle);
				return FVector(Face.X(Point.Y) + Face.Inward() * (0.5 * Plan::CollarAlong + In), Point.Y, Point.Z);
			};
			const double HalfUp = 0.5 * (Plan::CollarToB - Plan::CollarFromB);
			const FBar Flat(Node(T0, 0.0), Node(T1, 0.0), 0.5 * (T0 + T1), 0.5 * Plan::CollarAlong, HalfUp);
			double Into = 0.0;
			for (int32 Sample = 0; Sample <= CollarSamples; ++Sample)
			{
				for (const FVector& Corner : Flat.Corners(Flat.Length * Sample / CollarSamples))
				{
					Into = FMath::Max(Into, Face.Inward() * (Face.X(Corner.Y) - Corner.X));
				}
			}
			return FBar(Node(T0, Into), Node(T1, Into), 0.5 * (T0 + T1), 0.5 * Plan::CollarAlong, HalfUp);
		}

		/**
		 * Where a collar's flat stops at a node: short of the plane that halves the angle with the next flat, so the two
		 * meet at their undersides; at the eaves, above the eaves' top.
		 */
		double CollarEnd(const FBar& Flat, const FBar* Next)
		{
			auto Clear = [&](double Distance)
			{
				for (const FVector& Corner : Flat.Corners(Flat.Length - Distance))
				{
					const bool bInside = Next ? FVector::DotProduct(Corner - Flat.To, (Flat.Along + Next->Along).GetSafeNormal()) <= 0.0
											  : Corner.Z >= Plan::EavesTop;
					if (!bInside)
					{
						return false;
					}
				}
				return true;
			};
			return Setback(Clear, 0.0, 0.5 * Flat.Length);
		}

		/** The collars, fixed to each lining under the vault glass's end, and how far each flat stands in from its lining. */
		void Collars(TArray<FFormDrawing>& Out)
		{
			const TArray<double> Lines = VaultLines();
			for (const EHallEnd End : {EHallEnd::West, EHallEnd::East})
			{
				TArray<FBar> Flats;
				for (int32 Panel = 0; Panel + 1 < Lines.Num(); ++Panel)
				{
					Flats.Add(CollarFlat(End, Lines[Panel], Lines[Panel + 1]));
				}
				for (int32 Panel = 0; Panel < Flats.Num(); ++Panel)
				{
					const FBar& Flat = Flats[Panel];
					const FBar Back = Flat.Reversed(0.5 * (Lines[Panel] + Lines[Panel + 1]));
					const FBar* Previous = Panel > 0 ? &Flats[Panel - 1] : nullptr;
					const FBar PreviousBack = Previous ? Previous->Reversed(0.5 * (Lines[Panel - 1] + Lines[Panel])) : Back;
					const double Start = CollarEnd(Back, Previous ? &PreviousBack : nullptr);
					const double Stop = CollarEnd(Flat, Panel + 1 < Flats.Num() ? &Flats[Panel + 1] : nullptr);
					const FString Name = FString::Printf(TEXT("collar.%s.%d"), End == EHallEnd::West ? TEXT("west") : TEXT("east"), Panel);
					Out.Add(Flat.Drawing(Name, VaultSteel(), Start, Stop));
				}
			}
		}

		/** How far a collar stands in from its lining at a purlin's line: the more of its two flats there. */
		double CollarInAt(EHallEnd End, int32 Line, const TArray<double>& Lines)
		{
			const FEndFace Face = InnerFace(End);
			double In = 0.0;
			for (const int32 Panel : {Line - 1, Line})
			{
				const FBar Flat = CollarFlat(End, Lines[Panel], Lines[Panel + 1]);
				const FVector At = Panel == Line ? Flat.From : Flat.To;
				In = FMath::Max(In, Face.Inward() * (At.X - Face.X(At.Y)) - 0.5 * Plan::CollarAlong);
			}
			return In;
		}

		/**
		 * The purlins, in lengths between the arch webs and from each collar to the first or last arch. A length at a
		 * collar stops square where it clears the collar's inner face over its section.
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
				double Y0 = TNumericLimits<double>::Max();
				double Y1 = TNumericLimits<double>::Lowest();
				for (const double A : {-Plan::PurlinHalf, Plan::PurlinHalf})
				{
					for (const double B : {Inner, Outer})
					{
						const double Y = Across.Y * A + Radial.Y * B;
						Y0 = FMath::Min(Y0, Y);
						Y1 = FMath::Max(Y1, Y);
					}
				}
				const FRunLimit WestCollar = FRunLimit::OnFace(InnerFace(EHallEnd::West, Plan::CollarAlong + CollarInAt(EHallEnd::West, Line, Lines)));
				const FRunLimit EastCollar = FRunLimit::OnFace(InnerFace(EHallEnd::East, Plan::CollarAlong + CollarInAt(EHallEnd::East, Line, Lines)));
				for (int32 Length = 0; Length <= Plan::Arches; ++Length)
				{
					const double X0 = Length == 0 ? WestCollar.Innermost(Y0, Y1, 1.0) : ArchX(Length - 1) + Plan::ArchWeb.Half;
					const double X1 = Length == Plan::Arches ? EastCollar.Innermost(Y0, Y1, -1.0) : ArchX(Length) - Plan::ArchWeb.Half;
					FFormExtrusion Shape;
					Shape.Outline = FFormProfile::Rectangle(FVector2D(-Plan::PurlinHalf, Inner), FVector2D(Plan::PurlinHalf, Outer));
					Shape.DepthMeters = X1 - X0;
					Out.Add(Drawn(FString::Printf(TEXT("purlin.%d.%d"), Line, Length), VaultSteel(), MoveTemp(Shape), nullptr,
						FrameOfAxes(FVector(X0, 0.0, Plan::VaultAxisZ), Across, Radial)));
				}
			}
		}

		/**
		 * The vault's panes, one per panel, hot-bent to R 5.30 and centred on it: gasketed against the arch webs and the
		 * purlins either side, standing on the eaves' top at their feet; in the end bays from the lining (where the pane
		 * first touches it) to the first or last arch.
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
				const double Y0 = FMath::Min(Inner * FMath::Sin(From), Outer * FMath::Sin(From));
				const double Y1 = FMath::Max(Inner * FMath::Sin(To), Outer * FMath::Sin(To));
				for (int32 Bay = 0; Bay < Bays; ++Bay)
				{
					const double X0 = Bay == 0 ? FRunLimit::OnFace(InnerFace(EHallEnd::West)).Innermost(Y0, Y1, 1.0) : ArchX(Bay - 1) + AtWeb;
					const double X1 = Bay + 1 == Bays ? FRunLimit::OnFace(InnerFace(EHallEnd::East)).Innermost(Y0, Y1, -1.0) : ArchX(Bay) - AtWeb;
					FFormLathe Shape;
					Shape.Section = FFormProfile::Rectangle(FVector2D(Inner, X0), FVector2D(Outer, X1));
					// The axis's frame turns from the crown towards the north: a vault angle is its negative.
					Shape.FromDegrees = -FMath::RadiansToDegrees(To);
					Shape.ToDegrees = -FMath::RadiansToDegrees(From);
					FFormDrawing Pane = Drawn(FString::Printf(TEXT("vault.glass.%d.%d"), Bay, Panel), Surface(TEXT("vault.glass")), MoveTemp(Shape), nullptr,
						VaultAxis());
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
