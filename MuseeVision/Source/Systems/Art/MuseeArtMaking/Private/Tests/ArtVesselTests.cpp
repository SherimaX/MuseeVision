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

	/** The test's photograph: square, its camera looking at the cylinder's axis from far off, level with its middle. */
	constexpr int32 PhotoPixels = 1000;
	constexpr double CameraDistanceMeters = 10.0;
	constexpr double CameraFocalPixels = 40000.0;
	constexpr double CameraBearingDegrees = 40.0;
	constexpr double CylinderRadiusMeters = 0.1;

	/** The two stripes painted on the photograph: one down the axis, one where the camera sees the wall 30 degrees round. */
	constexpr double AskanceDegrees = 30.0;
	constexpr double AxisStripePixels = 41.0;
	constexpr double AskanceStripePixels = 21.0;

	/** A plain cylinder 0.1 m in radius and 0.2 m tall, its outside the wall from its foot's edge to its rim's, and its one view. */
	FArtVessel Cylinder()
	{
		FArtVessel Vessel;
		Vessel.Work = TEXT("cylinder");
		Vessel.Section = { FVector2D(0.0, 0.0), FVector2D(CylinderRadiusMeters, 0.0), FVector2D(CylinderRadiusMeters, 0.2), FVector2D(0.0, 0.2) };
		Vessel.Outside = { 1, 2 };
		FArtVesselView View;
		View.Image = TEXT("assets/ceramics/test/striped.png");
		View.AzimuthDegrees = CameraBearingDegrees;
		View.DistanceMeters = CameraDistanceMeters;
		View.HeightMeters = 0.1;
		View.FocalPixels = CameraFocalPixels;
		View.AxisPixels = { 0.5 * PhotoPixels, 0.5 * PhotoPixels };
		View.Placement = TEXT("fitted");
		Vessel.Views.Add(View);
		return Vessel;
	}

	/** A composition as the record's: views lend from 60 degrees off face on; 1000 pixels a metre at the finest. */
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

	/** Where the wall at so many degrees round from the camera's bearing falls across the photograph, pixels from its centre. */
	double ColumnOffset(double DegreesRound)
	{
		const double Round = FMath::DegreesToRadians(DegreesRound);
		return CameraFocalPixels * CylinderRadiusMeters * FMath::Sin(Round) / (CameraDistanceMeters - CylinderRadiusMeters * FMath::Cos(Round));
	}

	/**
	 * The cylinder's photograph through its view's camera: a white ground, a red stripe down its axis and another where
	 * the camera sees the wall AskanceDegrees round to its right.
	 */
	FVesselPicture StripedView(const FArtVessel& Vessel)
	{
		FVesselPicture View;
		View.Image.Init(PhotoPixels, PhotoPixels, 1, ERawImageFormat::BGRA8, EGammaSpace::sRGB);
		View.Camera = CameraOf(Vessel.Views[0]);
		const TArrayView64<FColor> Pixels = View.Image.AsBGRA8();
		const double Centre = 0.5 * PhotoPixels;
		const double Askance = Centre + ColumnOffset(AskanceDegrees);
		for (int32 Y = 0; Y < PhotoPixels; ++Y)
		{
			for (int32 X = 0; X < PhotoPixels; ++X)
			{
				const double Column = X + 0.5;
				const bool bRed = FMath::Abs(Column - Centre) < 0.5 * AxisStripePixels || FMath::Abs(Column - Askance) < 0.5 * AskanceStripePixels;
				Pixels[int64(Y) * PhotoPixels + X] = bRed ? FColor(220, 20, 20) : FColor(240, 240, 240);
			}
		}
		return View;
	}

	bool IsRed(const FColor& C)
	{
		return C.R > 150 && C.G < 120 && C.B < 120;
	}

	/** A red run along a composed row: its middle and its width, in degrees round from the front towards the right. */
	struct FRun
	{
		double MiddleDegrees = 0.0;
		double WidthDegrees = 0.0;
	};

	TArray<FRun> RedRuns(const FImage& Image, int32 Row)
	{
		const TArrayView64<const FColor> Pixels = Image.AsBGRA8();
		TArray<FRun> Out;
		int32 Start = INDEX_NONE;
		for (int32 X = 0; X <= Image.SizeX; ++X)
		{
			const bool bRed = X < Image.SizeX && IsRed(Pixels[int64(Row) * Image.SizeX + X]);
			if (bRed && Start == INDEX_NONE)
			{
				Start = X;
			}
			else if (!bRed && Start != INDEX_NONE)
			{
				Out.Add({ 360.0 * 0.5 * (Start + X) / Image.SizeX, 360.0 * (X - Start) / Image.SizeX });
				Start = INDEX_NONE;
			}
		}
		return Out;
	}

	/** The angle round the wall a stripe of so many pixels spans where the camera sees the wall so many degrees round. */
	double StripeDegrees(double Pixels, double DegreesRound)
	{
		const double Half = 0.5 * Pixels;
		const double Offset = ColumnOffset(DegreesRound);
		// the wall's angles at the stripe's two edges, found by bisection on the (monotonic) column offset
		auto AngleAt = [](double Target)
		{
			double Low = -80.0;
			double High = 80.0;
			for (int32 Step = 0; Step < 60; ++Step)
			{
				const double Mid = 0.5 * (Low + High);
				(ColumnOffset(Mid) < Target ? Low : High) = Mid;
			}
			return 0.5 * (Low + High);
		};
		return AngleAt(Offset + Half) - AngleAt(Offset - Half);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtPaintedVesselTest, "Musee.Art.PaintedVessel", Musee::ArtMaking::VesselTests::Flags)

bool FArtPaintedVesselTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	using namespace Musee::ArtMaking::VesselTests;

	// The museum's vessels, as Chenghuai's plan shows them: each at its museum's height and, where published, diameter;
	// each section a solid, its wall nowhere thinner than half the wall it was drawn with.
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
			&& (FMath::IsNearlyZero(Box.Min.Y) || !Vessel.Feet.Each.IsEmpty()));
		const double Across = Vessel.Museum.IsOval() ? Vessel.Museum.MouthAxesMeters[0] : Vessel.Museum.DiameterMeters;
		if (Across > 0.0)
		{
			TestTrue(FString::Printf(TEXT("%s is its museum's %.1f cm across"), *Vessel.Work, 100.0 * Across),
				FMath::IsNearlyEqual(2.0 * Box.Max.X, Across, SizeToleranceMeters));
		}
		const FString Solid = SectionFault(Vessel.Section, Vessel.Outside, Vessel.WallMeters);
		TestTrue(FString::Printf(TEXT("%s's section is a solid with a wall (%s)"), *Vessel.Work, *Solid), Solid.IsEmpty() && Vessel.WallMeters > 0.0);
		for (const FArtVesselFoot& Foot : Vessel.Feet.Each)
		{
			TestTrue(FString::Printf(TEXT("%s's feet are outlines"), *Vessel.Work), Foot.Outline.Num() >= 3);
		}
		for (const FArtVesselBand& Band : Vessel.Bands)
		{
			TestTrue(FString::Printf(TEXT("%s's %s repeats a whole number of times round"), *Vessel.Work, *Band.Name), Band.PeriodDegrees > 0.0
				&& FMath::IsNearlyEqual(360.0 / Band.PeriodDegrees, FMath::RoundToDouble(360.0 / Band.PeriodDegrees), 1e-6));
		}
		for (int32 Index = 0; Index < Vessel.Views.Num(); ++Index)
		{
			const FArtVesselView& View = Vessel.Views[Index];
			TestTrue(FString::Printf(TEXT("%s's view %s is the one its section was fitted to, or placed after it"), *Vessel.Work, *View.Image),
				Index == 0 ? View.Placement == TEXT("fitted") && View.AzimuthDegrees == 0.0 : FArtVesselView::IsPlacement(View.Placement) && View.Placement != TEXT("fitted"));
		}
	}
	TestEqual(TEXT("Seven vessels are derived from their photographs"), Derived, 7);

	// A turned section that crosses itself, or whose inside comes nearer its outside than half its wall, is no solid.
	{
		const TArray<FVector2D> Folded = { FVector2D(0.0, 0.0), FVector2D(0.1, 0.0), FVector2D(0.1, 0.2), FVector2D(0.09, 0.19), FVector2D(0.11, 0.1),
			FVector2D(0.0, 0.1) };
		TestFalse(TEXT("A section whose inside crosses its outside is no solid"), SectionFault(Folded, { 1, 2 }, 0.005).IsEmpty());
		const TArray<FVector2D> Thin = { FVector2D(0.0, 0.0), FVector2D(0.1, 0.0), FVector2D(0.1, 0.2), FVector2D(0.099, 0.2), FVector2D(0.099, 0.05),
			FVector2D(0.0, 0.05) };
		TestFalse(TEXT("A section whose wall is thinner than half the wall it was drawn with is no solid"), SectionFault(Thin, { 1, 2 }, 0.005).IsEmpty());
		const TArray<FVector2D> Sound = { FVector2D(0.0, 0.0), FVector2D(0.1, 0.0), FVector2D(0.1, 0.2), FVector2D(0.095, 0.2), FVector2D(0.095, 0.005),
			FVector2D(0.0, 0.005) };
		TestTrue(TEXT("A section with its wall is a solid"), SectionFault(Sound, { 1, 2 }, 0.005).IsEmpty());
	}

	// A view's painting lands where its camera saw it, at the width it has on the vessel, and nowhere its camera saw only
	// askance.
	const FArtVessel Vessel = Cylinder();
	const FArtVesselComposition How = Composition();
	const TArray<FVector2D> Section = Vessel.Section;
	const FVesselLayout Layout = LayoutOf(Section);
	const FVesselPicture Views[] = { StripedView(Vessel) };
	const FIntPoint Pixels = DecorationPixels(How, Vessel, Layout);
	TestEqual(TEXT("The decoration is composed at the finest the record allows its view to resolve"), Pixels.X,
		FMath::RoundToInt32(Layout.SizeMeters().X * FMath::Min(CameraFocalPixels / CameraDistanceMeters, How.MostPixelsPerMeter)));
	FImage Decoration;
	FDecorationStats Stats;
	ComposeDecoration(How, Vessel, Section, Views, Pixels, Decoration, Stats);
	// The wall's middle, 0.1 m up it, is 0.2 m along the section (0.1 across the foot, then 0.1 up).
	const int32 Middle = FMath::FloorToInt32(0.2 / Layout.SizeMeters().Y * Decoration.SizeY);
	const TArray<FRun> Runs = RedRuns(Decoration, Middle);
	const double Texel = 360.0 / Decoration.SizeX;
	if (TestEqual(TEXT("Both stripes land on the wall, and nothing else red"), Runs.Num(), 2))
	{
		TestTrue(FString::Printf(TEXT("The axis's stripe lands on the side its camera faced, %.0f degrees round (%.2f)"), CameraBearingDegrees,
			Runs[0].MiddleDegrees), FMath::Abs(Runs[0].MiddleDegrees - CameraBearingDegrees) < Texel);
		TestTrue(FString::Printf(TEXT("The other lands %.0f degrees further round (%.2f)"), AskanceDegrees, Runs[1].MiddleDegrees),
			FMath::Abs(Runs[1].MiddleDegrees - (CameraBearingDegrees + AskanceDegrees)) < Texel);
		// Neither is stretched nor squeezed: each spans on the decoration the angle it spans on the vessel, to a texel.
		const double AxisWidth = StripeDegrees(AxisStripePixels, 0.0);
		const double AskanceWidth = StripeDegrees(AskanceStripePixels, AskanceDegrees);
		TestTrue(FString::Printf(TEXT("The axis's stripe spans its %.2f degrees of wall (%.2f)"), AxisWidth, Runs[0].WidthDegrees),
			FMath::Abs(Runs[0].WidthDegrees - AxisWidth) <= Texel);
		TestTrue(FString::Printf(TEXT("The stripe seen askance spans its %.2f degrees of wall, not its width in the photograph (%.2f)"), AskanceWidth,
			Runs[1].WidthDegrees), FMath::Abs(Runs[1].WidthDegrees - AskanceWidth) <= Texel);
	}
	const TArrayView64<const FColor> Colours = Decoration.AsBGRA8();
	auto AtAngle = [&](double Degrees) { return Colours[int64(Middle) * Decoration.SizeX + FMath::FloorToInt32(Degrees / 360.0 * Decoration.SizeX)]; };
	TestTrue(TEXT("Where the view saw the wall 75 degrees askance it lends nothing: the glaze fills in"), AtAngle(115.0).R > 200 && AtAngle(115.0).G > 200);
	TestFalse(TEXT("Nor on the far side"), IsRed(AtAngle(220.0)));
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
