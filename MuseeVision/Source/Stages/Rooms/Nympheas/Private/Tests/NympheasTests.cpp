#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ArtHang.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "MotionSchedule.h"
#include "MuseeRecords.h"
#include "NaturePlanting.h"
#include "NympheasPlan.h"
#include "NympheasRoom.h"
#include "SiteDescription.h"
#include "SitePortClass.h"

/**
 * The oval's description against its sheet and its schedules: its ports and their classes, its anchors against every
 * anchor the schedules name, the stage contract's own check, and a description that is the same on every call. Its
 * shells are proved sealed by Studio's Musee.Studio.RoomsSealed, which makes its drawings.
 */
namespace Musee::Nympheas::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		FSiteDescription Described()
		{
			FSiteDescription Description;
			GetDefault<UNympheasRoom>()->Describe(Description);
			return Description;
		}

		const FMuseeStageAnchor* FindAnchor(const FSiteDescription& Description, const FMuseeKey& Key, FName Kind)
		{
			return Description.Anchors.FindByPredicate([&](const FMuseeStageAnchor& A) { return A.Key == Key && A.Kind == Kind; });
		}

		const FSitePort* FindPort(const FSiteDescription& Description, const TCHAR* Key)
		{
			return Description.Ports.FindByPredicate([Key](const FSitePort& P) { return P.Key == FMuseeKey(Key); });
		}

		FString Schedule(const TCHAR* File)
		{
			return FString::Printf(TEXT("Stages/%s/%s"), Plan::Stage, File);
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNympheasPortsTest, "Musee.Nympheas.Ports", Flags)
	bool FNympheasPortsTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		TestEqual(TEXT("two ports"), Description.Ports.Num(), 2);
		const FSitePort* East = FindPort(Description, TEXT("nympheas.east"));
		const FSitePort* Stair = FindPort(Description, TEXT("nympheas.stair"));
		if (!TestNotNull(TEXT("the east door"), East) || !TestNotNull(TEXT("the stair's foot"), Stair))
		{
			return false;
		}
		TestTrue(TEXT("the east door stands on the port face on the axis"), East->Frame.OriginMeters.Equals(FVector(11.9, 0.0, 0.0), 1e-9));
		TestTrue(TEXT("and faces east"), East->Facing().Equals(FVector(1.0, 0.0, 0.0), 1e-9));
		TestEqual(TEXT("it meets the Salon's west door"), East->Mates, FMuseeKey(TEXT("salon.west")));
		TestTrue(TEXT("the stair's foot stands on the port face at the Reserve's floor"), Stair->Frame.OriginMeters.Equals(FVector(11.9, 0.0, -5.8), 1e-9));
		TestTrue(TEXT("and faces east"), Stair->Facing().Equals(FVector(1.0, 0.0, 0.0), 1e-9));
		TestTrue(TEXT("the oval serves the stair's well"), Stair->bHasWell && Stair->Well.bServing);
		TestTrue(TEXT("the well is the prism under the oval"), Stair->Well.MinMeters.Equals(FVector(-4.3, -1.6, -6.15), 1e-9)
			&& Stair->Well.MaxMeters.Equals(FVector(11.9, 1.6, 0.0), 1e-9));

		// The rough openings the room builds are its ports' classes.
		const TValueOrError<FSitePortClass, FString> Door = Musee::Site::FindPortClass(East->Class);
		const TValueOrError<FSitePortClass, FString> Reserve = Musee::Site::FindPortClass(Stair->Class);
		if (TestTrue(TEXT("both classes are in the site's records"), Door.HasValue() && Reserve.HasValue()))
		{
			TestEqual(TEXT("the passage is the door's class wide"), Plan::DoorWidth, Door.GetValue().ClearWidthMeters);
			TestEqual(TEXT("and springs at its head"), Plan::DoorSpringing, Door.GetValue().HeadMeters);
			TestEqual(TEXT("the Reserve's door is the shaft's clear width"), 2.0 * Plan::ShaftHalf, Reserve.GetValue().ClearWidthMeters);
			TestEqual(TEXT("and springs at its head"), Plan::ReserveDoorSpringing, Reserve.GetValue().HeadMeters);
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNympheasAnchorsTest, "Musee.Nympheas.Anchors", Flags)
	bool FNympheasAnchorsTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		const TValueOrError<FArtHang, FString> Hang = Musee::ReadRecord<FArtHang>(Schedule(TEXT("Hang.json")));
		const TValueOrError<FMotionSchedule, FString> Motion = Musee::ReadRecord<FMotionSchedule>(Schedule(TEXT("Mechanisms.json")));
		if (!TestTrue(TEXT("the hanging and mechanisms schedules read"), Hang.HasValue() && Motion.HasValue()))
		{
			AddError(Hang.HasError() ? Hang.GetError() : Motion.GetError());
			return false;
		}
		for (const FArtHangEntry& Work : Hang.GetValue().Works)
		{
			TestNotNull(*FString::Printf(TEXT("%s hangs on a Wall anchor"), *Work.Work), FindAnchor(Description, Work.Anchor, TEXT("Wall")));
		}
		for (const FMotionMechanismSpec& Mechanism : Motion.GetValue().Mechanisms)
		{
			TestNotNull(*FString::Printf(TEXT("%s moves a Body anchor"), *Mechanism.Key.ToString()), FindAnchor(Description, Mechanism.Body, TEXT("Body")));
			if (!Mechanism.Control.IsNone())
			{
				TestNotNull(*FString::Printf(TEXT("%s is worked from a Use anchor"), *Mechanism.Key.ToString()),
					FindAnchor(Description, Mechanism.Control, TEXT("Use")));
			}
		}

		// The willows' wall starts where Hang.json's path starts, looking into the room.
		const FMuseeStageAnchor* Willows = FindAnchor(Description, FMuseeKey(TEXT("nympheas.wall.willows")), TEXT("Wall"));
		if (TestNotNull(TEXT("the willows' wall"), Willows) && TestTrue(TEXT("its path is given"), !Hang.GetValue().Walls.IsEmpty()))
		{
			const double From = FMath::DegreesToRadians(Hang.GetValue().Walls[0].Path.From_Deg);
			const FVector Start(Plan::OvalA * FMath::Cos(From), Plan::OvalB * FMath::Sin(From), 0.0);
			TestTrue(TEXT("it starts at the door's north jamb, on its path"), Willows->Frame.OriginMeters.Equals(Start, 1e-9)
				&& Willows->Frame.OriginMeters.Equals(FVector(10.778, -1.5, 0.0), 0.001));
			const FVector Out = Willows->Frame.DirectionToParent(FVector(0.0, 1.0, 0.0));
			TestTrue(TEXT("its y looks into the room"), FVector::DotProduct(Out, -Start) > 0.0 && FMath::IsNearlyZero(Out.Z));
			TestTrue(TEXT("its z is up"), Willows->Frame.DirectionToParent(FVector::UpVector).Equals(FVector::UpVector, 1e-9));
		}

		// Every seat faces out to the willows.
		int32 Seats = 0;
		for (const FMuseeStageAnchor& Anchor : Description.Anchors)
		{
			if (Anchor.Kind == FName(TEXT("Seat")))
			{
				++Seats;
				const FVector Facing = Anchor.Frame.DirectionToParent(FVector::ForwardVector);
				TestTrue(*FString::Printf(TEXT("%s faces away from the pond"), *Anchor.Key.ToString()),
					FVector::DotProduct(Facing, Anchor.Frame.OriginMeters.GetSafeNormal2D()) > 0.0);
			}
		}
		TestEqual(TEXT("four benches and the coping's seat"), Seats, 5);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNympheasPlantingAnchorsTest, "Musee.Nympheas.PlantingAnchors", Flags)
	bool FNympheasPlantingAnchorsTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		const TValueOrError<FNaturePlanting, FString> Planting = Musee::ReadRecord<FNaturePlanting>(Schedule(TEXT("Planting.json")));
		if (!TestTrue(TEXT("the planting schedule reads"), Planting.HasValue()))
		{
			AddError(Planting.GetError());
			return false;
		}
		for (const FNatureWater& Water : Planting.GetValue().Water)
		{
			TestNotNull(*FString::Printf(TEXT("%s has its Water anchor"), *Water.Key.ToString()), FindAnchor(Description, Water.Anchor, TEXT("Water")));
			TestTrue(TEXT("and rides a Body anchor"), Water.Body.IsNone() || FindAnchor(Description, Water.Body, TEXT("Body")) != nullptr);
		}
		for (const FNatureBed& Bed : Planting.GetValue().Beds)
		{
			TestNotNull(*FString::Printf(TEXT("%s is a Bed anchor"), *Bed.Bed.ToString()), FindAnchor(Description, Bed.Bed, TEXT("Bed")));
			TestTrue(TEXT("and rides a Body anchor"), Bed.Body.IsNone() || FindAnchor(Description, Bed.Body, TEXT("Body")) != nullptr);
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNympheasDescribedTest, "Musee.Nympheas.Described", Flags)
	bool FNympheasDescribedTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		TestEqual(TEXT("it is the nympheas stage"), Description.Stage, FMuseeKey(TEXT("nympheas")));
		const TArray<FMuseeFinding> Findings = Musee::Site::CheckDescription(Description);
		for (const FMuseeFinding& Finding : Findings)
		{
			AddError(Finding.Summary);
		}
		TestEqual(TEXT("the stage contract holds"), Findings.Num(), 0);
		TestEqual(TEXT("two enclosures"), Description.Enclosures.Num(), 2);
		TestTrue(TEXT("the dome is its one opening to the sky"), Description.Openings.ContainsByPredicate([](const FFormOpening& O)
		{
			return O.Key == FMuseeKey(TEXT("nympheas.roof")) && O.Kind == EFormOpeningKind::Glazed && O.Glazing == FName(TEXT("nympheas.roof.glass"));
		}));
		TestTrue(TEXT("the footprint is a closed outline"), Description.Footprint.OutlineMeters.Num() > Plan::OvalStations / 2);

		// Every surface a piece names is in the stage's finish schedule.
		FString Finishes;
		if (TestTrue(TEXT("the finish schedule reads"), FFileHelper::LoadFileToString(Finishes, *(Musee::Records::DataRoot() / Schedule(TEXT("Finishes.json"))))))
		{
			for (const FFormDrawing& Piece : Description.Pieces)
			{
				TArray<FName> Surfaces;
				Piece.PartSurfaces.GenerateValueArray(Surfaces);
				Surfaces.Add(Piece.Surface);
				for (const FName Surface : Surfaces)
				{
					const FString Entry = FString::Printf(TEXT("\"surface\": \"%s\""), *Surface.ToString());
					TestTrue(*FString::Printf(TEXT("%s's surface %s is scheduled"), *Piece.Name.ToString(), *Surface.ToString()), Finishes.Contains(Entry));
				}
			}
		}

		// The description is a plain function of the plan: the same on every call.
		const FSiteDescription Again = Described();
		TestEqual(TEXT("the same pieces"), Again.Pieces.Num(), Description.Pieces.Num());
		bool bSame = Again.Pieces.Num() == Description.Pieces.Num();
		for (int32 Index = 0; bSame && Index < Again.Pieces.Num(); ++Index)
		{
			bSame = Again.Pieces[Index].Name == Description.Pieces[Index].Name && Again.Pieces[Index].Frame.OriginMeters == Description.Pieces[Index].Frame.OriginMeters;
		}
		TestTrue(TEXT("in the same order and places"), bSame);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNympheasPlanTest, "Musee.Nympheas.Plan", Flags)
	bool FNympheasPlanTest::RunTest(const FString& Parameters)
	{
		// The sheet's measured numbers, from the plan's setting-out.
		const FSiteDescription Description = Described();
		const TArray<FVector2D>& Outline = Description.Footprint.OutlineMeters;
		TestTrue(TEXT("the footprint starts at the passage block's south-east corner"), Outline[0].Equals(FVector2D(11.9, 2.1), 1e-9));
		TestTrue(TEXT("and meets the outer face at x 11.207"), Outline[1].Equals(FVector2D(11.207, 2.1), 0.001));
		double West = 0.0;
		for (const FVector2D& Point : Outline)
		{
			West = FMath::Min(West, Point.X);
		}
		TestEqual(TEXT("the outer face passes x -11.60 at the west apex"), West, -11.6, 1e-6);
		return true;
	}
}

#endif
