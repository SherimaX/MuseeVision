#include "ArtHang.h"
#include "ArtWork.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "MuseeRecords.h"
#include "MuseeUnits.h"
#include "Vessel/VesselDecoration.h"
#include "Vessel/VesselMesh.h"
#include "Vessel/VesselSection.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace Musee::ArtMaking::VesselTests
{
	/** Art's making tests run in the editor, where its making twin lives, and belong to the product's own filter. */
	inline constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	/** A derived cylinder 0.1 m in radius and 0.2 m tall, its museum's 0.2 m across (a section measured from the vessel). */
	FArtVessel DerivedCylinder(double RadiusMeters, double HeightMeters)
	{
		FArtVessel Vessel;
		Vessel.Work = TEXT("derived-cylinder");
		Vessel.Grade = TEXT("derived");
		Vessel.Museum.HeightMeters = 0.2;
		Vessel.Museum.DiameterMeters = 0.2;
		Vessel.Section = { FVector2D(0.0, 0.0), FVector2D(RadiusMeters, 0.0), FVector2D(RadiusMeters, HeightMeters), FVector2D(0.0, HeightMeters) };
		Vessel.Outside = { 1, 2 };
		return Vessel;
	}

	/** The test's photograph: square, its camera looking at the cylinder's axis from far off, level with its middle. */
	constexpr int32 PhotoPixels = 1000;
	constexpr double CameraDistanceMeters = 10.0;
	constexpr double CameraFocalPixels = 40000.0;
	constexpr double CameraBearingDegrees = 40.0;
	constexpr double CylinderRadiusMeters = 0.1;

	/**
	 * The stripes painted on the photograph: one down the axis, one where the camera sees the wall 30 degrees round, and
	 * one where it sees the wall obliquely, 70 degrees round (the wall tilted away, a pixel covering three times the wall
	 * it does face on).
	 */
	constexpr double AskanceDegrees = 30.0;
	constexpr double ObliqueDegrees = 70.0;
	constexpr double AxisStripePixels = 41.0;
	constexpr double AskanceStripePixels = 21.0;
	constexpr double ObliqueStripePixels = 21.0;

	/** A camera looking at a vessel's axis from its bearing, so far off and so high, its photograph square. */
	FArtVesselView ViewFrom(double BearingDegrees, double DistanceMeters, double HeightMeters, double PitchDegrees, double FocalPixels)
	{
		FArtVesselView View;
		View.Image = TEXT("assets/ceramics/test/striped.png");
		View.AzimuthDegrees = BearingDegrees;
		View.DistanceMeters = DistanceMeters;
		View.HeightMeters = HeightMeters;
		View.PitchDegrees = PitchDegrees;
		View.FocalPixels = FocalPixels;
		View.AxisPixels = { 0.5 * PhotoPixels, 0.5 * PhotoPixels };
		View.Placement = TEXT("fitted");
		return View;
	}

	/** A plain cylinder 0.1 m in radius and 0.2 m tall, its outside the wall from its foot's edge to its rim's, and its one view. */
	FArtVessel Cylinder()
	{
		FArtVessel Vessel;
		Vessel.Work = TEXT("cylinder");
		Vessel.Section = { FVector2D(0.0, 0.0), FVector2D(CylinderRadiusMeters, 0.0), FVector2D(CylinderRadiusMeters, 0.2), FVector2D(0.0, 0.2) };
		Vessel.Outside = { 1, 2 };
		Vessel.Views.Add(ViewFrom(CameraBearingDegrees, CameraDistanceMeters, 0.1, 0.0, CameraFocalPixels));
		return Vessel;
	}

	/** A composition as the record's: views lend wherever they see, 2 px inside the silhouette; 1000 pixels a metre at the finest. */
	FArtVesselComposition Composition()
	{
		FArtVesselComposition How;
		How.EdgePixels = 2.0;
		How.ClearanceMeters = 0.001;
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

	/** A photograph of the given size through a view's camera, each pixel coloured by where it lies. */
	FVesselPicture Painted(const FArtVesselView& View, TFunctionRef<FColor(double Column, double Row)> Colour)
	{
		FVesselPicture Picture;
		Picture.Image.Init(PhotoPixels, PhotoPixels, 1, ERawImageFormat::BGRA8, EGammaSpace::sRGB);
		Picture.Camera = CameraOf(View);
		const TArrayView64<FColor> Pixels = Picture.Image.AsBGRA8();
		for (int32 Y = 0; Y < PhotoPixels; ++Y)
		{
			for (int32 X = 0; X < PhotoPixels; ++X)
			{
				Pixels[int64(Y) * PhotoPixels + X] = Colour(X + 0.5, Y + 0.5);
			}
		}
		return Picture;
	}

	const FColor Red(220, 20, 20);
	const FColor Ground(240, 240, 240);

	/**
	 * The cylinder's photograph through its view's camera: a white ground, a red stripe down its axis and others where the
	 * camera sees the wall AskanceDegrees and ObliqueDegrees round to its right.
	 */
	FVesselPicture StripedView(const FArtVessel& Vessel)
	{
		const double Centre = 0.5 * PhotoPixels;
		const double Askance = Centre + ColumnOffset(AskanceDegrees);
		const double Oblique = Centre + ColumnOffset(ObliqueDegrees);
		return Painted(Vessel.Views[0], [=](double Column, double)
		{
			const bool bRed = FMath::Abs(Column - Centre) < 0.5 * AxisStripePixels || FMath::Abs(Column - Askance) < 0.5 * AskanceStripePixels
				|| FMath::Abs(Column - Oblique) < 0.5 * ObliqueStripePixels;
			return bRed ? Red : Ground;
		});
	}

	bool IsRed(const FColor& C)
	{
		return C.R > 150 && C.G < 120 && C.B < 120;
	}

	/** Whether a colour is tinged towards red at all: the ground's red lent where the vessel isn't. */
	bool IsTinged(const FColor& C)
	{
		return int32(C.R) - int32(C.G) > 20;
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

	/**
	 * The angles round the wall (from the camera's bearing) at the two edges of a stripe of so many pixels centred where
	 * the camera sees the wall so many degrees round, found by bisection on the (monotonic) column offset.
	 */
	FVector2D StripeEdges(double Pixels, double DegreesRound)
	{
		const double Half = 0.5 * Pixels;
		const double Offset = ColumnOffset(DegreesRound);
		auto AngleAt = [](double Target)
		{
			double Low = -89.0;
			double High = 89.0;
			for (int32 Step = 0; Step < 60; ++Step)
			{
				const double Mid = 0.5 * (Low + High);
				(ColumnOffset(Mid) < Target ? Low : High) = Mid;
			}
			return 0.5 * (Low + High);
		};
		return FVector2D(AngleAt(Offset - Half), AngleAt(Offset + Half));
	}

	/** The angle round the wall a stripe spans, and its middle (not its centre's angle where the wall turns away). */
	double StripeDegrees(double Pixels, double DegreesRound)
	{
		const FVector2D Edges = StripeEdges(Pixels, DegreesRound);
		return Edges.Y - Edges.X;
	}

	double StripeMiddle(double Pixels, double DegreesRound)
	{
		const FVector2D Edges = StripeEdges(Pixels, DegreesRound);
		return 0.5 * (Edges.X + Edges.Y);
	}

	/** A composed decoration's texel at a height up a vessel's outside and an angle round it (degrees from the front). */
	FColor TexelAt(const FImage& Decoration, const FVesselLayout& Layout, double LengthMeters, double Degrees)
	{
		const int32 Row = FMath::Clamp(FMath::FloorToInt32(LengthMeters / Layout.SizeMeters().Y * Decoration.SizeY), 0, Decoration.SizeY - 1);
		const int32 Column = FMath::Clamp(FMath::FloorToInt32(FMath::Fmod(Degrees + 360.0, 360.0) / 360.0 * Decoration.SizeX), 0, Decoration.SizeX - 1);
		return Decoration.AsBGRA8()[int64(Row) * Decoration.SizeX + Column];
	}

	/** A turned mesh's widest across (x) and front to back (y) at a height, metres: twice its rings' farthest there. */
	FVector2D AxesAt(const UE::Geometry::FDynamicMesh3& Mesh, double HeightMeters)
	{
		FVector2D Most = FVector2D::ZeroVector;
		for (const int32 Vertex : Mesh.VertexIndicesItr())
		{
			const FVector Point = Musee::FromEngine(FVector(Mesh.GetVertex(Vertex)));
			if (FMath::IsNearlyEqual(Point.Z, HeightMeters, 1e-6))
			{
				Most = FVector2D(FMath::Max(Most.X, FMath::Abs(Point.X)), FMath::Max(Most.Y, FMath::Abs(Point.Y)));
			}
		}
		return 2.0 * Most;
	}

	/** How far a measure stands from its museum's, as a share of it. */
	double Off(double Measured, double Published)
	{
		return FMath::Abs(Measured - Published) / Published;
	}

	/** A painted vessel's photographs, as the record names them, each with its record's camera; false where one won't read. */
	bool LoadViews(const FArtVessel& Vessel, TArray<FVesselPicture>& Out)
	{
		for (const FArtVesselView& View : Vessel.Views)
		{
			FImage Loaded;
			if (!FImageUtils::LoadImage(*Musee::Art::SourcePath(View.Image), Loaded))
			{
				return false;
			}
			FVesselPicture& Picture = Out.AddDefaulted_GetRef();
			Loaded.CopyTo(Picture.Image, ERawImageFormat::BGRA8, EGammaSpace::sRGB);
			Picture.Camera = CameraOf(View);
		}
		return true;
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
	if (!TestTrue(FString::Printf(TEXT("The vessels' record and Chenghuai's plan read (%s%s)"), Record.HasError() ? *Record.GetError() : TEXT(""),
		Hang.HasError() ? *Hang.GetError() : TEXT("")), Record.HasValue() && Hang.HasValue()))
	{
		return false;
	}
	FString Sheet;
	TestTrue(TEXT("Chenghuai's sheet reads"), FFileHelper::LoadFileToString(Sheet, *FPaths::Combine(FPaths::ProjectDir(), TEXT("Docs/Stages/chenghuai.md"))));
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
		// Measured on the section itself, as the record keeps it: the plan's size can't stand in for it.
		FBox2D Box(ForceInit);
		for (const FVector2D& Point : Vessel.Section)
		{
			Box += Point;
		}
		// Its foot, or its feet, stand on the plane at 0.
		TestTrue(FString::Printf(TEXT("%s's section stands its museum's %.1f cm (%.2f)"), *Vessel.Work, 100.0 * Vessel.Museum.HeightMeters, 100.0 * Box.Max.Y),
			FMath::IsNearlyEqual(Box.Max.Y, Vessel.Museum.HeightMeters, SectionHeightToleranceMeters) && Box.Min.Y >= 0.0
			&& (FMath::IsNearlyZero(Box.Min.Y) || !Vessel.Feet.Each.IsEmpty()));
		const double Across = Vessel.Museum.IsOval() ? Vessel.Museum.MouthAxesMeters[0] : Vessel.Museum.DiameterMeters;
		if (Across > 0.0)
		{
			TestTrue(FString::Printf(TEXT("%s's section is its museum's %.1f cm across within %.0f%% (%.2f), or its record says why"), *Vessel.Work,
				100.0 * Across, 100.0 * Record.GetValue().DiameterTolerance, 200.0 * Box.Max.X),
				Off(2.0 * Box.Max.X, Across) <= Record.GetValue().DiameterTolerance || !Vessel.Departure.IsEmpty());
		}
		// What its derivation says it measured is what the section as stored measures, one way: every point, a lip's too.
		const FString Widest = TEXT("widest diameter ");
		const int32 Said = Vessel.Derivation.Find(Widest);
		if (Said != INDEX_NONE)
		{
			const double Stated = FCString::Atod(*Vessel.Derivation.Mid(Said + Widest.Len()));
			TestTrue(FString::Printf(TEXT("%s's derivation gives its widest diameter as its section's, %.2f cm (says %.2f)"), *Vessel.Work,
				200.0 * Box.Max.X, Stated), FMath::Abs(Stated - 200.0 * Box.Max.X) <= 0.005);
		}
		// A vessel that departs from its museum's diameter is never given in the sheet at its museum's height by diameter.
		if (!Vessel.Departure.IsEmpty() && Vessel.Museum.DiameterMeters > 0.0)
		{
			const FString MuseumSize = FString::Printf(TEXT("%.1f × %g cm"), 100.0 * Vessel.Museum.HeightMeters, 100.0 * Vessel.Museum.DiameterMeters);
			TestFalse(FString::Printf(TEXT("Chenghuai's sheet never gives %s as its museum's %s, which it departs from"), *Vessel.Work, *MuseumSize),
				Sheet.Contains(MuseumSize));
		}
		const FString Size = SizeFault(Vessel, Entry->SizeMeters(), Record.GetValue().DiameterTolerance);
		TestTrue(FString::Printf(TEXT("%s is its museum's size and its plan the section's (%s)"), *Vessel.Work, *Size), Size.IsEmpty());
		// Turned at its plan's size, it keeps its section's proportion: one scale across and up.
		const TArray<FVector2D> Sized = SizedSection(Vessel, Entry->SizeMeters());
		FBox2D SizedBox(ForceInit);
		for (const FVector2D& Point : Sized)
		{
			SizedBox += Point;
		}
		TestTrue(FString::Printf(TEXT("%s turned at its plan's size keeps its section's proportion"), *Vessel.Work),
			FMath::IsNearlyEqual(SizedBox.Max.X / Box.Max.X, SizedBox.Max.Y / Box.Max.Y, 1e-9));
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
			TestFalse(FString::Printf(TEXT("%s's %s says how its repeat is known"), *Vessel.Work, *Band.Name), Band.Reading.IsEmpty());
		}
		for (int32 Index = 0; Index < Vessel.Views.Num(); ++Index)
		{
			const FArtVesselView& View = Vessel.Views[Index];
			TestTrue(FString::Printf(TEXT("%s's view %s is the one its section was fitted to, or placed after it"), *Vessel.Work, *View.Image),
				Index == 0 ? View.Placement == TEXT("fitted") && View.AzimuthDegrees == 0.0 : FArtVesselView::IsPlacement(View.Placement) && View.Placement != TEXT("fitted"));
			TestTrue(FString::Printf(TEXT("%s's view %s stands where its photographs place it, not unverified"), *Vessel.Work, *View.Image),
				View.Placement != TEXT("unverified"));
		}
		// Each registered view stands where the evidence that placed it puts it; turned a quarter turn either way, it doesn't.
		if (Vessel.Views.ContainsByPredicate([](const FArtVesselView& View) { return View.Placement == TEXT("registered"); }))
		{
			const FArtVesselRegistration& Registration = Record.GetValue().Registration;
			TArray<FVesselPicture> Pictures;
			if (TestTrue(FString::Printf(TEXT("%s's photographs read"), *Vessel.Work), LoadViews(Vessel, Pictures)))
			{
				const TArray<TArray<FViewOverlap>> Overlaps = ViewOverlaps(Registration, Vessel, Pictures);
				for (int32 I = 0; I < Pictures.Num(); ++I)
				{
					for (int32 J = I + 1; J < Pictures.Num(); ++J)
					{
						AddInfo(FString::Printf(TEXT("%s: %s and %s share %.4f at a correlation of %.3f"), *Vessel.Work, *FPaths::GetCleanFilename(Vessel.Views[I].Image),
							*FPaths::GetCleanFilename(Vessel.Views[J].Image), Overlaps[I][J].Share, Overlaps[I][J].Correlation));
					}
				}
				const TArray<FString> Faults = PlacementFaults(Registration, Vessel, Pictures);
				TestTrue(FString::Printf(TEXT("%s's registered views stand where their overlaps placed them (%s)"), *Vessel.Work,
					*FString::Join(Faults, TEXT("; "))), Faults.IsEmpty());
				for (int32 Index = 0; Index < Vessel.Views.Num(); ++Index)
				{
					if (Vessel.Views[Index].Placement != TEXT("registered"))
					{
						continue;
					}
					for (const double Turn : { 90.0, -90.0 })
					{
						FArtVessel Turned = Vessel;
						Turned.Views[Index].AzimuthDegrees += Turn;
						const FVesselCamera Placed = Pictures[Index].Camera;
						Pictures[Index].Camera = CameraOf(Turned.Views[Index]);
						TestFalse(FString::Printf(TEXT("%s's view %s turned %+.0f degrees from its place is found"), *Vessel.Work, *Vessel.Views[Index].Image, Turn),
							PlacementFaults(Registration, Turned, Pictures).IsEmpty());
						Pictures[Index].Camera = Placed;
					}
				}
			}
		}
	}
	TestEqual(TEXT("Seven vessels are derived from their photographs"), Derived, 7);

	// A derived section is checked on itself: one too narrow for its museum's diameter is a fault unless its record says
	// why, one of the wrong height is a fault whatever its plan, and turned at a plan wider than itself it isn't stretched.
	{
		const double Tolerance = 0.02;
		TestTrue(TEXT("A derived section of its museum's size is sound"), SizeFault(DerivedCylinder(0.1, 0.2), FVector2D(0.2, 0.2), Tolerance).IsEmpty());
		const FArtVessel Narrow = DerivedCylinder(0.093, 0.2);
		TestFalse(TEXT("A derived section 7% narrower than its museum's diameter is a fault"), SizeFault(Narrow, FVector2D(0.186, 0.2), Tolerance).IsEmpty());
		TestFalse(TEXT("Nor does a plan at the museum's diameter hide it"), SizeFault(Narrow, FVector2D(0.2, 0.2), Tolerance).IsEmpty());
		FArtVessel Said = Narrow;
		Said.Departure = TEXT("its photographs show it so");
		TestTrue(TEXT("A departure the record explains is not a fault"), SizeFault(Said, FVector2D(0.186, 0.2), Tolerance).IsEmpty());
		TestFalse(TEXT("A derived section of the wrong height is a fault"), SizeFault(DerivedCylinder(0.1, 0.19), FVector2D(0.2, 0.2), Tolerance).IsEmpty());
		const TArray<FVector2D> Turned = SizedSection(Narrow, FVector2D(0.2, 0.2));
		TestTrue(TEXT("A derived section turned at a wider plan keeps its proportion"),
			FMath::IsNearlyEqual(Turned[1].X, 0.093, 1e-12) && FMath::IsNearlyEqual(Turned[2].Y, 0.2, 1e-12));
	}

	// The oval Ru basin, turned as make turns it, is its museum's mouth and foot on both axes; pressed to its mouth's ratio
	// alone, its foot would not be.
	if (const FArtVessel* Ru = Record.GetValue().Find(TEXT("chinese-ru-basin")))
	{
		const FArtHangEntry* Entry = Hang.GetValue().Works.FindByPredicate([](const FArtHangEntry& E) { return E.Work == TEXT("chinese-ru-basin"); });
		FVesselTurning Turning;
		if (TestNotNull(TEXT("The Ru basin is in the plan"), Entry)
			&& TestTrue(TEXT("The Ru basin is pressed to its oval"), TurningOf(*Ru, Entry->SizeMeters(), Record.GetValue().CreaseDegrees, 0.0005, Turning).IsEmpty()))
		{
			const TArray<FVector2D> Sized = SizedSection(*Ru, Entry->SizeMeters());
			const double Tolerance = Record.GetValue().DiameterTolerance;
			UE::Geometry::FDynamicMesh3 Mesh;
			BuildVessel(Sized, LayoutOf(Sized), Turning, Mesh);
			const FVector2D Mouth = AxesAt(Mesh, Turning.Press.MouthHeightMeters);
			const FVector2D Foot = AxesAt(Mesh, Turning.Press.FootHeightMeters);
			const TArray<double>& MouthAxes = Ru->Museum.MouthAxesMeters;
			const TArray<double>& FootAxes = Ru->Museum.FootAxesMeters;
			TestTrue(FString::Printf(TEXT("The Ru basin's mouth is its museum's %.1f x %.1f cm (%.2f x %.2f)"), 100.0 * MouthAxes[0], 100.0 * MouthAxes[1],
				100.0 * Mouth.X, 100.0 * Mouth.Y), Off(Mouth.X, MouthAxes[0]) <= Tolerance && Off(Mouth.Y, MouthAxes[1]) <= Tolerance);
			TestTrue(FString::Printf(TEXT("The Ru basin's foot is its museum's %.1f x %.1f cm (%.2f x %.2f)"), 100.0 * FootAxes[0], 100.0 * FootAxes[1],
				100.0 * Foot.X, 100.0 * Foot.Y), Off(Foot.X, FootAxes[0]) <= Tolerance && Off(Foot.Y, FootAxes[1]) <= Tolerance);
			FVesselTurning MouthOnly = Turning;
			MouthOnly.Press.FootRatio = MouthOnly.Press.MouthRatio;
			UE::Geometry::FDynamicMesh3 Pressed;
			BuildVessel(Sized, LayoutOf(Sized), MouthOnly, Pressed);
			TestTrue(TEXT("Pressed to its mouth's ratio alone, its foot is too deep front to back, and found"),
				Off(AxesAt(Pressed, Turning.Press.FootHeightMeters).Y, FootAxes[1]) > Tolerance);
		}
	}

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

	// A view's painting lands where its camera saw it, at the width it has on the vessel, obliquely too.
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
	if (TestEqual(TEXT("The three stripes land on the wall, and nothing else red"), Runs.Num(), 3))
	{
		TestTrue(FString::Printf(TEXT("The axis's stripe lands on the side its camera faced, %.0f degrees round (%.2f)"), CameraBearingDegrees,
			Runs[0].MiddleDegrees), FMath::Abs(Runs[0].MiddleDegrees - CameraBearingDegrees) < Texel);
		TestTrue(FString::Printf(TEXT("The next lands %.0f degrees further round (%.2f)"), AskanceDegrees, Runs[1].MiddleDegrees),
			FMath::Abs(Runs[1].MiddleDegrees - (CameraBearingDegrees + AskanceDegrees)) < Texel);
		const double ObliqueMiddle = CameraBearingDegrees + StripeMiddle(ObliqueStripePixels, ObliqueDegrees);
		TestTrue(FString::Printf(TEXT("The one seen obliquely, where the wall tilts %.0f degrees away, lands there too, its middle at %.2f (%.2f)"),
			ObliqueDegrees, ObliqueMiddle, Runs[2].MiddleDegrees), FMath::Abs(Runs[2].MiddleDegrees - ObliqueMiddle) < Texel);
		// None is stretched nor squeezed: each spans on the decoration the angle it spans on the vessel, to a texel.
		const double AxisWidth = StripeDegrees(AxisStripePixels, 0.0);
		const double AskanceWidth = StripeDegrees(AskanceStripePixels, AskanceDegrees);
		const double ObliqueWidth = StripeDegrees(ObliqueStripePixels, ObliqueDegrees);
		TestTrue(FString::Printf(TEXT("The axis's stripe spans its %.2f degrees of wall (%.2f)"), AxisWidth, Runs[0].WidthDegrees),
			FMath::Abs(Runs[0].WidthDegrees - AxisWidth) <= Texel);
		TestTrue(FString::Printf(TEXT("The stripe seen askance spans its %.2f degrees of wall, not its width in the photograph (%.2f)"), AskanceWidth,
			Runs[1].WidthDegrees), FMath::Abs(Runs[1].WidthDegrees - AskanceWidth) <= Texel);
		TestTrue(FString::Printf(TEXT("The stripe seen obliquely spans its %.2f degrees of wall (%.2f)"), ObliqueWidth, Runs[2].WidthDegrees),
			FMath::Abs(Runs[2].WidthDegrees - ObliqueWidth) <= Texel);
	}
	const TArrayView64<const FColor> Colours = Decoration.AsBGRA8();
	auto AtAngle = [&](double Degrees) { return Colours[int64(Middle) * Decoration.SizeX + FMath::FloorToInt32(Degrees / 360.0 * Decoration.SizeX)]; };
	TestTrue(TEXT("On the far side, which the view doesn't see, the glaze is left"), AtAngle(220.0).R > 200 && AtAngle(220.0).G > 200);
	TestTrue(FString::Printf(TEXT("The view gave the outside whole all round the side it sees, and none of the far side (%.3f)"), Stats.Seen),
		Stats.Seen > 0.4 && Stats.Seen < 0.5);

	// A view lends nothing of the ground past the vessel's silhouette, even where its camera's fit strays by a pixel or two.
	{
		const double Limb = CameraFocalPixels * CylinderRadiusMeters / FMath::Sqrt(FMath::Square(CameraDistanceMeters) - FMath::Square(CylinderRadiusMeters));
		// The wall's rims: its top and its foot at their nearest, where the photograph's outline runs.
		auto RowOf = [](double Height)
		{
			return 0.5 * PhotoPixels - CameraFocalPixels * (Height - 0.1) / (CameraDistanceMeters - CylinderRadiusMeters);
		};
		const double TopRow = RowOf(0.2);
		const double FootRow = RowOf(0.0);
		FArtVessel Fitted = Cylinder();
		Fitted.Views[0].FitPixels = 1.0;
		const FVesselPicture Grounded[] = { Painted(Fitted.Views[0], [=](double Column, double Row)
		{
			const bool bVessel = FMath::Abs(Column - 0.5 * PhotoPixels) < Limb && Row > TopRow && Row < FootRow;
			return bVessel ? Ground : Red;
		}) };
		FImage Clean;
		FDecorationStats CleanStats;
		ComposeDecoration(How, Fitted, Section, Grounded, Pixels, Clean, CleanStats);
		int32 Tinged = 0;
		for (const FColor& Colour : Clean.AsBGRA8())
		{
			Tinged += IsTinged(Colour) ? 1 : 0;
		}
		TestEqual(TEXT("No texel takes the red ground beyond the vessel's silhouette, at its sides, its rim or its foot"), Tinged, 0);
	}

	// Nor what another part of the vessel hides: under a flange, where the view's line of sight passes through it.
	{
		FArtVessel Flanged;
		Flanged.Work = TEXT("flanged");
		Flanged.Section = { FVector2D(0.0, 0.0), FVector2D(0.1, 0.0), FVector2D(0.1, 0.1), FVector2D(0.2, 0.1), FVector2D(0.2, 0.11),
			FVector2D(0.1, 0.11), FVector2D(0.1, 0.2), FVector2D(0.0, 0.2) };
		Flanged.Outside = { 1, 6 };
		// seen from 45 degrees above, its lower wall faces the camera, but the flange stands between; the photograph a red
		// stripe down the middle, where the vessel's front shows, on the white glaze
		Flanged.Views.Add(ViewFrom(0.0, 1.0, 1.0, 45.0, 1000.0));
		const FVesselPicture Seen[] = { Painted(Flanged.Views[0], [](double Column, double) { return FMath::Abs(Column - 0.5 * PhotoPixels) < 20.0 ? Red : Ground; }) };
		const FVesselLayout FlangedLayout = LayoutOf(Flanged.Section);
		FImage Hidden;
		FDecorationStats HiddenStats;
		ComposeDecoration(How, Flanged, Flanged.Section, Seen, DecorationPixels(How, Flanged, FlangedLayout), Hidden, HiddenStats);
		// Along the section: 0.1 across the foot, 0.1 up the lower wall, 0.1 out under the flange, 0.01 up its face, 0.1 back
		// over it, then the upper wall.
		TestTrue(TEXT("The lower wall under the flange, facing the camera but hidden from it, is left in the glaze"),
			TexelAt(Hidden, FlangedLayout, 0.15, 0.0).G > 200);
		TestTrue(TEXT("The flange's top, which the camera sees, takes the photograph's red"), IsRed(TexelAt(Hidden, FlangedLayout, 0.36, 0.0)));
		TestTrue(TEXT("And the upper wall, clear of the flange, takes it too"), IsRed(TexelAt(Hidden, FlangedLayout, 0.46, 0.0)));
	}

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
