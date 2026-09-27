#include "Tests/SpecimenRoom.h"

#include "MuseeCollision.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpecimenRoom)

namespace
{
	/**
	 * The specimen's plan, metres: an 8 m by 8 m room east of its west wall's face (x 0) and south of its north wall's
	 * face (y -4), both walls 0.6 m thick and 6 m high on a 0.3 m slab; the door is the museum's standard arched doorway
	 * (doorway.3000x4800.arched) on the room's east-west axis.
	 */
	constexpr double RoomMeters = 8.0;
	constexpr double WallMeters = 0.6;
	constexpr double WallHeightMeters = 6.0;
	constexpr double SlabMeters = 0.3;
	constexpr double DoorWidthMeters = 3.0;
	constexpr double DoorSpringingMeters = 3.3;

	/** The post: a 0.15 m radius drum 1 m tall on the body 2 m in from the corner. */
	constexpr double PostRadiusMeters = 0.15;
	constexpr double PostHeightMeters = 1.0;
	const FVector PlinthMeters(2.0, -2.0, 0.0);

	FFormDrawing Wall(const TCHAR* Name, const FVector2D& From, const FVector2D& To, TArray<FFormWallOpening> Openings)
	{
		FFormWall Shape;
		Shape.Plan = FFormProfile::Polyline({From, To}, false);
		Shape.ThicknessMeters = WallMeters;
		Shape.HeightMeters = WallHeightMeters;
		Shape.Openings = MoveTemp(Openings);
		FFormDrawing Drawing = FFormDrawing::Of(Name, TEXT("wall"), MoveTemp(Shape));
		Drawing.PartSurfaces.Add(FFormWall::BackPart, TEXT("masonry"));
		Drawing.PartSurfaces.Add(FFormWall::RevealPart, TEXT("masonry"));
		Drawing.Collision = Musee::Collision::Solid.Profile();
		return Drawing;
	}
}

FMuseeKey USpecimenRoom::StageKey() const
{
	return FMuseeKey(TEXT("specimen-room"));
}

void USpecimenRoom::Describe(FSiteDescription& Out) const
{
	const double Half = 0.5 * RoomMeters;
	Out.Stage = StageKey();
	Out.DisplayName = NSLOCTEXT("MuseeStudio", "SpecimenRoom", "Studio's specimen room");
	Out.Footprint.OutlineMeters = {{-WallMeters, -Half - WallMeters}, {RoomMeters, -Half - WallMeters}, {RoomMeters, Half}, {-WallMeters, Half}};
	Out.Footprint.BottomMeters = -SlabMeters;
	Out.Footprint.TopMeters = WallHeightMeters;

	// The west wall runs south along x 0, so its thickness lies west; the north wall runs west along y -4, so its lies north.
	FFormWallOpening Door;
	Door.CenterMeters = Half;
	Door.WidthMeters = DoorWidthMeters;
	Door.HeadMeters = DoorSpringingMeters;
	Door.bArched = true;
	Out.Pieces.Add(Wall(TEXT("wall.west"), {0.0, -Half}, {0.0, Half}, {Door}));
	Out.Pieces.Add(Wall(TEXT("wall.north"), {RoomMeters, -Half}, {0.0, -Half}, {}));

	FFormExtrusion Slab;
	Slab.Outline = FFormProfile::Rectangle({-WallMeters, -Half - WallMeters}, {RoomMeters, Half});
	Slab.DepthMeters = SlabMeters;
	FFormDrawing& Floor = Out.Pieces.Add_GetRef(FFormDrawing::Of(TEXT("floor"), TEXT("floor"), MoveTemp(Slab)));
	Floor.Frame = FMuseeFrame(FVector(0.0, 0.0, -SlabMeters));
	Floor.Collision = Musee::Collision::Walkable.Profile();

	FFormLathe Drum;
	Drum.Section = FFormProfile::From({0.0, 0.0});
	Drum.Section.LineTo({PostRadiusMeters, 0.0}, EFormJoint::Crisp).LineTo({PostRadiusMeters, PostHeightMeters}, EFormJoint::Crisp)
		.LineTo({0.0, PostHeightMeters}, EFormJoint::Crisp);
	FFormDrawing& Post = Out.Pieces.Add_GetRef(FFormDrawing::Of(TEXT("post"), TEXT("post"), MoveTemp(Drum)));
	Post.Body = FMuseeKey(TEXT("specimen-room.plinth"));
	Post.Collision = Musee::Collision::Solid.Profile();
	Out.Anchors.Add({FMuseeKey(TEXT("specimen-room.plinth")), TEXT("Body"), FMuseeFrame(PlinthMeters)});

	FSitePort& West = Out.Ports.AddDefaulted_GetRef();
	West.Key = FMuseeKey(TEXT("specimen-room.west"));
	West.Kind = ESitePortKind::Doorway;
	West.Class = FMuseeKey(TEXT("doorway.3000x4800.arched"));
	West.Frame = FMuseeFrame(FVector(-WallMeters, 0.0, 0.0), FRotator(0.0, 180.0, 0.0).Quaternion());
	West.WallDepthMeters = WallMeters;

	FFormEnclosure& Room = Out.Enclosures.AddDefaulted_GetRef();
	Room.Key = FMuseeKey(TEXT("specimen-room.room"));
	Room.Pieces = {TEXT("wall.west"), TEXT("wall.north"), TEXT("floor")};
}
