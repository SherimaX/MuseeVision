#include "SalonParts.h"
#include "SalonPlan.h"

/**
 * The walls (§3.1, §2): the travertine masonry of the long walls, the end wall with its step from the Rotunda's arch
 * to the Salon's own door, and the far wall with the oval's rough opening; the stone linings that carry each door's
 * reveal out to its architrave; the stretched silk of every bay and of the cabinet; and the two lunettes over the end
 * walls, up behind the vault.
 */
namespace Musee::Salon
{
	namespace
	{
		/** A door in a wall: centred a distance along the face, a clear width, springing above the wall's bed, arched. */
		FFormWallOpening Door(double Along, double Width, double Springing)
		{
			FFormWallOpening Opening;
			Opening.CenterMeters = Along;
			Opening.WidthMeters = Width;
			Opening.HeadMeters = Springing - Plan::WallBed;
			Opening.bArched = true;
			return Opening;
		}

		/**
		 * A stretch of masonry from the floor build-up's underside to the roof's soffit, its face on a straight line walked
		 * with the wall to the right, finished inside behind the silk as the piers are, outside as the exterior.
		 */
		FFormDrawing Masonry(const TCHAR* Name, const FVector2D& From, const FVector2D& To, double Thickness, TArray<FFormWallOpening> Openings,
			const TCHAR* Reveal, const TCHAR* Face = TEXT("pier"))
		{
			FFormWall Wall;
			Wall.Plan = FFormProfile::Polyline({From, To}, false);
			Wall.ThicknessMeters = Thickness;
			Wall.BaseMeters = Plan::WallBed;
			Wall.HeightMeters = Plan::WallTop - Plan::WallBed;
			Wall.Openings = MoveTemp(Openings);
			FFormDrawing Drawing = Drawn(Name, Face, MoveTemp(Wall), Solid);
			for (const TCHAR* Part : {FFormWall::BackPart, FFormWall::TopPart, FFormWall::BedPart, FFormWall::EndPart})
			{
				Drawing.PartSurfaces.Add(Part, Surface(TEXT("exterior.wall")));
			}
			Drawing.PartSurfaces.Add(FFormWall::RevealPart, Surface(Reveal));
			return Drawing;
		}

		/**
		 * The long walls, the end wall's two layers and the far wall. The north wall is thinner behind the cabinet, whose
		 * silk face is the Salon's outer face; the end wall's inner layer carries the Salon's own door, its outer layer the
		 * Rotunda's class opening, and the inner layer's back between them is the step.
		 */
		void MasonryWalls(TArray<FFormDrawing>& Out)
		{
			const double M = Plan::MasonryHalf;
			const double Full = Plan::OuterHalf - M;
			const double CabinetEastOuter = Plan::CabinetEast + Plan::CabinetWall;
			const double CabinetWestOuter = Plan::CabinetWest - Plan::CabinetWall;
			const double BehindCabinet = -Plan::CabinetSouth - Plan::SilkBuildUp - M;
			Out.Add(Masonry(TEXT("wall.north.east"), {Plan::EndOuterX, -M}, {CabinetEastOuter, -M}, Full, {}, TEXT("door.reveal")));
			Out.Add(Masonry(Pieces::NorthWallCabinet, {CabinetEastOuter, -M}, {CabinetWestOuter, -M}, BehindCabinet,
				{Door(CabinetEastOuter - Plan::CabinetDoorX, Plan::DoorWidth, Plan::DoorSpringing)}, TEXT("door.reveal")));
			Out.Add(Masonry(TEXT("wall.north.west"), {CabinetWestOuter, -M}, {Plan::FarOuterX, -M}, Full, {}, TEXT("door.reveal")));
			Out.Add(Masonry(TEXT("wall.south"), {Plan::FarOuterX, M}, {Plan::EndOuterX, M}, Full, {}, TEXT("door.reveal")));

			const double Rough = Plan::DoorWidth + 2.0 * Plan::LiningAllowance;
			FFormDrawing Inner = Masonry(TEXT("wall.east.inner"), {Plan::EndMasonryX, M}, {Plan::EndMasonryX, -M}, Plan::EastStepX - Plan::EndMasonryX,
				{Door(M, Plan::DoorWidth, Plan::DoorSpringing)}, TEXT("east.reveal"));
			Inner.PartSurfaces.Add(FFormWall::BackPart, Surface(TEXT("east.step")));
			Out.Add(MoveTemp(Inner));
			Out.Add(Masonry(TEXT("wall.east.outer"), {Plan::EastStepX, M}, {Plan::EastStepX, -M}, Plan::EndOuterX - Plan::EastStepX,
				{Door(M, Rough, Plan::EastClassSpringing)}, TEXT("port.rough-opening"), TEXT("exterior.wall")));
			Out.Add(Masonry(TEXT("wall.west"), {Plan::FarMasonryX, -M}, {Plan::FarMasonryX, M}, Plan::FarMasonryX - Plan::FarOuterX,
				{Door(M, Rough, Plan::DoorSpringing)}, TEXT("port.rough-opening")));
		}

		/**
		 * The stone linings round the doors in the silk's 25 mm build-up: out from each opening to where the silk stops,
		 * under the architrave, so the reveal is stone from the architrave's back through the wall. The oval's door is
		 * the rough opening, which the joint lines.
		 */
		void Linings(TArray<FFormDrawing>& Out)
		{
			const double Half = 0.5 * Plan::DoorWidth;
			const double RoughHalf = Half + Plan::LiningAllowance;
			const double Depth = Plan::SilkBuildUp;
			Out.Add(FaceSlab(TEXT("door.east.lining"), TEXT("east.reveal"), FaceFrame(FVector(Plan::EndMasonryX, 0.0, 0.0), FMuseeFrame::West()),
				ArchedBand(Half, Half + Plan::LiningMargin, Plan::DoorSpringing, Plan::WallBed), {}, Depth, Solid));
			Out.Add(FaceSlab(TEXT("door.west.lining"), TEXT("port.rough-opening"), FaceFrame(FVector(Plan::FarMasonryX, 0.0, 0.0), FMuseeFrame::East()),
				ArchedBand(RoughHalf, RoughHalf + Plan::LiningMargin, Plan::DoorSpringing, Plan::WallBed), {}, Depth, Solid));
			const FFormProfile Cabinet = ArchedBand(Half, Half + Plan::LiningMargin, Plan::DoorSpringing, Plan::WallBed);
			Out.Add(FaceSlab(TEXT("door.cabinet.lining.bay"), TEXT("door.reveal"),
				FaceFrame(FVector(Plan::CabinetDoorX, -Plan::MasonryHalf, 0.0), FMuseeFrame::South()), Cabinet, {}, Depth, Solid));
			Out.Add(FaceSlab(TEXT("door.cabinet.lining.cabinet"), TEXT("door.reveal"),
				FaceFrame(FVector(Plan::CabinetDoorX, Plan::CabinetSouth + Plan::SilkBuildUp, 0.0), FMuseeFrame::North()), Cabinet, {}, Depth, Solid));
		}

		/** Where a stretch of silk lies on its wall: the masonry face's two ends, the room's side, and a door in it if any. */
		struct FSilkStretch
		{
			const TCHAR* Name;
			const TCHAR* Surface;
			FVector From;
			FVector To;
			FVector N;
			double Top;
			/** The door's centre on the face, its half-width at the silk's edge and its springing; no door when the half is zero. */
			FVector DoorAt = FVector::ZeroVector;
			double DoorHalf = 0.0;
		};

		/**
		 * One stretch of silk: 25 mm from the masonry face to the silk plane, from the skirting's slot to behind the cornice,
		 * stopped round a door under its architrave.
		 */
		FFormDrawing Silk(const FSilkStretch& S)
		{
			const FMuseeFrame Face = FaceFrame(S.From, S.N);
			const FVector Along = FVector::CrossProduct(FVector::UpVector, S.N);
			const double U0 = FMath::Min(0.0, FVector::DotProduct(S.To - S.From, Along));
			const double U1 = FMath::Max(0.0, FVector::DotProduct(S.To - S.From, Along));
			const double V0 = Plan::SilkBottom;
			const double V1 = S.Top;
			TArray<FVector2D> Corners = {{U0, V0}, {U1, V0}, {U1, V1}, {U0, V1}};
			if (S.DoorHalf <= 0.0)
			{
				return FaceSlab(S.Name, S.Surface, Face, FFormProfile::Polygon(Corners), {}, Plan::SilkBuildUp, Solid);
			}
			const double C = FVector::DotProduct(S.DoorAt - S.From, Along);
			const double W = S.DoorHalf;
			FFormProfile Outline = FFormProfile::From(FVector2D(U0, V0));
			Outline.LineTo(FVector2D(C - W, V0), EFormJoint::Crisp).LineTo(FVector2D(C - W, Plan::DoorSpringing), EFormJoint::Crisp)
				.ArcAbout(FVector2D(C, Plan::DoorSpringing), W, W, 180.0, 0.0, EFormJoint::Smooth).LineTo(FVector2D(C + W, V0), EFormJoint::Smooth)
				.LineTo(FVector2D(U1, V0), EFormJoint::Crisp).LineTo(FVector2D(U1, V1), EFormJoint::Crisp).LineTo(FVector2D(U0, V1), EFormJoint::Crisp);
			return FaceSlab(S.Name, S.Surface, Face, Outline.Close(), {}, Plan::SilkBuildUp, Solid);
		}

		/** The silk of the five bays between their piers and round the end walls' doors, and of the cabinet. */
		void Silks(TArray<FFormDrawing>& Out)
		{
			const double M = Plan::MasonryHalf;
			const double Top = Plan::SilkTop;
			const double Stop = 0.5 * Plan::DoorWidth + Plan::LiningMargin;
			const double RoughStop = Stop + Plan::LiningAllowance;
			for (int32 Bay = 0; Bay < Plan::BayCount; ++Bay)
			{
				const Plan::FBay& B = Plan::Bays[Bay];
				const FString Colour = FString::Printf(TEXT("bay%d.silk"), Bay + 1);
				const FString Name = FString::Printf(TEXT("silk.bay%d"), Bay + 1);
				const FString NorthName = Name + TEXT(".north");
				FSilkStretch NorthSide{*NorthName, *Colour, FVector(B.West, -M, 0.0), FVector(B.East, -M, 0.0), FMuseeFrame::South(), Top};
				if (Bay == 0)
				{
					NorthSide.DoorAt = FVector(Plan::CabinetDoorX, -M, 0.0);
					NorthSide.DoorHalf = Stop;
				}
				Out.Add(Silk(NorthSide));
				const FString SouthName = Name + TEXT(".south");
				Out.Add(Silk({*SouthName, *Colour, FVector(B.West, M, 0.0), FVector(B.East, M, 0.0), FMuseeFrame::North(), Top}));
			}
			const double SilkHalf = Plan::SilkHalf;
			Out.Add(Silk({TEXT("silk.bay1.east"), TEXT("bay1.silk"), FVector(Plan::EndMasonryX, -SilkHalf, 0.0), FVector(Plan::EndMasonryX, SilkHalf, 0.0),
				FMuseeFrame::West(), Top, FVector(Plan::EndMasonryX, 0.0, 0.0), Stop}));
			Out.Add(Silk({TEXT("silk.bay5.west"), TEXT("bay5.silk"), FVector(Plan::FarMasonryX, -SilkHalf, 0.0), FVector(Plan::FarMasonryX, SilkHalf, 0.0),
				FMuseeFrame::East(), Top, FVector(Plan::FarMasonryX, 0.0, 0.0), RoughStop}));

			// The cabinet's four walls, its silk faces x -10.000 and -2.000, y -15.600 and -7.600, to its cornice's slot.
			const double CT = Plan::CabinetSilkTop;
			const double Build = Plan::SilkBuildUp;
			const FVector NW(Plan::CabinetWest - Build, Plan::CabinetNorth - Build, 0.0);
			const FVector NE(Plan::CabinetEast + Build, Plan::CabinetNorth - Build, 0.0);
			const FVector SW(Plan::CabinetWest - Build, Plan::CabinetSouth + Build, 0.0);
			const FVector SE(Plan::CabinetEast + Build, Plan::CabinetSouth + Build, 0.0);
			Out.Add(Silk({TEXT("silk.cabinet.north"), TEXT("cabinet.silk"), NW, NE, FMuseeFrame::South(), CT}));
			Out.Add(Silk({TEXT("silk.cabinet.east"), TEXT("cabinet.silk"), NE, SE, FMuseeFrame::West(), CT}));
			Out.Add(Silk({TEXT("silk.cabinet.west"), TEXT("cabinet.silk"), NW, SW, FMuseeFrame::East(), CT}));
			Out.Add(Silk({TEXT("silk.cabinet.south"), TEXT("cabinet.silk"), SW, SE, FMuseeFrame::North(), CT, FVector(Plan::CabinetDoorX, SW.Y, 0.0), Stop}));
		}

		/** The lunettes: the end and far walls above the springing, in plaster up behind the vault's face. */
		void Lunettes(TArray<FFormDrawing>& Out)
		{
			const double R = Plan::SpandrelOuter;
			FFormProfile HalfDisc = FFormProfile::From(FVector2D(R, 0.0));
			HalfDisc.ArcAbout(FVector2D::ZeroVector, R, R, 0.0, 180.0, EFormJoint::Crisp);
			HalfDisc.Close();
			Out.Add(FaceSlab(TEXT("lunette.east"), TEXT("lunette"), FaceFrame(FVector(Plan::EndMasonryX, 0.0, Plan::Springing), FMuseeFrame::West()), HalfDisc, {},
				Plan::SilkBuildUp, Solid));
			Out.Add(FaceSlab(TEXT("lunette.west"), TEXT("lunette"), FaceFrame(FVector(Plan::FarMasonryX, 0.0, Plan::Springing), FMuseeFrame::East()), HalfDisc, {},
				Plan::SilkBuildUp, Solid));
		}
	}

	void DrawWalls(TArray<FFormDrawing>& Out)
	{
		MasonryWalls(Out);
		Linings(Out);
		Silks(Out);
		Lunettes(Out);
	}
}
