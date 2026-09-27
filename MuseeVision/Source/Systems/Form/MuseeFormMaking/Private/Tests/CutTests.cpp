#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/FormMakeTesting.h"

/**
 * Cuts and repeats: a block sawn on a slant, a moulding's sink trimmed to the face it meets, a turned ring cut across by
 * a plane that leaves two cut faces; and identical members, made once and set at their frames.
 */
namespace Musee::Form::Tests::Cuts
{
	namespace
	{
		constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		FFormExtrusion Cube()
		{
			FFormExtrusion Shape;
			Shape.Outline = FFormProfile::Rectangle(FVector2D(0.0, 0.0), FVector2D(1.0, 1.0));
			Shape.DepthMeters = 1.0;
			return Shape;
		}

		FFormCut Cut(const FVector& Point, const FVector& Normal)
		{
			FFormCut Out;
			Out.PointMeters = Point;
			Out.Normal = Normal;
			return Out;
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormCutTest, "Musee.Form.Drawing.Cuts", Flags)
	bool FFormCutTest::RunTest(const FString& Parameters)
	{
		// A metre cube sawn where x + z passes 0.5: a wedge of an eighth of a cube, its cut face finished apart.
		FFormDrawing Sawn = FFormDrawing::Of(TEXT("block"), TEXT("stone"), Cube());
		Sawn.Cuts.Add(Cut(FVector(0.5, 0.0, 0.0), FVector(1.0, 0.0, 1.0)));
		Sawn.PartSurfaces.Add(FFormDrawing::CutPart, TEXT("stone.sawn"));
		const TArray<FFormMadePiece> Wedge = MakeDrawing(*this, Sawn);
		if (TestEqual(TEXT("the sawn block is one piece"), Wedge.Num(), 1))
		{
			TestClosed(*this, TEXT("the sawn block"), Wedge[0], 2);
			TestEqual(TEXT("it keeps the wedge below the plane"), VolumeCubicMeters(Wedge[0]), 0.125, 1e-9);
			TestEqual(TEXT("its cut face is finished apart"), Wedge[0].SlotSurfaces, TArray<FName>{TEXT("stone.sawn")});
			TestEqual(TEXT("the cut face lies on the slant"), FacingArea(Wedge[0], 1, FVector(1.0, 0.0, 1.0).GetSafeNormal()), 0.5 * UE_DOUBLE_SQRT_2, 1e-9);
		}

		// A moulding whose drawn section sinks 10 mm behind the wall it runs along, trimmed back to the wall's face.
		FFormSweep Moulding;
		Moulding.Section = FFormProfile::Rectangle(FVector2D(-0.01, 0.0), FVector2D(0.1, 0.2));
		Moulding.Path = FFormProfile::Polyline({FVector2D(0.0, 0.0), FVector2D(2.0, 0.0)}, false);
		FFormDrawing Trimmed = FFormDrawing::Of(TEXT("moulding"), TEXT("moulding"), Moulding);
		Trimmed.Cuts.Add(Cut(FVector::ZeroVector, -FVector::RightVector));
		const TArray<FFormMadePiece> Rail = MakeDrawing(*this, Trimmed);
		if (TestEqual(TEXT("the trimmed moulding is one piece"), Rail.Num(), 1))
		{
			TestClosed(*this, TEXT("the trimmed moulding"), Rail[0], 2);
			TestEqual(TEXT("it keeps what stands proud of the wall"), VolumeCubicMeters(Rail[0]), 2.0 * 0.1 * 0.2, 1e-9);
			TestEqual(TEXT("its back lies on the wall's face"), Bounds(Rail[0]).Min.Y, 0.0, 1e-9);
		}

		// A skirting round a round room (r 3 m), its section sinking 10 mm behind the wall and 25 mm under the floor: its sinks
		// trimmed, it keeps its 0.025 by 0.14 proud of both, on the curve.
		FFormSweep Skirting;
		Skirting.Section = FFormProfile::Rectangle(FVector2D(-0.01, -0.025), FVector2D(0.025, 0.14));
		Skirting.Path = FFormProfile::Ellipse(FVector2D::ZeroVector, 3.0, 3.0, 0.0, true);
		Skirting.bTrimSinkA = true;
		Skirting.bTrimSinkB = true;
		const TArray<FFormMadePiece> Round = MakeDrawing(*this, FFormDrawing::Of(TEXT("skirting"), TEXT("oak"), Skirting));
		if (TestEqual(TEXT("the trimmed skirting is one piece"), Round.Num(), 1))
		{
			TestClosed(*this, TEXT("the trimmed skirting"), Round[0], 0);
			const FBox Box = Bounds(Round[0]);
			TestEqual(TEXT("it stands on the floor"), Box.Min.Z, 0.0, 1e-9);
			TestEqual(TEXT("its back lies on the wall, 3 m out"), Box.Max.X, 3.0, 1e-6);
			// The inscribed polygon's area within a millimetre of the circle's, the ring 25 mm wide.
			TestEqual(TEXT("it holds its section round the room"), VolumeCubicMeters(Round[0]), 0.025 * 0.14 * 2.0 * UE_DOUBLE_PI * (3.0 - 0.0125),
				0.001 * 0.025 * 0.14 * 2.0 * UE_DOUBLE_PI * 3.0);
		}

		// A die's base moulding whose section reaches the die's face exactly at a point of its own before sinking behind it,
		// run round the die: trimmed, the point on the face is where it is cut.
		FFormSweep Base;
		Base.Section = FFormProfile::Polyline({FVector2D(-0.01, -0.005), FVector2D(0.015, -0.005), FVector2D(0.015, 0.025), FVector2D(0.0, 0.045),
			FVector2D(-0.01, 0.045)}, true);
		Base.Path = FFormProfile::Polyline({FVector2D(0.2, 0.2), FVector2D(0.2, -0.2), FVector2D(-0.2, -0.2), FVector2D(-0.2, 0.2)}, true);
		Base.bTrimSinkA = true;
		Base.bTrimSinkB = true;
		const TArray<FFormMadePiece> Moulded = MakeDrawing(*this, FFormDrawing::Of(TEXT("die.base"), TEXT("stone"), Base));
		if (TestEqual(TEXT("the trimmed base is one piece"), Moulded.Num(), 1))
		{
			TestClosed(*this, TEXT("a base trimmed where its section meets the face"), Moulded[0], 0);
		}

		// A ring turned about z, cut on the plane x = 0: half a ring, closed by two round cut faces.
		FFormLathe Ring;
		Ring.Section = FFormProfile::Ellipse(FVector2D(1.0, 0.0), 0.1, 0.1, 0.0, true);
		FFormDrawing Halved = FFormDrawing::Of(TEXT("ring"), TEXT("bronze"), Ring);
		Halved.Cuts.Add(Cut(FVector::ZeroVector, -FVector::ForwardVector));
		const TArray<FFormMadePiece> Half = MakeDrawing(*this, Halved);
		if (TestEqual(TEXT("the halved ring is one piece"), Half.Num(), 1))
		{
			TestClosed(*this, TEXT("the halved ring"), Half[0], 2);
			// Its section is a polygon within a millimetre of its circle: some 1.3 % less than the circle's area.
			TestEqual(TEXT("it holds half the ring, within its chords"), VolumeCubicMeters(Half[0]), UE_DOUBLE_PI * UE_DOUBLE_PI * 0.01, 0.02 * 0.1);
		}

		// A plane that misses leaves the block whole; one that takes it all is a finding.
		FFormDrawing Missed = FFormDrawing::Of(TEXT("block"), TEXT("stone"), Cube());
		Missed.Cuts.Add(Cut(FVector(2.0, 0.0, 0.0), FVector::ForwardVector));
		const TArray<FFormMadePiece> Whole = MakeDrawing(*this, Missed);
		TestTrue(TEXT("a plane that misses leaves the block whole"), Whole.Num() == 1 && FMath::IsNearlyEqual(VolumeCubicMeters(Whole[0]), 1.0, 1e-9));
		FFormDrawing Lost = FFormDrawing::Of(TEXT("block"), TEXT("stone"), Cube());
		Lost.Cuts.Add(Cut(FVector(-1.0, 0.0, 0.0), FVector::ForwardVector));
		TArray<FMuseeFinding> Findings;
		MakeDrawing(*this, Lost, &Findings);
		TestEqual(TEXT("a cut that takes the whole block is a finding"), Findings.Num(), 1);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormRepeatTest, "Musee.Form.Drawing.Repeats", Flags)
	bool FFormRepeatTest::RunTest(const FString& Parameters)
	{
		// Three balusters in a row along y, the row itself set 10 m east.
		FFormLathe Baluster;
		Baluster.Section = FFormProfile::Polyline({FVector2D(0.0, 0.0), FVector2D(0.05, 0.0), FVector2D(0.03, 0.8), FVector2D(0.0, 0.8)}, false);
		FFormDrawing Row = FFormDrawing::Of(TEXT("baluster"), TEXT("stone"), Baluster);
		Row.Frame = FMuseeFrame(FVector(10.0, 0.0, 0.0));
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Row.Repeats.Add(FMuseeFrame(FVector(0.0, 0.2 * Index, 0.0)));
		}
		const TArray<FFormMadePiece> Pieces = MakeDrawing(*this, Row);
		if (!TestEqual(TEXT("each member is a piece"), Pieces.Num(), 3))
		{
			return false;
		}
		for (int32 Index = 0; Index < 3; ++Index)
		{
			TestEqual(TEXT("each is named for its place in the row"), Pieces[Index].Name, FName(FString::Printf(TEXT("baluster.%d"), Index + 1)));
			TestTrue(TEXT("each stands at its frame within the drawing's"), Pieces[Index].Frame.OriginMeters.Equals(FVector(10.0, 0.2 * Index, 0.0), 1e-12));
			TestEqual(TEXT("each is the same solid"), Pieces[Index].Mesh->VertexCount(), Pieces[0].Mesh->VertexCount());
			TestTrue(TEXT("point for point"), Pieces[Index].Mesh->GetVertex(Pieces[Index].Mesh->MaxVertexID() - 1) == Pieces[0].Mesh->GetVertex(Pieces[0].Mesh->MaxVertexID() - 1));
		}
		return true;
	}
}

#endif
