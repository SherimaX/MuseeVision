#include "RotundaParts.h"
#include "RotundaPlan.h"
#include "RotundaStairLine.h"

/**
 * The stair's bronze (§3.9), after Momo's: round the well a balustrade side-fixed to the inner string in the plane r 2.15,
 * its heights standing plumb over the string's top: three bars and a soft handrail over the whole run, square standards at
 * every kink and at most 1.60 apart, and between them two registers, slender balusters below and a chain of rings above,
 * each ring touching the bars that frame it, a boss on four spokes at its heart. A newel with its ball stands at the foot.
 *
 * A run is drawn in its plane: round the well, the development of the cylinder r 2.15 about the stage's axis
 * (RotundaStairLine), where the string's top is a line of chords, a bar is a sweep along it square to its rake and a ring
 * on the rake is a true circle; carried onto the cylinder the bars wind as helices and the rings follow the plane. Behind
 * the top landing the plane is flat, standing on the radial line of its back edge.
 *
 * Under the lowered dial, along the upper flight to tread 8's nosing and across the top landing's back edge, the balustrade
 * retracts: there it is a free panel (standards, balusters, rings, bars, handrail, and a carrier bar at its foot) riding
 * the body rotunda.stair.balustrade.panels, drawn at home 0.94 below its place, wholly inside a hollow housing hung on the
 * stair: marble sides between a bronze cap round the slot and a bronze shoe, a bronze floor in the slot, bronze end blocks.
 * Along the stair's wall a round rail on brackets runs from tread 7's nosing, returned level to the wall on a rose, to 4°
 * past the last riser. The housings' sunk fields are polish.
 */
namespace Musee::Rotunda
{
	namespace
	{
		/** The rings' register is between the middle and top bars, the balusters' between the bottom and middle ones. */
		constexpr int32 BottomBar = 0;
		constexpr int32 MiddleBar = 1;
		constexpr int32 TopBar = 2;

		/**
		 * The plane a run stands in, drawn in its own (S along it, Z up), W out of it: round the well, the cylinder r 2.15
		 * (S metres round it from φ 0 the way φ rises, W away from the axis); behind the top landing, the flat plane standing
		 * Behind its back edge (S from the axis along the edge's radial line, W back towards the landing).
		 */
		struct FRunPlane
		{
			bool bRound = true;
			double Radius = 0.0;
			double LineDeg = 0.0;
			double Behind = 0.0;
			double Level = 0.0;

			double DegOf(double S) const { return FMath::RadiansToDegrees(S / Radius); }
			double SOf(double Deg) const { return Stair::Around(Radius, Deg); }

			/** The height the run's heights stand over: the string's top round the well, the landing behind it. */
			double Base(double S) const { return bRound ? Stair::StringTop(DegOf(S)) : Level; }

			/** Where the base turns, in S. */
			TArray<double> Kinks() const
			{
				TArray<double> Out;
				for (const double Deg : bRound ? Stair::Kinks() : TArray<double>())
				{
					Out.Add(SOf(Deg));
				}
				return Out;
			}

			/** The line Over the base, less a drop, from S0 to S1, as drawn at W = Off: through every kink between. */
			TArray<FVector2D> Line(double S0, double S1, double Drop, double Over, double Off) const
			{
				if (!bRound)
				{
					return {FVector2D(S0, Level - Drop + Over), FVector2D(S1, Level - Drop + Over)};
				}
				return Stair::LineOver(Radius + Off, DegOf(S0), DegOf(S1), [Drop](double Deg) { return Stair::StringTop(Deg) - Drop; }, Over);
			}

			/** Sets a drawing in the plane: its W from Off, its own origin at the plane's (S, Z). */
			void Place(FFormDrawing& Drawing, double Off, double S, double Z) const
			{
				if (bRound)
				{
					Drawing.Ground = Stair::Cylinder(Radius + Off);
					Drawing.Frame = Turned(DegOf(S), FVector(0.0, 0.0, Z));
					return;
				}
				// x along the radial line, y up, and so z = x × y back towards the landing.
				const FVector At = Radial(LineDeg) * S + Tangent(LineDeg) * (Behind - Off) + FVector(0.0, 0.0, Z);
				Drawing.Frame = FrameOfAxes(At, Radial(LineDeg), FVector::UpVector);
			}
		};

		/** A run of the balustrade from S From to S To in its plane: fixed, or a retracting panel drawn at home. */
		struct FRailRun
		{
			FString Name;
			const FRunPlane* Plane = nullptr;
			double From = 0.0;
			double To = 0.0;
			double Drop = 0.0;
			bool bPanel = false;
			bool bStandardAtFrom = true;
			bool bStandardAtTo = true;
			FMuseeKey Body;

			double Base(double S) const { return Plane->Base(S) - Drop; }
		};

		/** A drawing of a run's plane. */
		template <typename TShape>
		void AddInPlane(TArray<FFormDrawing>& Out, const FRailRun& Run, const FString& Name, const TCHAR* LocalSurface, TShape&& Shape, double Off, double S,
			double Z)
		{
			FFormDrawing& Drawing = Out.Add_GetRef(Drawn(Name, LocalSurface, Forward<TShape>(Shape), Solid));
			Run.Plane->Place(Drawing, Off, S, Z);
			Drawing.Body = Run.Body;
		}

		/** A block of the plane between two corners, standing Thick out of it. */
		FFormExtrusion Block(const FVector2D& Low, const FVector2D& High, double Thick)
		{
			FFormExtrusion Shape;
			Shape.Outline = FFormProfile::Rectangle(Low, High);
			Shape.DepthMeters = Thick;
			return Shape;
		}

		/** The band of the plane between two lines over the same stations, standing Thick out of it. */
		FFormExtrusion Band(const TArray<FVector2D>& Upper, const TArray<FVector2D>& Lower, double Thick)
		{
			TArray<FVector2D> Outline = Upper;
			for (int32 Index = Lower.Num() - 1; Index >= 0; --Index)
			{
				Outline.Add(Lower[Index]);
			}
			FFormExtrusion Shape;
			Shape.Outline = FFormProfile::Polyline(Outline, true);
			Shape.DepthMeters = Thick;
			return Shape;
		}

		FFormProfile Round(double Radius)
		{
			return FFormProfile::Ellipse(FVector2D::ZeroVector, Radius, Radius, 0.0, false);
		}

		/** A bar's section: its height across the rake (A) by its thickness out of the plane (B). */
		FFormProfile BarSection(double HalfHigh, double Thick)
		{
			return FFormProfile::Rectangle(FVector2D(-HalfHigh, 0.0), FVector2D(HalfHigh, Thick));
		}

		/**
		 * The handrail's section, a soft bar (A across the rake, B out of the plane from 0 to its width): the superellipse
		 * whose cosine and sine are each raised to the roundness, drawn from its underside, where its seam lies unseen.
		 */
		FFormProfile HandrailSection()
		{
			const double HalfHigh = 0.5 * Plan::HandrailHigh;
			const double HalfAcross = 0.5 * Plan::HandrailAcross;
			auto Soft = [](double V) { return FMath::Sign(V) * FMath::Pow(FMath::Abs(V), Plan::HandrailRoundness); };
			auto At = [&](int32 Station)
			{
				const double T = UE_DOUBLE_PI + UE_DOUBLE_TWO_PI * Station / Plan::HandrailStations;
				return FVector2D(HalfHigh * Soft(FMath::Cos(T)), HalfAcross * (1.0 + Soft(FMath::Sin(T))));
			};
			FFormProfile Section = FFormProfile::From(At(0));
			for (int32 Station = 1; Station < Plan::HandrailStations; ++Station)
			{
				Section.LineTo(At(Station), EFormJoint::Smooth);
			}
			return Section.Close();
		}

		/** A sweep along a run Over its base, its section standing out of the plane from Off. */
		void RunSweep(TArray<FFormDrawing>& Out, const FRailRun& Run, const TCHAR* Part, const TCHAR* LocalSurface, FFormProfile Section, double Off,
			double Over)
		{
			FFormSweep Shape;
			Shape.Section = MoveTemp(Section);
			Shape.Path = FFormProfile::Polyline(Run.Plane->Line(Run.From, Run.To, Run.Drop, Over, Off), false);
			AddInPlane(Out, Run, Run.Name + Part, LocalSurface, MoveTemp(Shape), Off, 0.0, 0.0);
		}

		/** Where a run's standards stand: at every kink, and evenly between, no further apart along the rake than their pitch. */
		TArray<double> StandardStations(const FRailRun& Run)
		{
			TArray<double> Breaks = {Run.From};
			for (const double Kink : Run.Plane->Kinks())
			{
				if (Kink > Run.From + Plan::StandardSide && Kink < Run.To - Plan::StandardSide)
				{
					Breaks.Add(Kink);
				}
			}
			Breaks.Add(Run.To);
			TArray<double> Posts;
			for (int32 Gap = 0; Gap + 1 < Breaks.Num(); ++Gap)
			{
				const double A = Breaks[Gap];
				const double B = Breaks[Gap + 1];
				const double Length = FVector2D::Distance(FVector2D(A, Run.Base(A)), FVector2D(B, Run.Base(B)));
				const int32 Count = FMath::Max(1, FMath::CeilToInt32(Length / Plan::StandardPitch));
				for (int32 Step = 0; Step < Count; ++Step)
				{
					Posts.Add(FMath::Lerp(A, B, double(Step) / Count));
				}
			}
			Posts.Add(Run.To);
			// A run's end standards stand wholly within it.
			Posts[0] += 0.5 * Plan::StandardSide;
			Posts.Last() -= 0.5 * Plan::StandardSide;
			return Posts;
		}

		/** A standard and, on a fixed run, its bracket to the string: from its back to a centimetre into the string's face. */
		void Standard(TArray<FFormDrawing>& Out, const FRailRun& Run, int32 Index, double S)
		{
			const double Half = 0.5 * Plan::StandardSide;
			const double Foot = Run.bPanel ? Plan::CarrierBottom + Plan::StemInto : Plan::StandardFoot;
			const FString Name = FString::Printf(TEXT("%s.standard.%02d"), *Run.Name, Index + 1);
			AddInPlane(Out, Run, Name, TEXT("stair.balustrade"), Block(FVector2D(-Half, Foot), FVector2D(Half, Plan::HandrailHeight - Plan::StandardUnderRail),
				Plan::StandardSide), -Half, S, Run.Base(S));
			if (!Run.bPanel)
			{
				const double Into = Plan::StringInner + Plan::TreadInto - Run.Plane->Radius;
				AddInPlane(Out, Run, Name + TEXT(".bracket"), TEXT("stair.balustrade"), Block(FVector2D(-0.5 * Plan::BracketWide, Plan::BracketBottom),
					FVector2D(0.5 * Plan::BracketWide, Plan::BracketTop), Into - Half), Half, S, Run.Base(S));
			}
		}

		/** The balusters of the panel between two standards' faces: evenly, about their pitch apart, clear of the standards. */
		void Balusters(TArray<FFormDrawing>& Out, const FRailRun& Run, int32 Panel, double From, double To)
		{
			const double Half = 0.5 * Plan::BalusterSide;
			const double First = From + Plan::BalusterClear;
			const double Last = To - Plan::BalusterClear;
			if (Last - First < Plan::BalusterSide)
			{
				return;
			}
			const int32 Count = FMath::Max(1, FMath::FloorToInt32((Last - First) / Plan::BalusterPitch));
			const double Height = Plan::BarHeights[MiddleBar] - Plan::BarHeights[BottomBar];
			for (int32 Step = 0; Step <= Count; ++Step)
			{
				const double S = FMath::Lerp(First, Last, double(Step) / Count);
				AddInPlane(Out, Run, FString::Printf(TEXT("%s.panel.%02d.baluster.%02d"), *Run.Name, Panel + 1, Step + 1), TEXT("stair.balustrade"),
					Block(FVector2D(-Half, 0.0), FVector2D(Half, Height), Plan::BalusterSide), -Half, S, Run.Base(S) + Plan::BarHeights[BottomBar]);
			}
		}

		/** One ring at its centre: its band, its boss, and its four spokes as one cross. */
		void RailRing(TArray<FFormDrawing>& Out, const FRailRun& Run, const FString& Name, double Rho, double S, double Z)
		{
			const double Outer = Rho + Plan::RingBandOver;
			const double Inner = Outer - Plan::RingBand;
			FFormExtrusion Band;
			Band.Outline = Round(Outer);
			Band.Holes.Add(Round(Inner));
			Band.DepthMeters = Plan::RingThick;
			AddInPlane(Out, Run, Name + TEXT(".band"), TEXT("stair.balustrade"), MoveTemp(Band), -0.5 * Plan::RingThick, S, Z);
			FFormExtrusion Boss;
			Boss.Outline = Round(Plan::RingBossRadius);
			Boss.DepthMeters = Plan::RingBossThick;
			AddInPlane(Out, Run, Name + TEXT(".boss"), TEXT("stair.balustrade"), MoveTemp(Boss), -0.5 * Plan::RingBossThick, S, Z);
			// Four arms at 45° + 90° k from the heart into the band: arm k's leaving side meets arm k+1's arriving side.
			const double Reach = Inner + Plan::SpokeInto;
			const double Half = 0.5 * Plan::SpokeWide;
			TArray<FVector2D> Cross;
			for (int32 Arm = 0; Arm < 4; ++Arm)
			{
				const double Angle = FMath::DegreesToRadians(45.0 + 90.0 * Arm);
				const FVector2D Along(FMath::Cos(Angle), FMath::Sin(Angle));
				const FVector2D Side(-Along.Y, Along.X);
				Cross.Add(Along * Reach - Side * Half);
				Cross.Add(Along * Reach + Side * Half);
				Cross.Add((Along + Side) * Half);
			}
			FFormExtrusion Spokes;
			Spokes.Outline = FFormProfile::Polyline(Cross, true);
			Spokes.DepthMeters = Plan::SpokeThick;
			AddInPlane(Out, Run, Name + TEXT(".spokes"), TEXT("stair.balustrade"), MoveTemp(Spokes), -0.5 * Plan::SpokeThick, S, Z);
		}

		/**
		 * The rings of the panel between two standards' faces, touching both bars across the rake: the band's outer edge is
		 * the circle through the two bars' inner faces (run in by RingBandOver), and along the rake each touches the next.
		 */
		void Rings(TArray<FFormDrawing>& Out, const FRailRun& Run, int32 Panel, double From, double To)
		{
			const double Span = To - From;
			const double Rake = (Run.Base(To) - Run.Base(From)) / Span;
			const double Cos = 1.0 / FMath::Sqrt(1.0 + Rake * Rake);
			const double Rho = 0.5 * ((Plan::BarHeights[TopBar] - Plan::BarHeights[MiddleBar]) * Cos - 2.0 * Plan::BarHalfHigh);
			const double Step = 2.0 * Rho * Cos;
			const int32 Count = FMath::FloorToInt32((Span - 2.0 * Plan::RingClear) / Step);
			const double First = From + 0.5 * (Span - Count * Step) + 0.5 * Step;
			const double Middle = 0.5 * (Plan::BarHeights[MiddleBar] + Plan::BarHeights[TopBar]);
			for (int32 Index = 0; Index < Count; ++Index)
			{
				const double S = First + Index * Step;
				RailRing(Out, Run, FString::Printf(TEXT("%s.panel.%02d.ring.%02d"), *Run.Name, Panel + 1, Index + 1), Rho, S, Run.Base(S) + Middle);
			}
		}

		/** A run of the balustrade: its standards and registers, its bars and handrail, and a panel's carrier bar. */
		void RailRun(TArray<FFormDrawing>& Out, const FRailRun& Run)
		{
			const TArray<double> Posts = StandardStations(Run);
			for (int32 Index = 0; Index < Posts.Num(); ++Index)
			{
				const bool bLeftOut = (Index == 0 && !Run.bStandardAtFrom) || (Index + 1 == Posts.Num() && !Run.bStandardAtTo);
				if (!bLeftOut)
				{
					Standard(Out, Run, Index, Posts[Index]);
				}
			}
			const double Half = 0.5 * Plan::StandardSide;
			for (int32 Panel = 0; Panel + 1 < Posts.Num(); ++Panel)
			{
				Balusters(Out, Run, Panel, Posts[Panel] + Half, Posts[Panel + 1] - Half);
				Rings(Out, Run, Panel, Posts[Panel] + Half, Posts[Panel + 1] - Half);
			}
			for (int32 Bar = 0; Bar < int32(UE_ARRAY_COUNT(Plan::BarHeights)); ++Bar)
			{
				RunSweep(Out, Run, *FString::Printf(TEXT(".bar.%d"), Bar + 1), TEXT("stair.balustrade"), BarSection(Plan::BarHalfHigh, Plan::BarThick),
					-0.5 * Plan::BarThick, Plan::BarHeights[Bar]);
			}
			RunSweep(Out, Run, TEXT(".handrail"), TEXT("stair.handrail"), HandrailSection(), -0.5 * Plan::HandrailAcross, Plan::HandrailHeight);
			if (Run.bPanel)
			{
				RunSweep(Out, Run, TEXT(".carrier"), TEXT("stair.balustrade"), BarSection(0.5 * (Plan::CarrierTop - Plan::CarrierBottom), Plan::CarrierThick),
					-0.5 * Plan::CarrierThick, 0.5 * (Plan::CarrierBottom + Plan::CarrierTop));
			}
		}

		/** The newel at the fixed run's foot, standing on the landing: its post, its cap, and its ball. */
		void Newel(TArray<FFormDrawing>& Out, const FRailRun& Run)
		{
			const double Deg = Run.Plane->DegOf(Run.From);
			const FVector2D At = Polar(Run.Plane->Radius, Deg);
			const double Top = Run.Base(Run.From) + Plan::HandrailHeight + Plan::NewelOverRail;
			auto Square = [](double Side) { return FFormProfile::Rectangle(FVector2D(-0.5 * Side, -0.5 * Side), FVector2D(0.5 * Side, 0.5 * Side)); };
			auto Standing = [&](double Z) { return FrameOfAxes(FVector(At.X, At.Y, Z), Radial(Deg), Tangent(Deg)); };
			Out.Add(Drawn(Run.Name + TEXT(".newel.post"), TEXT("stair.balustrade"), Slab(Square(Plan::NewelSide), {}, Top - Plan::LandingZ), Solid,
				Standing(Plan::LandingZ)));
			Out.Add(Drawn(Run.Name + TEXT(".newel.cap"), TEXT("stair.balustrade"), Slab(Square(Plan::NewelCapSide), {}, Plan::NewelCapHigh), Solid,
				Standing(Top)));
			FFormLathe Ball;
			Ball.Section = FFormProfile::From(FVector2D(0.0, -Plan::NewelBallRadius))
				.ArcAbout(FVector2D::ZeroVector, Plan::NewelBallRadius, Plan::NewelBallRadius, -90.0, 90.0);
			Out.Add(Drawn(Run.Name + TEXT(".newel.ball"), TEXT("stair.balustrade"), MoveTemp(Ball), Solid,
				FMuseeFrame(FVector(At.X, At.Y, Top + Plan::NewelCapHigh + Plan::NewelBallRadius - Plan::NewelBallSeat))));
		}

		/**
		 * A retracting run's housing from S From to To in its plane, W Low to High across it, its top on the base: marble
		 * sides either side of the slot between the bronze cap and shoe, the bronze floor in the slot, a bronze end block at
		 * each end. It stands still.
		 */
		void Housing(TArray<FFormDrawing>& Out, const FRunPlane& Plane, const TCHAR* Name, double From, double To, double Low, double High)
		{
			FRailRun Fixed;
			Fixed.Name = FString::Printf(TEXT("stair.housing.%s"), Name);
			Fixed.Plane = &Plane;
			const double Slot = 0.5 * Plan::HousingSlot;
			const double Depth = Plan::HousingDepth;
			auto Part = [&](const TCHAR* Local, const TCHAR* LocalSurface, double S0, double S1, double Bottom, double Top, double W0, double W1)
			{
				FFormExtrusion Shape = Band(Plane.Line(S0, S1, 0.0, Top, W0), Plane.Line(S0, S1, 0.0, Bottom, W0), W1 - W0);
				AddInPlane(Out, Fixed, Fixed.Name + TEXT(".") + Local, LocalSurface, MoveTemp(Shape), W0, 0.0, 0.0);
			};
			const double A = From + Plan::HousingEndBlock;
			const double B = To - Plan::HousingEndBlock;
			Part(TEXT("side.low"), TEXT("stair.housing.side"), A, B, -Depth + Plan::HousingShoe, -Plan::HousingCap, Low, -Slot);
			Part(TEXT("side.high"), TEXT("stair.housing.side"), A, B, -Depth + Plan::HousingShoe, -Plan::HousingCap, Slot, High);
			Part(TEXT("cap.low"), TEXT("stair.housing.cap"), A, B, -Plan::HousingCap, 0.0, Low, -Slot);
			Part(TEXT("cap.high"), TEXT("stair.housing.cap"), A, B, -Plan::HousingCap, 0.0, Slot, High);
			Part(TEXT("shoe.low"), TEXT("stair.housing.cap"), A, B, -Depth, -Depth + Plan::HousingShoe, Low, -Slot);
			Part(TEXT("shoe.high"), TEXT("stair.housing.cap"), A, B, -Depth, -Depth + Plan::HousingShoe, Slot, High);
			Part(TEXT("floor"), TEXT("stair.housing.cap"), A, B, -Depth, -Depth + Plan::HousingFloor, -Slot, Slot);
			Part(TEXT("end.from"), TEXT("stair.housing.cap"), From, A, -Depth, 0.0, Low, High);
			Part(TEXT("end.to"), TEXT("stair.housing.cap"), B, To, -Depth, 0.0, Low, High);
		}

		/** A retracting panel inside its housing, 5 mm clear of the end blocks, riding the panels' body at home. */
		FRailRun PanelRun(const TCHAR* Name, const FRunPlane& Plane, double HousingFrom, double HousingTo)
		{
			FRailRun Run;
			Run.Name = FString::Printf(TEXT("stair.balustrade.panel.%s"), Name);
			Run.Plane = &Plane;
			Run.From = HousingFrom + Plan::HousingEndBlock + Plan::PanelClear;
			Run.To = HousingTo - Plan::HousingEndBlock - Plan::PanelClear;
			Run.Drop = Plan::PanelHome;
			Run.bPanel = true;
			Run.Body = PanelsBody();
			return Run;
		}

		/**
		 * The wall's rail: a round bar on the cylinder r 6.40, its middle 0.92 over the pitch line from 4° past the last riser
		 * up to tread 7's nosing, then level for a bend's length, returned level round the bend and a centimetre into the wall
		 * through a rose; and its brackets, from 0.30 below its top and every 1.25 on down it.
		 */
		void WallRail(TArray<FFormDrawing>& Out)
		{
			const double Radius = Plan::WallRailRadius;
			const double Half = 0.5 * Plan::WallRailDiameter;
			const double Ground = Radius - Half;
			const double Foot = Stair::Front(Plan::Risers - 1) - Plan::WallRailPastDeg;
			const double Top = Stair::Front(Plan::WallRailFromTread);
			const double Level = Top + FMath::RadiansToDegrees(Plan::WallReturnBend / Radius);
			TArray<FVector2D> Line = Stair::LineOver(Ground, Foot, Top, [](double Deg) { return Stair::Pitch(Deg); }, Plan::WallRailOverPitch);
			const double Z = Line.Last().Y;
			Line.Add(FVector2D(Stair::Around(Ground, Level), Z));
			FFormSweep Rail;
			Rail.Section = FFormProfile::Ellipse(FVector2D(0.0, Half), Half, Half, 0.0, false);
			Rail.Path = FFormProfile::Polyline(Line, false);
			Out.Add_GetRef(Drawn(TEXT("stair.handrail.wall"), TEXT("stair.handrail"), MoveTemp(Rail), Solid)).Ground = Stair::Cylinder(Ground);

			// The return, in plan: a quarter bend from the rail's level end towards the wall, then straight into it.
			const FVector2D Along(Tangent(Level));
			const FVector2D Out2(Radial(Level));
			const FVector2D Start = Polar(Radius, Level);
			const FVector2D Centre = Start + Out2 * Plan::WallReturnBend;
			const FVector2D Bent = Centre + Along * Plan::WallReturnBend;
			auto Reach = [&Bent, &Out2](double To)
			{
				const double Radially = FVector2D::DotProduct(Bent, Out2);
				return -Radially + FMath::Sqrt(FMath::Max(0.0, Radially * Radially - Bent.SizeSquared() + To * To));
			};
			const double From = FMath::RadiansToDegrees(FMath::Atan2(-Out2.Y, -Out2.X));
			FFormSweep Return;
			Return.Section = FFormProfile::Ellipse(FVector2D::ZeroVector, Half, Half, 0.0, false);
			Return.Path = FFormProfile::From(Start).ArcAbout(Centre, Plan::WallReturnBend, Plan::WallReturnBend, From, From - 90.0);
			Return.Path.LineTo(Bent + Out2 * Reach(Plan::TreadOuter + Plan::TreadInto));
			Return.ElevationMeters = Z;
			Out.Add(Drawn(TEXT("stair.handrail.wall.return"), TEXT("stair.handrail"), MoveTemp(Return), Solid));

			// The rose on the wall's face round the return, standing out of it into the stair.
			const FVector2D Face = Bent + Out2 * Reach(Plan::TreadOuter);
			FFormLathe Rose;
			Rose.Section = FFormProfile::Polyline({FVector2D(0.0, 0.0), FVector2D(Plan::RoseRadius, 0.0), FVector2D(Plan::RoseRadius, Plan::RoseProud),
				FVector2D(0.0, Plan::RoseProud)}, false);
			Out.Add(Drawn(TEXT("stair.handrail.wall.rose"), TEXT("stair.handrail"), MoveTemp(Rose), Solid,
				FrameOfAxes(FVector(Face.X, Face.Y, Z), FVector::UpVector, Tangent(Level))));

			// The brackets: a stem under the rail and an arm from its foot into the wall.
			const double Side = Plan::WallBracketSide;
			const double Length = Stair::Around(Radius, Top - Foot);
			int32 Index = 0;
			for (double S = Plan::WallBracketFirst; S < Length; S += Plan::WallBracketPitch)
			{
				const double Deg = Top - FMath::RadiansToDegrees(S / Radius);
				const double Middle = Stair::Pitch(Deg) + Plan::WallRailOverPitch;
				const FString Name = FString::Printf(TEXT("stair.handrail.wall.bracket.%02d"), ++Index);
				FFormExtrusion Stem;
				Stem.Outline = FFormProfile::Rectangle(FVector2D(-0.5 * Side, Plan::WallBracketFoot), FVector2D(0.5 * Side, Plan::WallBracketStemTop));
				Stem.DepthMeters = Side;
				FFormDrawing& StemDrawing = Out.Add_GetRef(Drawn(Name + TEXT(".stem"), TEXT("stair.handrail"), MoveTemp(Stem), Solid,
					Turned(Deg, FVector(0.0, 0.0, Middle))));
				StemDrawing.Ground = Stair::Cylinder(Radius - 0.5 * Side);
				FFormExtrusion Arm;
				Arm.Outline = FFormProfile::Rectangle(FVector2D(-0.5 * Side, Plan::WallBracketFoot), FVector2D(0.5 * Side, Plan::WallBracketArmTop));
				Arm.DepthMeters = Plan::TreadOuter + Plan::TreadInto - (Radius - 0.5 * Side);
				FFormDrawing& ArmDrawing = Out.Add_GetRef(Drawn(Name + TEXT(".arm"), TEXT("stair.handrail"), MoveTemp(Arm), Solid,
					Turned(Deg, FVector(0.0, 0.0, Middle))));
				ArmDrawing.Ground = Stair::Cylinder(Radius - 0.5 * Side);
			}
		}
	}

	void DrawStairFittings(TArray<FFormDrawing>& Out)
	{
		FRunPlane Well;
		Well.Radius = Plan::BalustradeRadius;
		const double Split = Well.SOf(Stair::Front(Plan::SplitTread));
		const double Landing = Well.SOf(Plan::TopLandingBackDeg);

		FRailRun Fixed;
		Fixed.Name = TEXT("stair.balustrade");
		Fixed.Plane = &Well;
		Fixed.From = Well.SOf(Stair::Front(Plan::Risers - 1) - Plan::BalustradePastDeg);
		Fixed.To = Split;
		Fixed.bStandardAtFrom = false;
		RailRun(Out, Fixed);
		Newel(Out, Fixed);

		const double WellHousingFrom = Split + Plan::HousingShort;
		Housing(Out, Well, TEXT("well"), WellHousingFrom, Landing, Plan::HousingInner - Well.Radius, Plan::HousingOuter - Well.Radius);
		RailRun(Out, PanelRun(TEXT("well"), Well, WellHousingFrom, Landing));

		FRunPlane Back;
		Back.bRound = false;
		Back.LineDeg = Plan::TopLandingBackDeg;
		Back.Behind = Plan::BackPanelBehind;
		Back.Level = Stair::TreadTop(1);
		const double BackFrom = Plan::HousingInner;
		const double BackTo = Plan::TreadOuter + Plan::TreadInto;
		Housing(Out, Back, TEXT("back"), BackFrom, BackTo, Plan::BackPanelBehind - Plan::BackHousingBehind, Plan::BackPanelBehind);
		RailRun(Out, PanelRun(TEXT("back"), Back, BackFrom, BackTo));

		WallRail(Out);
	}
}
