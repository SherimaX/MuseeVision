#include "ArtHang.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "Misc/AutomationTest.h"
#include "MuseeRecords.h"
#include "MuseeUnits.h"
#include "Vessel/VesselDecoration.h"
#include "Vessel/VesselMesh.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace Musee::ArtMaking::VesselTests
{
	/** Art's making tests run in the editor, where its making twin lives, and belong to the product's own filter. */
	inline constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	/** How near a turned vessel's height and diameter come to its museum's: half a millimetre. */
	constexpr double SizeToleranceMeters = 0.0005;

	/** A plain cylinder 0.1 m in radius and 0.2 m tall, its outside the wall from its foot's edge to its rim's. */
	FArtVessel Cylinder()
	{
		FArtVessel Vessel;
		Vessel.Work = TEXT("cylinder");
		Vessel.Section = { FVector2D(0.0, 0.0), FVector2D(0.1, 0.0), FVector2D(0.1, 0.2), FVector2D(0.0, 0.2) };
		Vessel.Outside = { 1, 2 };
		return Vessel;
	}

	/** A composition as the record's: views lend from 60 degrees off face on. */
	FArtVesselComposition Composition()
	{
		FArtVesselComposition How;
		How.SquarenessCut = 0.5;
		How.WhiteSquareness = 0.8;
		How.WhiteQuantile = 0.9;
		How.GlazeChroma = 0.25;
		How.GlazeLightness = 0.4;
		How.ShadeFloor = 0.2;
		How.HighlightOverWhite = 1.2;
		How.HighlightWeight = 0.05;
		How.FullWeight = 0.04;
		How.MostPixelsPerMeter = 1000.0;
		How.MostPixels = 2048;
		return How;
	}

	/**
	 * A photograph of the cylinder from far off, level with its middle, at a bearing: a white ground, and a red stripe
	 * down its middle column, which is the cylinder's axis.
	 */
	FVesselPicture StripedView(double AzimuthDegrees)
	{
		FVesselPicture View;
		View.Image.Init(1000, 1000, 1, ERawImageFormat::BGRA8, EGammaSpace::sRGB);
		const TArrayView64<FColor> Pixels = View.Image.AsBGRA8();
		for (int32 Y = 0; Y < 1000; ++Y)
		{
			for (int32 X = 0; X < 1000; ++X)
			{
				Pixels[int64(Y) * 1000 + X] = FMath::Abs(X - 499) <= 4 ? FColor(220, 20, 20) : FColor(240, 240, 240);
			}
		}
		View.Camera.AzimuthRadians = FMath::DegreesToRadians(AzimuthDegrees);
		View.Camera.DistanceMeters = 10.0;
		View.Camera.HeightMeters = 0.1;
		View.Camera.FocalPixels = 40000.0;
		View.Camera.AxisPixels = FVector2D(500.0, 500.0);
		return View;
	}

	bool IsRed(const FColor& C)
	{
		return C.R > 150 && C.G < 120 && C.B < 120;
	}

	/** The column of a composed row reddest along it, and the angle it stands at (degrees from the front towards the right). */
	double ReddestAngle(const FImage& Image, int32 Row)
	{
		const TArrayView64<const FColor> Pixels = Image.AsBGRA8();
		int32 Best = 0;
		int32 BestRed = -1000;
		for (int32 X = 0; X < Image.SizeX; ++X)
		{
			const FColor& C = Pixels[int64(Row) * Image.SizeX + X];
			const int32 Red = int32(C.R) - int32(C.G);
			if (Red > BestRed)
			{
				BestRed = Red;
				Best = X;
			}
		}
		return 360.0 * (Best + 0.5) / Image.SizeX;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtPaintedVesselTest, "Musee.Art.PaintedVessel", Musee::ArtMaking::VesselTests::Flags)

bool FArtPaintedVesselTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	using namespace Musee::ArtMaking::VesselTests;

	// The museum's vessels, as Chenghuai's plan shows them: each at its museum's height and, where published, diameter.
	const TValueOrError<FArtVessels, FString> Record = Musee::ReadRecord<FArtVessels>(FArtVessels::Record);
	const TValueOrError<FArtHang, FString> Hang = Musee::ReadRecord<FArtHang>(TEXT("Stages/chenghuai/Hang.json"));
	if (!TestTrue(TEXT("The vessels' record and Chenghuai's plan read"), Record.HasValue() && Hang.HasValue()))
	{
		return false;
	}
	int32 Derived = 0;
	for (const FArtVessel& Vessel : Record.GetValue().Vessels)
	{
		if (Vessel.Grade != TEXT("derived"))
		{
			continue;
		}
		++Derived;
		const FArtHangEntry* Entry = Hang.GetValue().Works.FindByPredicate([&Vessel](const FArtHangEntry& E) { return E.Work == Vessel.Work; });
		if (!TestNotNull(FString::Printf(TEXT("%s is in the plan"), *Vessel.Work), Entry))
		{
			continue;
		}
		const TArray<FVector2D> Sized = SizedSection(Vessel, Entry->SizeMeters());
		FBox2D Box(ForceInit);
		for (const FVector2D& Point : Sized)
		{
			Box += Point;
		}
		// Its foot, or its feet, stand on the plane at 0.
		TestTrue(FString::Printf(TEXT("%s stands its museum's %.1f cm"), *Vessel.Work, 100.0 * Vessel.Museum.HeightMeters),
			FMath::IsNearlyEqual(Box.Max.Y, Vessel.Museum.HeightMeters, SizeToleranceMeters) && Box.Min.Y >= 0.0
			&& (FMath::IsNearlyZero(Box.Min.Y) || !Vessel.Feet.Positions.IsEmpty()));
		const double Across = Vessel.Museum.IsOval() ? Vessel.Museum.MouthAxesMeters[0] : Vessel.Museum.DiameterMeters;
		if (Across > 0.0)
		{
			TestTrue(FString::Printf(TEXT("%s is its museum's %.1f cm across"), *Vessel.Work, 100.0 * Across),
				FMath::IsNearlyEqual(2.0 * Box.Max.X, Across, SizeToleranceMeters));
		}
		for (const FArtVesselBand& Band : Vessel.Bands)
		{
			TestTrue(FString::Printf(TEXT("%s's %s repeats a whole number of times round"), *Vessel.Work, *Band.Name), Band.PeriodDegrees > 0.0
				&& FMath::IsNearlyEqual(360.0 / Band.PeriodDegrees, FMath::RoundToDouble(360.0 / Band.PeriodDegrees), 1e-6));
		}
	}
	TestEqual(TEXT("Seven vessels are derived from their photographs"), Derived, 7);

	// A view's painting lands where its camera saw it, and nowhere its camera saw only askance.
	const FArtVessel Vessel = Cylinder();
	const FArtVesselComposition How = Composition();
	const TArray<FVector2D> Section = Vessel.Section;
	const FVesselLayout Layout = LayoutOf(Section);
	const FVesselPicture Views[] = { StripedView(40.0) };
	FImage Decoration;
	FDecorationStats Stats;
	ComposeDecoration(How, Vessel, Section, Views, DecorationPixels(How, Vessel, Layout), Decoration, Stats);
	// The wall's middle, 0.1 m up it, is 0.2 m along the section (0.1 across the foot, then 0.1 up).
	const int32 Middle = FMath::FloorToInt32(0.2 / Layout.SizeMeters().Y * Decoration.SizeY);
	TestTrue(TEXT("The stripe lands on the side its camera faced, 40 degrees round"), FMath::Abs(ReddestAngle(Decoration, Middle) - 40.0) < 1.0);
	const TArrayView64<const FColor> Pixels = Decoration.AsBGRA8();
	auto AtAngle = [&](double Degrees) { return Pixels[int64(Middle) * Decoration.SizeX + FMath::FloorToInt32(Degrees / 360.0 * Decoration.SizeX)]; };
	TestFalse(TEXT("It doesn't land on the far side"), IsRed(AtAngle(220.0)));
	TestTrue(TEXT("Where the view saw the wall 75 degrees askance it lends nothing: the glaze fills in"), AtAngle(115.0).R > 200 && AtAngle(115.0).G > 200);
	TestTrue(TEXT("No view is stretched past its cut"), Stats.MostStretch <= 1.0 / How.SquarenessCut + 1e-9);
	TestTrue(TEXT("The view gave some of the outside whole, and not all of it"), Stats.Seen > 0.05 && Stats.Seen < 0.5);

	// The turned mesh carries the same layout: a point of its wall 40 degrees round has the stripe's U.
	UE::Geometry::FDynamicMesh3 Mesh;
	BuildVessel(Section, Layout, FVesselTurning(), Mesh);
	const UE::Geometry::FDynamicMeshUVOverlay* UVs = Mesh.Attributes()->PrimaryUV();
	int32 Checked = 0;
	int32 Agreed = 0;
	for (const int32 Triangle : Mesh.TriangleIndicesItr())
	{
		const UE::Geometry::FIndex3i Corners = Mesh.GetTriangle(Triangle);
		const UE::Geometry::FIndex3i Elements = UVs->GetTriangle(Triangle);
		for (int32 K = 0; K < 3; ++K)
		{
			const FVector Point = Musee::FromEngine(FVector(Mesh.GetVertex(Corners[K])));
			const FVector2f UV = UVs->GetElement(Elements[K]);
			const double Radius = FVector2D(Point.X, Point.Y).Size();
			// The wall's rings, not the base's or the rim's centres on the axis.
			if (Radius < 0.09)
			{
				continue;
			}
			// Its angle from the front (+y) towards the right (+x), and the one its U says.
			const double Angle = FMath::Fmod(FMath::Atan2(Point.X, Point.Y) + 2.0 * UE_DOUBLE_PI, 2.0 * UE_DOUBLE_PI);
			const double FromU = UV.X / Layout.ReferenceRadius;
			const double Apart = FMath::Abs(FMath::Fmod(Angle - FromU + 3.0 * UE_DOUBLE_PI, 2.0 * UE_DOUBLE_PI) - UE_DOUBLE_PI);
			++Checked;
			Agreed += Apart < 1e-3 ? 1 : 0;
		}
	}
	TestTrue(TEXT("Every wall point's U is its angle round from the front"), Checked > 0 && Agreed == Checked);
	return true;
}

#endif
