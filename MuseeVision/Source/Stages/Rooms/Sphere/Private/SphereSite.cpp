#include "SphereParts.h"
#include "SpherePlan.h"
#include "SiteDescription.h"

/**
 * What the Sphere declares to the site and the systems (§1, §2): its footprint and the well it receives, its one
 * vertical port, the openings in its shells, the anchors the systems furnish, and its two enclosures with the pieces
 * that bound them.
 */
namespace Musee::Sphere
{
	namespace
	{
		/** The plan outline: the 96-gon round the drip ring, first corner at bearing 0, clockwise on the plan. */
		void Footprint(FSiteDescription& Out)
		{
			Out.Footprint.Kind = ESiteFootprintKind::Building;
			for (int32 Corner = 0; Corner < Plan::FootprintSides; ++Corner)
			{
				Out.Footprint.OutlineMeters.Add(AtBearing(Plan::FootprintCircumradius, 360.0 * Corner / Plan::FootprintSides));
			}
			Out.Footprint.BottomMeters = Plan::PortZ;
			Out.Footprint.TopMeters = Plan::FootprintTop;
		}

		/**
		 * The port: the plane z 7.715 on the axis, facing down (its x), with y east, where the throat's foot stands on the
		 * Atrium's collar. It meets atrium.up, whose collar iris closes it; the Sphere builds no closure on it. It receives
		 * the Atrium's well, the class's circle r 2.300 from the plane to the well's top, here its bounding prism.
		 */
		void Ports(FSiteDescription& Out)
		{
			FSitePort& Down = Out.Ports.AddDefaulted_GetRef();
			Down.Key = Key(TEXT("down"));
			Down.Kind = ESitePortKind::Shaft;
			Down.Class = FMuseeKey(Plan::PortClass);
			Down.Frame = FrameOfAxes(FVector(0.0, 0.0, Plan::PortZ), -FMuseeFrame::Up(), FMuseeFrame::East());
			Down.WallDepthMeters = Plan::NeckTop - Plan::PortZ;
			Down.Mates = FMuseeKey(Plan::PortMates);
			Down.bHasWell = true;
			Down.Well.bServing = false;
			Down.Well.MinMeters = FVector(-Plan::WellRadius, -Plan::WellRadius, Plan::PortZ);
			Down.Well.MaxMeters = FVector(Plan::WellRadius, Plan::WellRadius, Plan::WellTop);
		}

		FFormOpening& Opening(FSiteDescription& Out, const FString& Local, EFormOpeningKind Kind, const TCHAR* Enclosure, const FMuseeFrame& Frame,
			const FVector2D& Size)
		{
			FFormOpening& Made = Out.Openings.AddDefaulted_GetRef();
			Made.Key = Key(*Local);
			Made.Kind = Kind;
			Made.Enclosure = Key(Enclosure);
			Made.Frame = Frame;
			Made.SizeMeters = Size;
			return Made;
		}

		/**
		 * The openings. The Sphere has none to the sky: it shows the sky, never sees it. The port's rough opening is the
		 * throat's bore on the plane, looking down out of the neck. The irises' pockets are housings in the plane at their
		 * mid-height, spanning their plan; each dog's housing opens on the throat's bore, looking out along its bearing.
		 */
		void Openings(FSiteDescription& Out)
		{
			const FVector East = FMuseeFrame::East();
			Opening(Out, TEXT("down"), EFormOpeningKind::Port, TEXT("neck"), FrameOfAxes(FVector(0.0, 0.0, Plan::PortZ), East, FMuseeFrame::North()),
				FVector2D(2.0 * Plan::ThroatBore, 2.0 * Plan::ThroatBore));
			const FVector2D Pocket(2.0 * Plan::PocketOuter, 2.0 * Plan::PocketOuter);
			Opening(Out, TEXT("neck.iris.pocket"), EFormOpeningKind::Housing, TEXT("neck"),
				Raised(0.5 * (Plan::NeckPocketBottom + Plan::NeckPocketTop)), Pocket);
			Opening(Out, TEXT("platform.iris.pocket"), EFormOpeningKind::Housing, TEXT("interior"),
				Raised(0.5 * (Plan::PlatformPocketBottom + Plan::PlatformPocketTop)), Pocket);
			const double Floor = Plan::HousingBottom + Plan::HousingPlate;
			for (int32 Dog = 0; Dog < int32(UE_ARRAY_COUNT(Plan::DogBearings)); ++Dog)
			{
				const double Bearing = Plan::DogBearings[Dog];
				const FVector2D Mouth = AtBearing(Plan::ThroatBore, Bearing);
				Opening(Out, FString::Printf(TEXT("mast.dogs.housing.%d"), Dog + 1), EFormOpeningKind::Housing, TEXT("neck"),
					FrameOfAxes(FVector(Mouth.X, Mouth.Y, 0.5 * (Floor + Plan::Mouth)), Tangent(Bearing), FVector::UpVector),
					FVector2D(Plan::HousingInside, Plan::Mouth - Floor));
			}
		}

		void Anchor(FSiteDescription& Out, const FMuseeKey& Key, const TCHAR* Kind, const FMuseeFrame& Frame)
		{
			Out.Anchors.Add({Key, FName(Kind), Frame});
		}

		/**
		 * The anchors (§2), every body drawn closed: the mast's head and foot on the foot plane (the column at zero length,
		 * the foot ring at the car's stop mast), each dog at its retracted tip's top face with x along its slide, each
		 * blade at its pin on its top face; the Sphere's use of the car at the platform way's centre; and the image
		 * luminaire's mount at the centre C.
		 */
		void Anchors(FSiteDescription& Out)
		{
			const FMuseeFrame FootPlane(FVector(0.0, 0.0, Plan::Mouth));
			Anchor(Out, Key(TEXT("mast.head")), TEXT("Body"), FootPlane);
			Anchor(Out, Key(TEXT("mast.foot")), TEXT("Body"), FootPlane);
			for (int32 Dog = 0; Dog < int32(UE_ARRAY_COUNT(Plan::DogBearings)); ++Dog)
			{
				Anchor(Out, DogBody(Dog), TEXT("Body"), DogFrame(Dog));
			}
			for (int32 Blade = 0; Blade < Plan::IrisBlades; ++Blade)
			{
				Anchor(Out, IrisBladeBody(TEXT("neck"), Blade), TEXT("Body"), IrisPinFrame(Blade, Plan::NeckIrisTop));
			}
			for (int32 Blade = 0; Blade < Plan::IrisBlades; ++Blade)
			{
				Anchor(Out, IrisBladeBody(TEXT("platform"), Blade), TEXT("Body"), IrisPinFrame(Blade, Plan::PlatformIrisTop));
			}
			Anchor(Out, Key(TEXT("call")), TEXT("Use"), FMuseeFrame(FVector(0.0, 0.0, Plan::CallZ)));
			Anchor(Out, Key(TEXT("image.mount")), TEXT("Mount"), FMuseeFrame(FVector(0.0, 0.0, Plan::CentreZ)));
		}

		/**
		 * The enclosures. The interior is bounded by the image surface down to the throat, the throat's outer face, the
		 * flange, the dogs' housings and the shut neck iris. The neck, the airlock, is the throat's bore from the plane
		 * (closed by the Atrium's collar iris) up to the neck iris.
		 */
		void Enclosures(FSiteDescription& Out)
		{
			FFormEnclosure& Interior = Out.Enclosures.AddDefaulted_GetRef();
			Interior.Key = Key(TEXT("interior"));
			Interior.Kind = EFormEnclosureKind::Enclosed;
			Interior.Pieces = {Pieces::ImageSurface};
			Interior.Pieces.Append(Pieces::NeckWall());

			FFormEnclosure& Neck = Out.Enclosures.AddDefaulted_GetRef();
			Neck.Key = Key(TEXT("neck"));
			Neck.Kind = EFormEnclosureKind::Enclosed;
			Neck.Pieces = Pieces::NeckWall();
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
