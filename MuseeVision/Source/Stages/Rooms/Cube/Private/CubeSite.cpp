#include "Algo/AnyOf.h"
#include "CubeParts.h"
#include "SiteDescription.h"

/**
 * What the Cube declares to the site and the systems (§1, §2): the box's footprint, its one port with the well it
 * receives, the openings in its shells, the anchors the systems furnish, and its three enclosures with the pieces that
 * bound them.
 *
 * The received well is Site's box about the axis, the class's circle r 2.300 (its rail notches are Site's to make with
 * the class, KIT-2). A Mount's x is the way its panels face, into the room, as Light reads a mount's facing; the ceiling's
 * and the floor's keep the stage's y.
 */
namespace Musee::Cube
{
	namespace
	{
		void Footprint(FSiteDescription& Out)
		{
			using namespace Plan;
			Out.Footprint.Kind = ESiteFootprintKind::Building;
			Out.Footprint.OutlineMeters = {FVector2D(-FootprintHalf, -FootprintHalf), FVector2D(FootprintHalf, -FootprintHalf),
				FVector2D(FootprintHalf, FootprintHalf), FVector2D(-FootprintHalf, FootprintHalf)};
			Out.Footprint.BottomMeters = FootprintBottom;
			Out.Footprint.TopMeters = FootprintTop;
			FSitePrism& Box = Out.Footprint.Prisms.AddDefaulted_GetRef();
			Box.OutlineMeters = Out.Footprint.OutlineMeters;
			Box.BottomMeters = FootprintBottom;
			Box.TopMeters = RoofTop;
			FSitePrism& Column = Out.Footprint.Prisms.AddDefaulted_GetRef();
			for (int32 Side = 0; Side < ColumnSides; ++Side)
			{
				const double Angle = 2.0 * UE_DOUBLE_PI * Side / ColumnSides;
				Column.OutlineMeters.Add(ColumnCircumradius * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)));
			}
			Column.BottomMeters = RoofTop;
			Column.TopMeters = FootprintTop;
		}

		/**
		 * The port: the plane z -0.500 on the axis, facing up into the Atrium's slab, which meets atrium.down. Its depth is
		 * the 7.500 down to the ceiling. The Atrium serves the well; the Cube receives it and builds in it only the rails in
		 * the class's notches and its ceiling iris.
		 */
		void Ports(FSiteDescription& Out)
		{
			using namespace Plan;
			FSitePort& Up = Out.Ports.AddDefaulted_GetRef();
			Up.Key = Key(TEXT("up"));
			Up.Kind = ESitePortKind::Shaft;
			Up.Class = FMuseeKey(TEXT("iris.4600"));
			Up.Frame = FrameOfAxes(FVector(0.0, 0.0, MatingPlane), FVector::UpVector, FVector::ForwardVector);
			Up.WallDepthMeters = MatingPlane - Ceiling;
			Up.Mates = FMuseeKey(TEXT("atrium.down"));
			Up.bHasWell = true;
			Up.Well.bServing = false;
			Up.Well.MinMeters = FVector(-WellRadius, -WellRadius, WellBottom);
			Up.Well.MaxMeters = FVector(WellRadius, WellRadius, MatingPlane);
			// The ceiling iris is the class's closure of the well, built by the stage below its plane: its blades are the
			// class's iris (PortClasses.json, wellAdmits), and nothing else of the Cube's reaches into the well.
			Up.Well.Parts = {{Pieces::Blade, TEXT("iris")}};
		}

		FFormOpening& Opening(FSiteDescription& Out, const TCHAR* Local, EFormOpeningKind Kind, const TCHAR* Enclosure, const FMuseeFrame& Frame,
			const FVector2D& Size)
		{
			FFormOpening& Made = Out.Openings.AddDefaulted_GetRef();
			Made.Key = Key(Local);
			Made.Kind = Kind;
			Made.Enclosure = Key(Enclosure);
			Made.Frame = Frame;
			Made.SizeMeters = Size;
			return Made;
		}

		/**
		 * Form's one list: the port's rough opening at the top of the shaft, looking up; and the housing the ceiling iris
		 * moves and lies open in, round the shaft's foot. No Glazed or Open opening: no daylight and no sky (D-012).
		 */
		void Openings(FSiteDescription& Out)
		{
			using namespace Plan;
			const FVector2D Well(2.0 * WellRadius, 2.0 * WellRadius);
			Opening(Out, TEXT("up"), EFormOpeningKind::Port, TEXT("shaft"), FrameOfAxes(FVector(0.0, 0.0, MatingPlane), FVector::ForwardVector,
				FVector::RightVector), Well);
			const double Across = 2.0 * HousingOuter;
			Opening(Out, TEXT("up.iris.housing"), EFormOpeningKind::Housing, TEXT("shaft"),
				FMuseeFrame(FVector(0.0, 0.0, 0.5 * (SoffitRing[0][1] + HousingInnerWall[1]))), FVector2D(Across, Across));
		}

		void Anchor(FSiteDescription& Out, const FMuseeKey& AnchorKey, const TCHAR* Kind, const FMuseeFrame& Frame)
		{
			Out.Anchors.Add({AnchorKey, FName(Kind), Frame});
		}

		/**
		 * The anchors (§2): the seven blades, drawn shut, each at its pin on its top face; the work's control at the car's
		 * floor on the axis at the `cube` stop; and the six panel faces, each at its centre.
		 */
		void Anchors(FSiteDescription& Out)
		{
			using namespace Plan;
			for (int32 Blade = 0; Blade < IrisBlades; ++Blade)
			{
				Anchor(Out, BladeBody(Blade), TEXT("Body"), BladePin(Blade));
			}
			Anchor(Out, Key(TEXT("performance")), TEXT("Use"), FMuseeFrame(FVector(0.0, 0.0, CarAtCube)));

			const FVector X(1.0, 0.0, 0.0);
			const FVector Y(0.0, 1.0, 0.0);
			const FVector Z(0.0, 0.0, 1.0);
			Anchor(Out, Key(TEXT("panels.ceiling")), TEXT("Mount"), FrameOfAxes(FVector(0.0, 0.0, Ceiling), -Z, Y));
			Anchor(Out, Key(TEXT("panels.floor")), TEXT("Mount"), FrameOfAxes(FVector(0.0, 0.0, Floor), Z, Y));
			Anchor(Out, Key(TEXT("panels.north")), TEXT("Mount"), FrameOfAxes(FVector(0.0, -FaceHalf, EyesZ), Y, -X));
			Anchor(Out, Key(TEXT("panels.south")), TEXT("Mount"), FrameOfAxes(FVector(0.0, FaceHalf, EyesZ), -Y, X));
			Anchor(Out, Key(TEXT("panels.east")), TEXT("Mount"), FrameOfAxes(FVector(FaceHalf, 0.0, EyesZ), -X, -Y));
			Anchor(Out, Key(TEXT("panels.west")), TEXT("Mount"), FrameOfAxes(FVector(-FaceHalf, 0.0, EyesZ), X, Y));
		}

		/** The drawn pieces whose names start with any of the prefixes. */
		TArray<FName> Bounding(const FSiteDescription& Out, TConstArrayView<const TCHAR*> Prefixes)
		{
			TArray<FName> Names;
			for (const FFormDrawing& Piece : Out.Pieces)
			{
				const FString Name = Piece.Name.ToString();
				if (Algo::AnyOf(Prefixes, [&Name](const TCHAR* Prefix) { return Name.StartsWith(Prefix, ESearchCase::CaseSensitive); }))
				{
					Names.Add(Piece.Name);
				}
			}
			return Names;
		}

		/**
		 * The room: the panel faces, and at the ceiling's opening the soffit ring and the ceiling iris. The shaft: its bore
		 * from the iris up, the housing's inner wall and roof, the sleeve, the gilt frames and the glass between them. The
		 * cavity: the annulus between the glass and the ground's cut face, closed by the head and foot plates and crossed by
		 * the webs.
		 */
		void Enclosures(FSiteDescription& Out)
		{
			struct FSpace
			{
				const TCHAR* Local;
				TArray<const TCHAR*> Prefixes;
			};
			const FSpace Spaces[] = {
				{TEXT("room"), {Pieces::Panel, Pieces::SoffitRing, Pieces::Blade}},
				{TEXT("shaft"), {Pieces::HousingInner, Pieces::HousingRoof, Pieces::Sleeve, Pieces::Frame, Pieces::Glass, Pieces::Blade}},
				{TEXT("shaft.cavity"), {Pieces::Glass, Pieces::Plate, Pieces::Web, Pieces::Ground}},
			};
			for (const FSpace& Space : Spaces)
			{
				FFormEnclosure& Enclosure = Out.Enclosures.AddDefaulted_GetRef();
				Enclosure.Key = Key(Space.Local);
				Enclosure.Kind = EFormEnclosureKind::Enclosed;
				Enclosure.Pieces = Bounding(Out, Space.Prefixes);
			}
		}
	}

	void DescribeSite(FSiteDescription& Out)
	{
		Footprint(Out);
		Ports(Out);
		Openings(Out);
		Anchors(Out);
		Enclosures(Out);
	}
}
