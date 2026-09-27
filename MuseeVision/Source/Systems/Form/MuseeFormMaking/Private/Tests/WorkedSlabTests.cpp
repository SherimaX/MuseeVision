#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "FormAxes.h"
#include "Stations/Stations.h"
#include "Tests/FormMakeTesting.h"

/**
 * Worked slabs: an extrusion's faces finished apart, pockets sunk into its faces (blind, a rebate over its edge, one
 * stepped in another, from either face), its arrises eased round or chamfered; and the public drawing's corner-only
 * polygon and frame of axes. Every one is one closed solid holding the volume the mason would cut.
 */
namespace Musee::Form::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		/** A slab 1.2 m by 0.8 m, 0.1 m thick: a floor flag. */
		FFormExtrusion Flag()
		{
			FFormExtrusion Slab;
			Slab.Outline = FFormProfile::Rectangle(FVector2D(0.0, 0.0), FVector2D(1.2, 0.8));
			Slab.DepthMeters = 0.1;
			return Slab;
		}

		FFormPocket Pocket(const FVector2D& Min, const FVector2D& Max, double Depth, bool bNear = false)
		{
			FFormPocket Out;
			Out.Outline = FFormProfile::Rectangle(Min, Max);
			Out.DepthMeters = Depth;
			Out.bNearFace = bNear;
			return Out;
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormPolygonTest, "Musee.Form.Drawing.Polygon", Flags)
	bool FFormPolygonTest::RunTest(const FString& Parameters)
	{
		// A square with its start repeated, a point on its south side, a point that turns straight back, and a double.
		const FFormProfile Square = FFormProfile::Polygon({FVector2D(0.0, 0.0), FVector2D(0.5, 0.0), FVector2D(1.0, 0.0), FVector2D(1.0, 1.0),
			FVector2D(1.0, 1.0), FVector2D(1.0, 1.5), FVector2D(1.0, 1.0), FVector2D(0.0, 1.0), FVector2D(0.0, 0.0)});
		TestTrue(TEXT("a corner-only polygon is closed"), Square.Closed);
		TestEqual(TEXT("it keeps the square's four corners and nothing else"), Square.Steps.Num() + 1, 4);
		TestTrue(TEXT("it is well formed"), Square.IsWellFormed());
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormFrameOfAxesTest, "Musee.Form.Drawing.FrameOfAxes", Flags)
	bool FFormFrameOfAxesTest::RunTest(const FString& Parameters)
	{
		// A wall's anchor: x along the wall (north), y out of it (east, given leaning up), z completing them.
		const FMuseeFrame Frame = Musee::Form::FrameOfAxes(FVector(1.0, 2.0, 3.0), FMuseeFrame::North() * 2.0, FMuseeFrame::East() + FMuseeFrame::Up() * 0.3);
		TestTrue(TEXT("its origin is the given point"), Frame.OriginMeters.Equals(FVector(1.0, 2.0, 3.0)));
		TestTrue(TEXT("its x runs along X"), Frame.DirectionToParent(FVector::ForwardVector).Equals(FMuseeFrame::North(), 1e-9));
		TestTrue(TEXT("its y is Y made square to x"), Frame.DirectionToParent(FVector::RightVector).Equals((FMuseeFrame::East() + FMuseeFrame::Up() * 0.3).GetSafeNormal(), 1e-9));
		TestTrue(TEXT("its z completes them as the engine's axes do"),
			Frame.DirectionToParent(FVector::UpVector).Equals(FVector::CrossProduct(FMuseeFrame::North(), (FMuseeFrame::East() + FMuseeFrame::Up() * 0.3).GetSafeNormal()), 1e-9));
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormExtrusionFacesTest, "Musee.Form.Drawing.ExtrusionFaces", Flags)
	bool FFormExtrusionFacesTest::RunTest(const FString& Parameters)
	{
		// An iris blade: its pearl top and its gilt underside are the two faces of one extrusion.
		FFormDrawing Drawing = FFormDrawing::Of(TEXT("blade"), TEXT("blade.top"), Flag());
		Drawing.PartSurfaces.Add(FFormExtrusion::NearPart, TEXT("blade.underside"));
		const TArray<FFormMadePiece> Pieces = MakeDrawing(*this, Drawing);
		if (!TestEqual(TEXT("the blade is one piece"), Pieces.Num(), 1))
		{
			return false;
		}
		TestClosed(*this, TEXT("the blade"), Pieces[0], 2);
		TestEqual(TEXT("its top leads and its underside is finished apart"), Pieces[0].SlotSurfaces, TArray<FName>{TEXT("blade.underside")});
		TestEqual(TEXT("the underside is the whole face below"), FacingArea(Pieces[0], 1, -FVector::UpVector), 1.2 * 0.8, 1e-9);
		TestEqual(TEXT("the top keeps the face above"), FacingArea(Pieces[0], 0, FVector::UpVector), 1.2 * 0.8, 1e-9);
		TestEqual(TEXT("and the sides"), FacingArea(Pieces[0], 1, FVector::ForwardVector), 0.0, 1e-12);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormPocketTest, "Musee.Form.Drawing.Pockets", Flags)
	bool FFormPocketTest::RunTest(const FString& Parameters)
	{
		const double Slab = 1.2 * 0.8 * 0.1;
		struct FCase
		{
			const TCHAR* What;
			TArray<FFormPocket> Pockets;
			double Taken;
		};
		const FCase Cases[] = {
			{TEXT("a blind pocket in the top (a housing)"), {Pocket({0.2, 0.2}, {0.5, 0.4}, 0.03)}, 0.3 * 0.2 * 0.03},
			{TEXT("a rebate along the south edge"), {Pocket({-0.1, 0.7}, {1.3, 0.9}, 0.02)}, 1.2 * 0.1 * 0.02},
			{TEXT("a rebate round the corner"), {Pocket({0.9, -0.1}, {1.3, 0.3}, 0.04)}, 0.3 * 0.3 * 0.04},
			{TEXT("a pocket stepped in a pocket"), {Pocket({0.2, 0.2}, {0.8, 0.6}, 0.02), Pocket({0.3, 0.3}, {0.5, 0.5}, 0.05)},
				0.6 * 0.4 * 0.02 + 0.2 * 0.2 * 0.03},
			{TEXT("a pocket in the underside and one above it"), {Pocket({0.2, 0.2}, {0.5, 0.4}, 0.03, true), Pocket({0.2, 0.2}, {0.5, 0.4}, 0.03)},
				2.0 * 0.3 * 0.2 * 0.03},
		};
		for (const FCase& Case : Cases)
		{
			FFormExtrusion Shape = Flag();
			Shape.Pockets = Case.Pockets;
			FFormDrawing Drawing = FFormDrawing::Of(TEXT("flag"), TEXT("flag"), Shape);
			Drawing.PartSurfaces.Add(FFormExtrusion::PocketPart, TEXT("flag.pocket"));
			const TArray<FFormMadePiece> Pieces = MakeDrawing(*this, Drawing);
			if (Pieces.Num() != 1)
			{
				AddError(FString::Printf(TEXT("%s: not made"), Case.What));
				continue;
			}
			TestClosed(*this, Case.What, Pieces[0], 2);
			TestEqual(FString::Printf(TEXT("%s: the slab less what the pockets take"), Case.What), VolumeCubicMeters(Pieces[0]), Slab - Case.Taken, 1e-9);
			TestEqual(FString::Printf(TEXT("%s: the pockets are finished apart"), Case.What), Pieces[0].SlotSurfaces, TArray<FName>{TEXT("flag.pocket")});
		}
		// A pocket as deep as the slab has no floor: it is a hole, drawn as one.
		FFormExtrusion Through = Flag();
		Through.Pockets = {Pocket({0.2, 0.2}, {0.5, 0.4}, 0.1)};
		TArray<FMuseeFinding> Findings;
		MakeDrawing(*this, FFormDrawing::Of(TEXT("flag"), TEXT("flag"), Through), &Findings);
		TestEqual(TEXT("a pocket through the slab is a finding"), Findings.Num(), 1);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormEasedArrisTest, "Musee.Form.Drawing.EasedArrises", Flags)
	bool FFormEasedArrisTest::RunTest(const FString& Parameters)
	{
		// The Atrium stele's bronze foot: its top edges eased 6 mm, and the glass set 20 mm into a slot in its top.
		constexpr double Ease = 0.006;
		const double Box = 1.2 * 0.8 * 0.1;
		const double Perimeter = 2.0 * (1.2 + 0.8);
		for (const bool bChamfer : {false, true})
		{
			FFormExtrusion Foot = Flag();
			Foot.FarArris = bChamfer ? FFormArris::Chamfer(Ease) : FFormArris::Round(Ease);
			Foot.Pockets = {Pocket({0.1, 0.39}, {1.1, 0.41}, 0.02)};
			const FString What = bChamfer ? TEXT("chamfered") : TEXT("eased");
			const TArray<FFormMadePiece> Pieces = MakeDrawing(*this, FFormDrawing::Of(TEXT("stele.foot"), TEXT("stele.foot"), Foot));
			if (Pieces.Num() != 1)
			{
				AddError(What + TEXT(": not made"));
				continue;
			}
			TestClosed(*this, What + TEXT(" foot with its slot"), Pieces[0], 2);
			// Each edge loses the corner's square less what its chords keep of the round (half the square, chamfered), its
			// length taken along the mitred sweep's line through what it loses; the slot takes its own.
			const int32 Chords = ArcSegmentCount(Ease, UE_DOUBLE_HALF_PI, DefaultChordToleranceMeters);
			const double Kept = bChamfer ? 0.5 : 0.5 * Chords * FMath::Sin(UE_DOUBLE_HALF_PI / Chords);
			const double Section = (1.0 - Kept) * Ease * Ease;
			const double Expected = Box - Section * Perimeter - 1.0 * 0.02 * 0.02;
			TestEqual(What + TEXT(": the block less its arrises and its slot"), VolumeCubicMeters(Pieces[0]), Expected, 8.0 * Section * Ease);
			// Facing up: the top, inset by the ease, less the slot, and the slot's floor.
			TestEqual(What + TEXT(": the top is inset by the ease"), FacingArea(Pieces[0], 0, FVector::UpVector), (1.2 - 2.0 * Ease) * (0.8 - 2.0 * Ease), 1e-9);
		}
		// An eased arris wider than the slab is thick is a finding.
		FFormExtrusion Thin = Flag();
		Thin.FarArris = FFormArris::Round(0.06);
		Thin.NearArris = FFormArris::Round(0.05);
		TArray<FMuseeFinding> Findings;
		MakeDrawing(*this, FFormDrawing::Of(TEXT("thin"), TEXT("thin"), Thin), &Findings);
		TestEqual(TEXT("eases that meet across the slab are a finding"), Findings.Num(), 1);
		return true;
	}
}

#endif
