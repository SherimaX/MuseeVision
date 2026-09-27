#include "ReservePlan.h"
#include "ReserveParts.h"

/**
 * The luminaires' bodies (§3.8), which Light's lamps sit in: the 24 cased opal globe pendants at the bays' centres, the
 * nave's on chains and the aisles' on rods, each with its spun bronze gallery, socket cup, thumb screws and canopy at the
 * crown; the 56 uplight cans on the piers' impost ledges; and the easel spot's canopy, stem and head.
 */
namespace Musee::Reserve
{
	namespace
	{
		/** Out of reach overhead: nothing collides with the luminaires. */
		const Musee::Collision::FClass* const Overhead = nullptr;

		/** A sheet of metal spun to a section: the drawn line thickened to its left by the sheet, closed. */
		FFormProfile Spun(TConstArrayView<FVector2D> Line, double Sheet)
		{
			TArray<FVector2D> Inside;
			for (int32 Index = 0; Index < Line.Num(); ++Index)
			{
				const FVector2D Before = Line[FMath::Max(Index - 1, 0)];
				const FVector2D After = Line[FMath::Min(Index + 1, Line.Num() - 1)];
				const FVector2D Along = (After - Before).GetSafeNormal();
				Inside.Add(Line[Index] + FVector2D(-Along.Y, Along.X) * Sheet);
			}
			TArray<FVector2D> Loop(Line.GetData(), Line.Num());
			for (int32 Index = Inside.Num() - 1; Index >= 0; --Index)
			{
				Loop.Add(Inside[Index]);
			}
			return FFormProfile::Polyline(Loop, true);
		}

		/** The globe's glass (r, z above the cut): the sphere cut at 0.34 R, its neck and rolled lip, the 3 mm wall inside. */
		FFormLathe Globe(double R)
		{
			const double T = Plan::GlobeGlass;
			const double Neck = Plan::GlobeNeck * R;
			const double Below = FMath::Sqrt(R * R - Neck * Neck);
			const double Top = Plan::GlobeNeckHeight * R;
			const double CutDeg = FMath::RadiansToDegrees(FMath::Acos(Plan::GlobeNeck));
			const double InnerR = R - T;
			const double InnerNeck = Neck - T;
			const double InnerDeg = FMath::RadiansToDegrees(FMath::Acos(InnerNeck / InnerR));
			FFormProfile S = FFormProfile::From(FVector2D(0.0, -Below - R));
			S.ArcAbout(FVector2D(0.0, -Below), R, R, -90.0, CutDeg, EFormJoint::Smooth);
			// The neck rises from the sphere's cut at a crease; the lip is rolled.
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::GlobeLip)); ++Index)
			{
				S.LineTo(FVector2D(Neck + Plan::GlobeLip[Index][0], Top + Plan::GlobeLip[Index][1]), Index == 0 ? EFormJoint::Crisp : EFormJoint::Smooth);
			}
			S.LineTo(FVector2D(InnerNeck, Top + Plan::GlobeLip[1][1]), EFormJoint::Smooth);
			S.ArcAbout(FVector2D(0.0, -Below), InnerR, InnerR, InnerDeg, -90.0, EFormJoint::Crisp);
			FFormLathe Shape;
			Shape.Section = S.Close();
			return Shape;
		}

		/** A ball of a diameter about the frame's origin. */
		FFormLathe Ball(double Diameter)
		{
			const double R = 0.5 * Diameter;
			FFormLathe Shape;
			Shape.Section = FFormProfile::From(FVector2D(0.0, -R)).ArcAbout(FVector2D::ZeroVector, R, R, -90.0, 90.0, EFormJoint::Smooth).Close();
			return Shape;
		}

		/** A hex of a width across flats, from one height to another. */
		FFormLathe Hex(double AcrossFlats, double From, double To)
		{
			const double A = 0.5 * AcrossFlats;
			return Turned({FVector2D(0.0, From), FVector2D(A, From), FVector2D(A, To), FVector2D(0.0, To)}, 6);
		}

		/** A ring or a chain's link of round wire: a stadium of a straight length and a width, its centreline, in the frame's (x, y). */
		FFormSweep Link(double Straight, double Width)
		{
			const double R = 0.5 * Width;
			const double H = 0.5 * Straight;
			FFormSweep Shape;
			Shape.Section = Round(Plan::ChainWire);
			if (H <= 0.0)
			{
				Shape.Path = FFormProfile::Ellipse(FVector2D::ZeroVector, R, R, 0.0, true);
				return Shape;
			}
			Shape.Path = FFormProfile::From(FVector2D(R, -H));
			Shape.Path.LineTo(FVector2D(R, H), EFormJoint::Smooth).ArcAbout(FVector2D(0.0, H), R, R, 0.0, 180.0, EFormJoint::Smooth)
				.LineTo(FVector2D(-R, -H), EFormJoint::Smooth).ArcAbout(FVector2D(0.0, -H), R, R, 180.0, 360.0, EFormJoint::Smooth);
			Shape.Path.Close();
			return Shape;
		}

		/** A link or ring standing upright about a centre, in the plane of x (Turned false) or of y. */
		FMuseeFrame Upright(const FVector& Centre, bool bTurned)
		{
			return FrameOfAxes(Centre, bTurned ? FVector(0.0, 1.0, 0.0) : FVector(1.0, 0.0, 0.0), FVector::UpVector);
		}

		/**
		 * The nave's chain from the cup's top to the canopy: a loop on the cup, whole links each hung in the one above and
		 * turned 90° to it, and a loop at the top on the canopy's eye. Returns the height of the top loop's crown, where the
		 * canopy's eye comes down to meet it.
		 */
		double Chain(TArray<FFormDrawing>& Out, const FString& Name, const FVector2D& At, double CupTop, double Crown)
		{
			const double Wire = Plan::ChainWire;
			const double LoopR = 0.5 * Plan::LoopInside;
			const double LoopCentre = LoopR + 0.5 * Wire;
			const double HalfInside = 0.5 * Plan::LinkInsideLength;
			const double Pitch = Plan::LinkInsideLength;
			const double Eye = Crown + Plan::CanopyNeckZs[1];
			// Whole links from the cup's loop up to the highest that still leaves the top loop under the canopy's eye.
			const double Bottom = CupTop + LoopCentre + 0.5 * Wire;
			const double FirstLink = Bottom + LoopR + HalfInside;
			const int32 Links = 1 + FMath::FloorToInt32((Eye - LoopCentre - 0.5 * Wire - LoopR - HalfInside - FirstLink) / Pitch);
			const double LastLink = FirstLink + (Links - 1) * Pitch;
			const double TopLoop = LastLink + HalfInside + LoopR;

			const FFormSweep Ring = Link(0.0, 2.0 * LoopCentre);
			FFormDrawing Loops = Drawn(Name + TEXT(".loop"), TEXT("pendant.fitting"), Ring, Overhead);
			Loops.Repeats = {Upright(FVector(At.X, At.Y, Bottom), true), Upright(FVector(At.X, At.Y, TopLoop), Links % 2 == 1)};
			Out.Add(MoveTemp(Loops));
			for (const bool bTurned : {false, true})
			{
				FFormDrawing Each = Drawn(Name + (bTurned ? TEXT(".link.b") : TEXT(".link.a")), TEXT("pendant.fitting"),
					Link(Plan::LinkInsideLength + Wire - (Plan::LinkInsideWidth + Wire), Plan::LinkInsideWidth + Wire), Overhead);
				for (int32 Index = bTurned ? 1 : 0; Index < Links; Index += 2)
				{
					Each.Repeats.Add(Upright(FVector(At.X, At.Y, FirstLink + Index * Pitch), bTurned));
				}
				if (!Each.Repeats.IsEmpty())
				{
					Out.Add(MoveTemp(Each));
				}
			}
			return TopLoop + LoopCentre + 0.5 * Wire;
		}

		/** A canopy at the crown, its top let into the brick; its eye's neck comes down to Eye. */
		FFormLathe Canopy(double K, double Eye)
		{
			TArray<FVector2D> Points = {FVector2D(0.0, Eye), FVector2D(Plan::CanopyNeckRadius, Eye), FVector2D(Plan::CanopyNeckRadius, Plan::CanopyNeckZs[0])};
			for (int32 Index = UE_ARRAY_COUNT(Plan::CanopyRadii) - 1; Index >= 0; --Index)
			{
				Points.Add(FVector2D(K * Plan::CanopyRadii[Index], Plan::CanopyZs[Index]));
			}
			Points.Add(FVector2D(0.0, Plan::CanopyZs[0]));
			return Turned(Points);
		}

		/** One pendant at a bay's centre: its globe, gallery, cup and screws, and its chain (the nave) or rod (the aisles). */
		void Pendant(TArray<FFormDrawing>& Out, const FString& Name, const FVector2D& At, bool bNave)
		{
			const double R = 0.5 * (bNave ? Plan::NaveGlobeDiameter : Plan::AisleGlobeDiameter);
			const double Centre = bNave ? Plan::NaveGlobeCentre : Plan::AisleGlobeCentre;
			const double Cut = Centre + FMath::Sqrt(R * R - FMath::Square(Plan::GlobeNeck * R));
			const FMuseeFrame AtCut(FVector(At.X, At.Y, Cut));
			FFormDrawing Glass = Drawn(Name + TEXT(".globe"), TEXT("globe"), Globe(R), Overhead, AtCut);
			Glass.bNanite = false;
			Out.Add(MoveTemp(Glass));

			TArray<FVector2D> Gallery;
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::GalleryRadii)); ++Index)
			{
				Gallery.Add(FVector2D(Plan::GalleryRadii[Index] * R, Plan::GalleryZs[Index]));
			}
			FFormLathe Spinning;
			Spinning.Section = Spun(Gallery, Plan::GallerySheet);
			Out.Add(Drawn(Name + TEXT(".gallery"), TEXT("pendant.fitting"), MoveTemp(Spinning), Overhead, AtCut));
			TArray<FVector2D> Cup = {FVector2D(0.0, Plan::CupZs[0])};
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::CupRadii)); ++Index)
			{
				Cup.Add(FVector2D(Plan::CupRadii[Index] * R, Plan::CupZs[Index]));
			}
			Out.Add(Drawn(Name + TEXT(".cup"), TEXT("pendant.fitting"), Turned(Cup), Overhead, AtCut));

			// Three thumb screws through the gallery's band to the neck, at 90° + 120° k.
			const double Band = Plan::GalleryRadii[4] * R;
			const double NeckR = Plan::GlobeNeck * R;
			FFormDrawing Screws = Drawn(Name + TEXT(".thumb-screw"), TEXT("pendant.fitting"), Turned({FVector2D(0.0, NeckR - Band), FVector2D(0.5 * Plan::ThumbShank,
				NeckR - Band), FVector2D(0.5 * Plan::ThumbShank, 0.0), FVector2D(0.5 * Plan::ThumbHeadDiameter, 0.0), FVector2D(0.5 * Plan::ThumbHeadDiameter,
				Plan::ThumbHeadLength), FVector2D(0.0, Plan::ThumbHeadLength)}), Overhead);
			for (int32 K = 0; K < 3; ++K)
			{
				const double Angle = FMath::DegreesToRadians(90.0 + 120.0 * K);
				const FVector Out2D(FMath::Cos(Angle), FMath::Sin(Angle), 0.0);
				Screws.Repeats.Add(AlongAxis(FVector(At.X, At.Y, Cut + Plan::ThumbZ) + Out2D * Band, Out2D));
			}
			Out.Add(MoveTemp(Screws));

			const double Crown = Soffit(At.X, At.Y);
			const double CupTop = Cut + Plan::CupZs[UE_ARRAY_COUNT(Plan::CupZs) - 1];
			const FMuseeFrame AtCrown(FVector(At.X, At.Y, Crown));
			if (bNave)
			{
				const double Eye = Chain(Out, Name + TEXT(".chain"), At, CupTop, Crown);
				Out.Add(Drawn(Name + TEXT(".canopy"), TEXT("pendant.fitting"), Canopy(Plan::NaveCanopyK, Eye - Crown), Overhead, AtCrown));
				return;
			}
			const double NutTop = CupTop + Plan::LocknutHeight;
			const double Swivel = Crown - Plan::SwivelBelowCrown;
			const FMuseeFrame AtAxis(FVector(At.X, At.Y, 0.0));
			Out.Add(Drawn(Name + TEXT(".locknut"), TEXT("pendant.fitting"), Hex(Plan::LocknutAcrossFlats, CupTop, NutTop), Overhead, AtAxis));
			Out.Add(Drawn(Name + TEXT(".rod"), TEXT("pendant.fitting"), Cylinder(Plan::PendantRodDiameter, NutTop, Swivel), Overhead, AtAxis));
			Out.Add(Drawn(Name + TEXT(".swivel"), TEXT("pendant.fitting"), Ball(Plan::SwivelDiameter), Overhead, FMuseeFrame(FVector(At.X, At.Y, Swivel))));
			Out.Add(Drawn(Name + TEXT(".canopy"), TEXT("pendant.fitting"), Canopy(Plan::AisleCanopyK, Plan::CanopyNeckZs[1]), Overhead, AtCrown));
		}

		/** One uplight on an impost's corner ledge: the can aimed up the diagonal, its knuckle, and the stem down to the ledge. */
		void Uplight(TArray<FFormDrawing>& Out, const FString& Name, const FVector2D& Pier, const FVector2D& Corner)
		{
			const double Up = FMath::DegreesToRadians(Plan::UplightElevationDeg);
			const FVector2D Out2D = Corner.GetSafeNormal() * FMath::Cos(Up);
			const FVector Aim(Out2D.X, Out2D.Y, FMath::Sin(Up));
			const FVector Lens(Pier.X + Plan::UplightOff * Corner.X, Pier.Y + Plan::UplightOff * Corner.Y, Plan::UplightLensZ);
			const FVector Back = Lens - Aim * (Plan::CanLength - Plan::LensSetBack);
			const double R = 0.5 * Plan::CanDiameter;
			const double Bezel = R - Plan::BezelWidth;
			const double Face = Plan::CanLength - Plan::LensSetBack;
			Out.Add(Drawn(Name + TEXT(".can"), TEXT("uplight.body"), Turned({FVector2D(0.0, 0.0), FVector2D(R, 0.0), FVector2D(R, Plan::CanLength),
				FVector2D(Bezel, Plan::CanLength), FVector2D(Bezel, Face), FVector2D(0.0, Face)}), Overhead, AlongAxis(Back, Aim)));
			const FVector Knuckle = Back - Aim * (0.5 * Plan::KnuckleDiameter);
			Out.Add(Drawn(Name + TEXT(".knuckle"), TEXT("uplight.body"), Ball(Plan::KnuckleDiameter), Overhead, FMuseeFrame(Knuckle)));
			Out.Add(Drawn(Name + TEXT(".stem"), TEXT("uplight.body"), Cylinder(Plan::UplightStemDiameter, Plan::Springing, Knuckle.Z), Overhead,
				FMuseeFrame(FVector(Knuckle.X, Knuckle.Y, 0.0))));
		}

		/** The easel spot: the catalogue head, its face at the light's point aimed at the painting's centre, hung on its stem from a canopy on the web. */
		void EaselSpot(TArray<FFormDrawing>& Out)
		{
			const FVector Point(Plan::SpotX, Plan::SpotY, Plan::SpotZ);
			const FVector Aim = (FVector(Plan::SpotAim[0], Plan::SpotAim[1], Plan::SpotAim[2]) - Point).GetSafeNormal();
			const double R = 0.5 * Plan::SpotHeadDiameter;
			Out.Add(Drawn(TEXT("easel.spot.head"), TEXT("uplight.body"), Cylinder(Plan::SpotHeadDiameter, 0.0, Plan::SpotHeadLength), Overhead,
				AlongAxis(Point - Aim * Plan::SpotHeadLength, Aim)));
			// The stem meets the head's barrel where the vertical through the point crosses it.
			const double Meets = Point.Z + R / FMath::Sqrt(1.0 - Aim.Z * Aim.Z);
			const FMuseeFrame At(FVector(Plan::SpotX, Plan::SpotY, 0.0));
			const double Seat = SeatOver(Plan::SpotX, Plan::SpotY, 0.5 * Plan::RodCanopyDiameter);
			Out.Add(Drawn(TEXT("easel.spot.canopy"), TEXT("uplight.body"), Cylinder(Plan::RodCanopyDiameter, Seat - Plan::RodCanopyHeight, Seat), Overhead, At));
			Out.Add(Drawn(TEXT("easel.spot.stem"), TEXT("uplight.body"), Cylinder(Plan::SpotStemDiameter, Meets, Seat - Plan::RodCanopyHeight), Overhead, At));
		}
	}

	void DrawLuminaires(TArray<FFormDrawing>& Out)
	{
		for (int32 Bay = 0; Bay < Plan::Bays; ++Bay)
		{
			const double X = BayCentre(Bay);
			Pendant(Out, FString::Printf(TEXT("pendant.nave.%d"), Bay + 1), FVector2D(X, 0.0), true);
			Pendant(Out, FString::Printf(TEXT("pendant.aisle-n.%d"), Bay + 1), FVector2D(X, -Plan::AisleMiddle), false);
			Pendant(Out, FString::Printf(TEXT("pendant.aisle-s.%d"), Bay + 1), FVector2D(X, Plan::AisleMiddle), false);
		}
		for (int32 Line = 0; Line < Plan::PierLines; ++Line)
		{
			for (const double Side : {-1.0, 1.0})
			{
				const FVector2D Pier(PierX(Line), Side * Plan::ArcadeY);
				for (const double Dy : {-1.0, 1.0})
				{
					for (const double Dx : {1.0, -1.0})
					{
						Uplight(Out, FString::Printf(TEXT("uplight.p%d%s.%s%s"), Line + 1, Side < 0.0 ? TEXT("n") : TEXT("s"), Dy < 0.0 ? TEXT("n") : TEXT("s"),
							Dx < 0.0 ? TEXT("w") : TEXT("e")), Pier, FVector2D(Dx, Dy));
					}
				}
			}
		}
		EaselSpot(Out);
	}
}
