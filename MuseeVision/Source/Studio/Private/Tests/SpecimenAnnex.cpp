#include "Tests/SpecimenAnnex.h"

#include "MuseeCollision.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpecimenAnnex)

namespace Musee::Studio::Tests::AnnexPlan
{
	/**
	 * The annex's plan, metres: its east wall's outer face on x 0, 0.6 m thick to the west and 6 m high, 8 m long about
	 * the door, over a 0.3 m slab reaching 4 m west; the door is the specimen room's (doorway.3000x4800.arched).
	 */
	constexpr double LengthMeters = 8.0;
	constexpr double DepthMeters = 4.0;
	constexpr double WallMeters = 0.6;
	constexpr double WallHeightMeters = 6.0;
	constexpr double SlabMeters = 0.3;
	constexpr double DoorWidthMeters = 3.0;
	constexpr double DoorSpringingMeters = 3.3;
}

FMuseeKey USpecimenAnnex::StageKey() const
{
	return FMuseeKey(TEXT("specimen-annex"));
}

void USpecimenAnnex::Describe(FSiteDescription& Out) const
{
	namespace AnnexPlan = Musee::Studio::Tests::AnnexPlan;
	const double Half = 0.5 * AnnexPlan::LengthMeters;
	Out.Stage = StageKey();
	Out.DisplayName = NSLOCTEXT("MuseeStudio", "SpecimenAnnex", "Studio's specimen annex");
	Out.Footprint.OutlineMeters = {{-AnnexPlan::DepthMeters, -Half}, {0.0, -Half}, {0.0, Half}, {-AnnexPlan::DepthMeters, Half}};
	Out.Footprint.BottomMeters = -AnnexPlan::SlabMeters;
	Out.Footprint.TopMeters = AnnexPlan::WallHeightMeters;

	// The east wall runs south along x 0, so its thickness lies west, inside the annex.
	FFormWallOpening Door;
	Door.CenterMeters = Half;
	Door.WidthMeters = AnnexPlan::DoorWidthMeters;
	Door.HeadMeters = AnnexPlan::DoorSpringingMeters;
	Door.bArched = true;
	FFormWall Wall;
	Wall.Plan = FFormProfile::Polyline({{0.0, -Half}, {0.0, Half}}, false);
	Wall.ThicknessMeters = AnnexPlan::WallMeters;
	Wall.HeightMeters = AnnexPlan::WallHeightMeters;
	Wall.Openings.Add(Door);
	FFormDrawing& East = Out.Pieces.Add_GetRef(FFormDrawing::Of(TEXT("wall.east"), TEXT("masonry"), MoveTemp(Wall)));
	East.Collision = Musee::Collision::Solid.Profile();

	FFormExtrusion Slab;
	Slab.Outline = FFormProfile::Rectangle({-AnnexPlan::DepthMeters, -Half}, {0.0, Half});
	Slab.DepthMeters = AnnexPlan::SlabMeters;
	FFormDrawing& Floor = Out.Pieces.Add_GetRef(FFormDrawing::Of(TEXT("floor"), TEXT("floor"), MoveTemp(Slab)));
	Floor.Frame = FMuseeFrame(FVector(0.0, 0.0, -AnnexPlan::SlabMeters));
	Floor.Collision = Musee::Collision::Walkable.Profile();

	FSitePort& EastPort = Out.Ports.AddDefaulted_GetRef();
	EastPort.Key = FMuseeKey(TEXT("specimen-annex.east"));
	EastPort.Kind = ESitePortKind::Doorway;
	EastPort.Class = FMuseeKey(TEXT("doorway.3000x4800.arched"));
	EastPort.Frame = FMuseeFrame(FVector::ZeroVector);
	EastPort.WallDepthMeters = AnnexPlan::WallMeters;
	EastPort.Mates = FMuseeKey(TEXT("specimen-room.west"));

	FFormEnclosure& Room = Out.Enclosures.AddDefaulted_GetRef();
	Room.Key = FMuseeKey(TEXT("specimen-annex.room"));
	Room.Pieces = {TEXT("wall.east"), TEXT("floor")};
}
