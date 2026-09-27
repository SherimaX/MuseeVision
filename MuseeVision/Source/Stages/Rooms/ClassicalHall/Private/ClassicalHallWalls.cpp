#include "ClassicalHallParts.h"
#include "ClassicalHallPlan.h"

/**
 * The walls and the structure under and over them (§1, §2, §3.1): the slab; the south wall with the port's rough opening
 * and the sleeve that carries it out to the well's cylinder; the gallery's long walls and the tribune's round wall, each
 * a laid finish of 30 mm (the red dado below 0.45, the cream field above) over its travertine core; the lintel over the
 * throat and the masonry over the tribune's wall. The niches' bays are ClassicalHallNiches'; the vaults and the roof
 * over them are the vaults' own.
 */
namespace Musee::ClassicalHall
{
	namespace
	{
		const FVector2D O = FVector2D::ZeroVector;
		const FVector2D W(0.0, Plan::WellAxisY);

		/** The finish's back: the core starts 30 mm behind every face. */
		constexpr double FinishBack(double Face) { return Face + Plan::LaidFinish; }

		/** The y on the port cylinder over a plan x (the cylinder bulges north into the hall). */
		double PortY(double X)
		{
			return Plan::WellAxisY - FMath::Sqrt(FMath::Square(Plan::WellRadius) - X * X);
		}

		/** The angle about W of the port cylinder's point over a plan x. */
		double PortDeg(double X)
		{
			return FMath::RadiansToDegrees(FMath::Atan2(PortY(X) - Plan::WellAxisY, X));
		}

		/** The angle about O of a circle's point at a plan x on the side of positive y (the throat's side). */
		double ThroatDeg(double Radius, double X)
		{
			return FMath::RadiansToDegrees(FMath::Acos(X / Radius));
		}

		/** The shell's plan north of a y: the side faces down to the tribune's shell and round it the long way by the north. */
		FFormProfile ShellPlanNorthOf(double Y)
		{
			const double SideY = FMath::Sqrt(FMath::Square(Plan::ShellRadius) - FMath::Square(Plan::ShellHalfWidth));
			const double East = ThroatDeg(Plan::ShellRadius, Plan::ShellHalfWidth);
			FPen Pen(Plan::ShellHalfWidth, Y);
			Pen.To(Plan::ShellHalfWidth, SideY).Arc(O, Plan::ShellRadius, East, -180.0 - East).To(-Plan::ShellHalfWidth, Y);
			return Pen.Closed();
		}

		/** The slab under everything north of the south wall's face, z −0.40 to the floor stones' bed. */
		void Slab(TArray<FFormDrawing>& Out)
		{
			Out.Add(PlanPrism(TEXT("shell.slab"), TEXT("shell"), ShellPlanNorthOf(Plan::SouthFaceY), {}, Plan::SlabUnderside, -Plan::FloorStone, Solid));
		}

		/** The south face's plan, from y 31.10 to the port cylinder within the sleeve, and to the shell's south face beside it. */
		FFormProfile PassagePlan()
		{
			const double H = Plan::ShellHalfWidth;
			const double S = Plan::SleeveHalf;
			FPen Pen(-H, Plan::SouthFaceY);
			Pen.To(H, Plan::SouthFaceY).To(H, Plan::ShellSouthY).To(S, Plan::ShellSouthY).To(S, PortY(S));
			Pen.Arc(W, Plan::WellRadius, PortDeg(S), PortDeg(-S)).To(-S, Plan::ShellSouthY).To(-H, Plan::ShellSouthY);
			return Pen.Closed();
		}

		/** An arch-headed opening's outline in an elevation (A across, B up), cut up from the bottom of a band. */
		void ArchNotch(FPen& Pen, double Half, double Springing, double Bottom)
		{
			Pen.To(-Half, Bottom).To(-Half, Springing).Arc(FVector2D(0.0, Springing), Half, 180.0, 0.0).To(Half, Bottom);
		}

		/**
		 * The south wall: the finish on its face (the dado either side of the opening, the field round the arch up to the
		 * vault's plaster), and its core out to the shell's south face, pierced by the rough opening (the class opening
		 * grown by the lining allowance), in the elevation seen from the well (x west, z up, pushed south).
		 */
		void SouthWall(TArray<FFormDrawing>& Out)
		{
			const double Half = Plan::PortClearHalf + Plan::LiningAllowance;
			const double Sill = -Plan::LiningAllowance;
			const double Edge = FinishBack(Plan::GalleryHalf);
			const double Face = Plan::SouthFaceY;
			for (const double Side : {-1.0, 1.0})
			{
				const FString Name = FString::Printf(TEXT("south.dado.%s"), Side > 0.0 ? TEXT("east") : TEXT("west"));
				Out.Add(PlanPrism(Name, TEXT("wall.dado"), Rect(FMath::Min(Side * Half, Side * Edge), Face, FMath::Max(Side * Half, Side * Edge),
					FinishBack(Face)), {}, Sill, Plan::DadoTop, Solid));
			}
			const FMuseeFrame South = FrameOfAxes(FVector(0.0, Face, 0.0), FVector(-1.0, 0.0, 0.0), FVector::UpVector);
			const FVector2D Springing = Barrel(Plan::VaultSpringingS, Plan::VaultRadius + Plan::VaultPlaster);
			const FVector2D Axis(0.0, Plan::VaultAxisZ);
			const double Top = FMath::RadiansToDegrees(FMath::Atan2(Springing.Y - Axis.Y, Springing.X));
			FPen Field(-Edge, Plan::DadoTop);
			ArchNotch(Field, Half, Plan::PortSpringing, Plan::DadoTop);
			Field.To(Edge, Plan::DadoTop).To(Edge, Plan::Ledge).To(Springing.X, Plan::Ledge).To(Springing.X, Springing.Y);
			Field.Arc(Axis, Plan::VaultRadius + Plan::VaultPlaster, Top, 180.0 - Top).To(-Springing.X, Plan::Ledge).To(-Edge, Plan::Ledge);
			Out.Add(Drawn(TEXT("south.field"), TEXT("wall.field"), Prism(Field.Closed(), Plan::LaidFinish), Solid, South));

			FPen Core(-Plan::ShellHalfWidth, Sill);
			ArchNotch(Core, Half, Plan::PortSpringing, Sill);
			Core.To(Plan::ShellHalfWidth, Sill).To(Plan::ShellHalfWidth, Plan::RoofTop).To(-Plan::ShellHalfWidth, Plan::RoofTop);
			const FMuseeFrame Behind(South.PointToParent(FVector(0.0, 0.0, Plan::LaidFinish)), South.Rotation);
			Out.Add(Drawn(TEXT("shell.south"), TEXT("shell"), Prism(Core.Closed(), Plan::ShellSouthY - FinishBack(Face)), Solid, Behind));
			Out.Add(PlanPrism(TEXT("shell.passage.base"), TEXT("shell"), PassagePlan(), {}, Plan::SlabUnderside, Sill, Solid));
		}

		/**
		 * The sleeve that carries the rough opening from the shell's south face to the port cylinder, round the collar's
		 * envelope: one wall, its face the shell's south face y 31.40 (looking north) from x -2.10 to +2.10 and its back the
		 * port cylinder straight behind it, standing from the rough sill with its top the collar's round head (r 2.10 about
		 * the springing); the rough opening cut through it as the class's straight passage, jambs x ±1.85 and head r 1.85,
		 * so the joint's lining and voussoirs lie against it all the way round, with nothing open behind them.
		 */
		void Sleeve(TArray<FFormDrawing>& Out)
		{
			const double Half = Plan::PortClearHalf + Plan::LiningAllowance;
			const double Sill = -Plan::LiningAllowance;
			const double S = Plan::SleeveHalf;
			const double Jambs = Plan::PortSpringing - Sill;
			FFormWall Shape;
			Shape.Plan = FFormProfile::Polyline({FVector2D(-S, Plan::ShellSouthY), FVector2D(S, Plan::ShellSouthY)}, false);
			Shape.Back = FFormProfile::From(FVector2D(-S, PortY(-S)));
			Shape.Back.ArcAbout(W, Plan::WellRadius, Plan::WellRadius, PortDeg(-S), PortDeg(S), EFormJoint::Smooth);
			Shape.BaseMeters = Sill;
			Shape.HeightMeters = Jambs + S;
			// The collar's head on the face's development, which is the plan's x from the sleeve's west end.
			Shape.Top = FFormProfile::From(FVector2D(0.0, Jambs));
			Shape.Top.ArcAbout(FVector2D(S, Jambs), S, S, 180.0, 0.0, EFormJoint::Smooth);
			FFormWallOpening& Opening = Shape.Openings.AddDefaulted_GetRef();
			Opening.CenterMeters = S;
			Opening.WidthMeters = 2.0 * Half;
			Opening.HeadMeters = Jambs;
			Opening.bArched = true;
			Opening.bStraight = true;
			Out.Add(Drawn(TEXT("shell.sleeve"), TEXT("port.rough-opening"), MoveTemp(Shape), Solid));
		}

		/** The stretches of a gallery wall between the niches' bays, from one y to another. */
		TArray<TPair<double, double>> Stretches(double From, double To)
		{
			TArray<TPair<double, double>> Out;
			double Start = From;
			for (int32 Bay = Plan::Bays - 1; Bay >= 0; --Bay)
			{
				const double Half = Plan::GalleryNicheRadius + Plan::SurroundBand;
				Out.Add({Start, Plan::BayCentres[Bay] - Half});
				Start = Plan::BayCentres[Bay] + Half;
			}
			Out.Add({Start, To});
			return Out;
		}

		/**
		 * A gallery long wall between its niches: the finish (dado and field) on the face x ±3.00 from the throat's end on
		 * the tribune's wall to the south wall, and the core from the screen line to the south wall's core.
		 */
		void GalleryWall(TArray<FFormDrawing>& Out, double Side)
		{
			const TCHAR* Name = Side > 0.0 ? TEXT("east") : TEXT("west");
			const double Face = Plan::GalleryHalf;
			const double Back = FinishBack(Face);
			const double ThroatEnd = FMath::Sqrt(FMath::Square(FinishBack(Plan::TribuneRadius)) - Face * Face);
			const TArray<TPair<double, double>> Finish = Stretches(ThroatEnd, Plan::SouthFaceY);
			const TArray<TPair<double, double>> Core = Stretches(Plan::ScreenY, FinishBack(Plan::SouthFaceY));
			for (int32 Index = 0; Index < Finish.Num(); ++Index)
			{
				const FFormProfile Layer = Rect(FMath::Min(Side * Face, Side * Back), Finish[Index].Key, FMath::Max(Side * Face, Side * Back), Finish[Index].Value);
				Out.Add(PlanPrism(FString::Printf(TEXT("gallery.%s.dado.%d"), Name, Index + 1), TEXT("wall.dado"), Layer, {}, -Plan::FloorStone,
					Plan::DadoTop, Solid));
				Out.Add(PlanPrism(FString::Printf(TEXT("gallery.%s.field.%d"), Name, Index + 1), TEXT("wall.field"), Layer, {}, Plan::DadoTop, Plan::Ledge,
					Solid));
				const FFormProfile Mass = Rect(FMath::Min(Side * Back, Side * Plan::ShellHalfWidth), Core[Index].Key,
					FMath::Max(Side * Back, Side * Plan::ShellHalfWidth), Core[Index].Value);
				Out.Add(PlanPrism(FString::Printf(TEXT("shell.gallery.%s.%d"), Name, Index + 1), TEXT("shell"), Mass, {}, -Plan::FloorStone, Plan::Ledge,
					Solid));
			}
		}

		/** The tribune niche at a plan angle. */
		const FNiche& TribuneNicheAt(double Deg)
		{
			return *Niches().FindByPredicate([Deg](const FNiche& Niche)
			{
				return Niche.bTribune && Niche.In.Equals(Direction(Deg), 1e-9);
			});
		}

		/** A point in a tribune niche's plan: Axial out from O along its axis, Lateral along its mouth plane (+ towards larger φ). */
		FVector2D AtNiche(const FNiche& Niche, double Axial, double Lateral)
		{
			const FVector Point = Niche.In * Axial + Niche.Lateral() * Lateral;
			return FVector2D(Point.X, Point.Y);
		}

		/** The plan angle of a point of a circle at a lateral offset from a niche's axis. */
		double CircleDeg(const FNiche& Niche, double Radius, double Lateral)
		{
			return FMath::RadiansToDegrees(FMath::Atan2(Niche.In.Y, Niche.In.X) + FMath::Asin(Lateral / Radius));
		}

		/** The tribune's niches and the throat's two edges, in turn round the wall from the throat's west edge. */
		struct FRingEnd
		{
			const FNiche* Niche = nullptr;
			double ThroatX = 0.0;
		};

		TArray<FRingEnd> RingEnds()
		{
			TArray<FRingEnd> Ends = {{nullptr, -Plan::GalleryHalf}};
			for (const double Deg : {150.0, 210.0, 270.0, 330.0, 30.0})
			{
				Ends.Add({&TribuneNicheAt(Deg), 0.0});
			}
			Ends.Add({nullptr, Plan::GalleryHalf});
			return Ends;
		}

		/** The angle of a ring end on a circle: at a niche, the lateral side it leaves (+) or reaches (−); at the throat, its edge. */
		double EndDeg(const FRingEnd& End, double Radius, double Lateral)
		{
			const double Deg = End.Niche ? CircleDeg(*End.Niche, Radius, Lateral) : ThroatDeg(Radius, End.ThroatX);
			return Deg;
		}

		/**
		 * One stretch of the tribune wall's finish between two ends, a band between two heights: the face on r 6.60, the
		 * back on r 6.63, turned at each niche into the hook that returns to the niche's mouth plane (the sliver reveal)
		 * and at the throat cut on the jamb's plane.
		 */
		FFormProfile RingFinish(const FRingEnd& From, const FRingEnd& To)
		{
			const double Face = Plan::TribuneRadius;
			const double Back = FinishBack(Face);
			auto Unwrapped = [](double Deg, double After)
			{
				while (Deg < After)
				{
					Deg += 360.0;
				}
				return Deg;
			};
			const double Start = EndDeg(From, Face, From.Niche ? From.Niche->BayHalf() : 0.0);
			const double StartBack = EndDeg(From, Back, From.Niche ? From.Niche->BayHalf() + Plan::LaidFinish : 0.0);
			const double End = Unwrapped(EndDeg(To, Face, To.Niche ? -To.Niche->BayHalf() : 0.0), Start);
			const double EndBack = Unwrapped(EndDeg(To, Back, To.Niche ? -(To.Niche->BayHalf() + Plan::LaidFinish) : 0.0), StartBack);
			FPen Pen(Polar(Face, Start));
			Pen.Arc(O, Face, Start, End);
			if (To.Niche)
			{
				const double Half = To.Niche->BayHalf();
				const FVector2D A = AtNiche(*To.Niche, Back, -Half);
				const FVector2D B = AtNiche(*To.Niche, Back, -(Half + Plan::LaidFinish));
				Pen.To(A.X, A.Y).To(B.X, B.Y);
			}
			Pen.Arc(O, Back, EndBack, StartBack);
			if (From.Niche)
			{
				const double Half = From.Niche->BayHalf();
				const FVector2D A = AtNiche(*From.Niche, Back, Half + Plan::LaidFinish);
				const FVector2D B = AtNiche(*From.Niche, Back, Half);
				Pen.To(A.X, A.Y).To(B.X, B.Y);
			}
			return Pen.Closed();
		}

		/** The tribune wall's finish, six stretches between the throat and the niches, each in its dado and its field. */
		void TribuneFinish(TArray<FFormDrawing>& Out)
		{
			const TArray<FRingEnd> Ends = RingEnds();
			for (int32 Index = 0; Index + 1 < Ends.Num(); ++Index)
			{
				const FFormProfile Band = RingFinish(Ends[Index], Ends[Index + 1]);
				Out.Add(PlanPrism(FString::Printf(TEXT("tribune.dado.%d"), Index + 1), TEXT("wall.dado"), Band, {}, -Plan::FloorStone, Plan::DadoTop, Solid));
				Out.Add(PlanPrism(FString::Printf(TEXT("tribune.field.%d"), Index + 1), TEXT("wall.field"), Band, {}, Plan::DadoTop, Plan::Ledge, Solid));
			}
		}

		/**
		 * The tribune wall's core to the ledge: from the finish's back (r 6.63) to the shell (r 7.90), cut back at the
		 * throat to the gallery's cores and at each niche to the niche's bay, which fills it.
		 */
		void TribuneCore(TArray<FFormDrawing>& Out)
		{
			const double Back = FinishBack(Plan::TribuneRadius);
			const double Jamb = FinishBack(Plan::GalleryHalf);
			const double SideY = FMath::Sqrt(FMath::Square(Plan::ShellRadius) - FMath::Square(Plan::ShellHalfWidth));
			const double East = ThroatDeg(Plan::ShellRadius, Plan::ShellHalfWidth);
			FPen Pen(Jamb, Plan::ScreenY);
			Pen.To(Plan::ShellHalfWidth, Plan::ScreenY).To(Plan::ShellHalfWidth, SideY).Arc(O, Plan::ShellRadius, East, -180.0 - East);
			Pen.To(-Plan::ShellHalfWidth, Plan::ScreenY).To(-Jamb, Plan::ScreenY);
			double Deg = ThroatDeg(Back, -Jamb);
			for (const double NicheDeg : {150.0, 210.0, 270.0, 330.0, 390.0})
			{
				const FNiche& Niche = TribuneNicheAt(FMath::Fmod(NicheDeg, 360.0));
				const double Half = Niche.BayHalf() + Plan::LaidFinish;
				double Reach = CircleDeg(Niche, Back, -Half);
				while (Reach < Deg) { Reach += 360.0; }
				Pen.Arc(O, Back, Deg, Reach);
				const FVector2D Corners[] = {AtNiche(Niche, Plan::TribuneRadius + Plan::NicheBayDepth, -Half), AtNiche(Niche, Plan::TribuneRadius + Plan::NicheBayDepth, Half)};
				Pen.To(Corners[0].X, Corners[0].Y).To(Corners[1].X, Corners[1].Y);
				Deg = CircleDeg(Niche, Back, Half);
				while (Deg < Reach) { Deg += 360.0; }
				const FVector2D Leave = Polar(Back, Deg);
				Pen.To(Leave.X, Leave.Y);
			}
			Pen.Arc(O, Back, Deg, ThroatDeg(Back, Jamb) + 360.0);
			Out.Add(PlanPrism(TEXT("shell.tribune.lower"), TEXT("shell"), Pen.Closed(), {}, -Plan::FloorStone, Plan::Ledge, Solid));
		}

		/**
		 * Over the ledge: the tribune wall's masonry from r 6.60 to the shell, up to the roof's top; the lintel over the
		 * throat (x ±3.00, from the tribune's face to the screen line, soffit 3.075, its face under the vault's end arc),
		 * and the roof over it.
		 */
		void OverTheLedge(TArray<FFormDrawing>& Out)
		{
			const double Face = Plan::TribuneRadius;
			const double Half = Plan::GalleryHalf;
			const double SideY = FMath::Sqrt(FMath::Square(Plan::ShellRadius) - FMath::Square(Plan::ShellHalfWidth));
			const double East = ThroatDeg(Plan::ShellRadius, Plan::ShellHalfWidth);
			const double JambY = FMath::Sqrt(Face * Face - Half * Half);
			FPen Ring(Half, Plan::ScreenY);
			Ring.To(Plan::ShellHalfWidth, Plan::ScreenY).To(Plan::ShellHalfWidth, SideY).Arc(O, Plan::ShellRadius, East, -180.0 - East);
			Ring.To(-Plan::ShellHalfWidth, Plan::ScreenY).To(-Half, Plan::ScreenY).To(-Half, JambY);
			Ring.Arc(O, Face, ThroatDeg(Face, -Half), ThroatDeg(Face, Half) + 360.0);
			Out.Add(PlanPrism(TEXT("shell.tribune.upper"), TEXT("shell"), Ring.Closed(), {}, Plan::Ledge, Plan::RoofTop, Solid));

			FPen Throat(Half, Plan::ScreenY);
			Throat.To(-Half, Plan::ScreenY).To(-Half, JambY).Arc(O, Face, ThroatDeg(Face, -Half), ThroatDeg(Face, Half));
			Out.Add(PlanPrism(TEXT("shell.lintel"), TEXT("lintel"), Throat.Closed(), {}, Plan::LintelSoffit, Plan::RoofSoffit, Solid));
			Out.Add(PlanPrism(TEXT("shell.throat.roof"), TEXT("shell"), Throat.Closed(), {}, Plan::RoofSoffit, Plan::RoofTop, Solid));
		}
	}

	void DrawWalls(TArray<FFormDrawing>& Out)
	{
		Slab(Out);
		SouthWall(Out);
		Sleeve(Out);
		GalleryWall(Out, 1.0);
		GalleryWall(Out, -1.0);
		TribuneFinish(Out);
		TribuneCore(Out);
		OverTheLedge(Out);
	}
}
