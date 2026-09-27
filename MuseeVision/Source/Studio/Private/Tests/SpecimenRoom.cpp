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

	/** The post: a 0.15 m radius drum 1 m tall on the body 2 m in from the corner, with a cap on a body riding the plinth. */
	constexpr double PostRadiusMeters = 0.15;
	constexpr double PostHeightMeters = 1.0;
	const FVector PlinthMeters(2.0, -2.0, 0.0);
	const FVector CapMeters(2.0, -2.0, PostHeightMeters);

	/** The knob that turns the plinth: on the post's east face, at a hand's height, so it rides the plinth. */
	const FVector KnobMeters = PlinthMeters + FVector(PostRadiusMeters, 0.0, 0.9);

	/** Two identical studs on the floor, 2 m either side of the room's axis, cast once and placed twice. */
	const FVector StudMeters[] = {FVector(6.0, -2.0, 0.0), FVector(6.0, 2.0, 0.0)};

	/** A drum of a radius and height standing on its frame's origin. */
	FFormLathe Drum(double Radius, double Height)
	{
		FFormLathe Shape;
		Shape.Section = FFormProfile::From({0.0, 0.0});
		Shape.Section.LineTo({Radius, 0.0}, EFormJoint::Crisp).LineTo({Radius, Height}, EFormJoint::Crisp).LineTo({0.0, Height}, EFormJoint::Crisp);
		return Shape;
	}

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

	const FMuseeKey Plinth(TEXT("specimen-room.plinth"));
	FFormDrawing& Post = Out.Pieces.Add_GetRef(FFormDrawing::Of(TEXT("post"), TEXT("post"), Drum(PostRadiusMeters, PostHeightMeters)));
	Post.Body = Plinth;
	Post.Collision = Musee::Collision::Solid.Profile();
	Out.Anchors.Add({Plinth, TEXT("Body"), FMuseeFrame(PlinthMeters)});
	Out.Anchors.Add({FMuseeKey(TEXT("specimen-room.plinth.knob")), TEXT("Use"), FMuseeFrame(KnobMeters)});

	// The cap rides the plinth: its body is declared where it is drawn, on the post's top, and hangs from the plinth's.
	const FMuseeKey Cap(TEXT("specimen-room.plinth.cap"));
	FFormDrawing& Top = Out.Pieces.Add_GetRef(FFormDrawing::Of(TEXT("cap"), TEXT("post"), Drum(PostRadiusMeters, 0.5 * PostRadiusMeters)));
	Top.Body = Cap;
	Top.Collision = Musee::Collision::Solid.Profile();
	Out.Anchors.Add({Cap, TEXT("Body"), FMuseeFrame(CapMeters)});
	Out.BodyParents.Add(Cap, Plinth);

	// A barrier keeps the visitor 0.25 m off the post, 1.2 m high, and lets the eye through.
	Out.Barriers.Add({FMuseeKey(TEXT("specimen-room.plinth.barrier")), FMuseeFrame(PlinthMeters + FVector(0.0, 0.0, 0.6)), FVector(0.4, 0.4, 0.6)});

	for (int32 Stud = 0; Stud < UE_ARRAY_COUNT(StudMeters); ++Stud)
	{
		FFormDrawing& Placed = Out.Pieces.Add_GetRef(FFormDrawing::Of(FName(FString::Printf(TEXT("stud.%c"), TCHAR('a' + Stud))), TEXT("post"),
			Drum(PostRadiusMeters, PostRadiusMeters)));
		Placed.Frame = FMuseeFrame(StudMeters[Stud]);
		Placed.Collision = Musee::Collision::Barrier.Profile();
	}

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
