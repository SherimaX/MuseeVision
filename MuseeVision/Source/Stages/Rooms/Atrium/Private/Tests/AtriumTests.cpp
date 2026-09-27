#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ArtHang.h"
#include "AtriumParts.h"
#include "AtriumPlan.h"
#include "AtriumRoom.h"
#include "Misc/FileHelper.h"
#include "MotionSchedule.h"
#include "MuseeRecords.h"
#include "SiteDescription.h"
#include "SitePortClass.h"

/**
 * The Atrium's description against its sheet and its schedules: its ports and their classes, its anchors against the
 * sheet's table and every anchor the schedules name, the stage contract's own check, the sheet's measured numbers, and a
 * description that is the same on every call. Its shells are proved sealed by Studio's Musee.Studio.RoomsSealed.
 */
namespace Musee::Atrium::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		/** The sheet's numbers are given to the millimetre. */
		constexpr double Millimetre = 0.001;

		/** Half the thousandth of a degree the schedules give their angles to. */
		constexpr double ScheduleHalfDegree = 0.0005;

		FSiteDescription Described()
		{
			FSiteDescription Description;
			GetDefault<UAtriumRoom>()->Describe(Description);
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

		int32 CountPieces(const FSiteDescription& Description, const TCHAR* Prefix)
		{
			int32 Count = 0;
			for (const FFormDrawing& Piece : Description.Pieces)
			{
				Count += Piece.Name.ToString().StartsWith(Prefix, ESearchCase::CaseSensitive) ? 1 : 0;
			}
			return Count;
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAtriumPortsTest, "Musee.Atrium.Ports", Flags)
	bool FAtriumPortsTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		TestEqual(TEXT("three ports"), Description.Ports.Num(), 3);
		const FSitePort* West = FindPort(Description, TEXT("atrium.west"));
		const FSitePort* Down = FindPort(Description, TEXT("atrium.down"));
		const FSitePort* Up = FindPort(Description, TEXT("atrium.up"));
		if (!TestNotNull(TEXT("the west door"), West) || !TestNotNull(TEXT("the down port"), Down) || !TestNotNull(TEXT("the up port"), Up))
		{
			return false;
		}
		TestTrue(TEXT("the west door stands on the base's face on the axis"), West->Frame.OriginMeters.Equals(FVector(-14.8, 0.0, 0.0), 1e-9));
		TestTrue(TEXT("and faces west"), West->Facing().Equals(FVector(-1.0, 0.0, 0.0), 1e-9));
		TestEqual(TEXT("through the base's 0.800"), West->WallDepthMeters, 0.8, 1e-9);
		TestEqual(TEXT("on its convex face R 14.800"), West->FaceRadiusMeters, 14.8, 1e-9);
		TestEqual(TEXT("it meets the Hall of Light's east port"), West->Mates, FMuseeKey(TEXT("hall-of-light.east")));

		TestTrue(TEXT("the down port lies on the slab's soffit"), Down->Frame.OriginMeters.Equals(FVector(0.0, 0.0, -0.5), 1e-9));
		TestTrue(TEXT("and faces down"), Down->Facing().Equals(FVector(0.0, 0.0, -1.0), 1e-9));
		TestEqual(TEXT("it meets the Cube's up port"), Down->Mates, FMuseeKey(TEXT("cube.up")));
		TestTrue(TEXT("the up port lies on the collar's top"), Up->Frame.OriginMeters.Equals(FVector(0.0, 0.0, 7.715), 1e-9));
		TestTrue(TEXT("and faces up"), Up->Facing().Equals(FVector(0.0, 0.0, 1.0), 1e-9));
		TestEqual(TEXT("it meets the Sphere's down port"), Up->Mates, FMuseeKey(TEXT("sphere.down")));
		for (const FSitePort* Port : {Down, Up})
		{
			TestEqual(*FString::Printf(TEXT("%s is an iris.4600"), *Port->Key.ToString()), Port->Class, FMuseeKey(TEXT("iris.4600")));
			TestTrue(*FString::Printf(TEXT("the Atrium serves %s's well"), *Port->Key.ToString()), Port->bHasWell && Port->Well.bServing);
		}
		TestTrue(TEXT("the well below runs to -23.800"), Down->Well.MinMeters.Equals(FVector(-2.3, -2.3, -23.8), 1e-9)
			&& Down->Well.MaxMeters.Equals(FVector(2.3, 2.3, -0.5), 1e-9));
		TestTrue(TEXT("the well above runs to 23.390"), Up->Well.MinMeters.Equals(FVector(-2.3, -2.3, 7.715), 1e-9)
			&& Up->Well.MaxMeters.Equals(FVector(2.3, 2.3, 23.39), 1e-9));

		// The rough opening the Atrium builds is the class's opening and 0.050 round it; the class is Site's record.
		const TValueOrError<FSitePortClass, FString> Door = Musee::Site::FindPortClass(West->Class);
		if (Door.HasValue())
		{
			TestEqual(TEXT("the rough opening is the class's clear width and 0.050 each side"), 2.0 * Plan::RoughHalf, Door.GetValue().ClearWidthMeters + 0.1, 1e-9);
			TestEqual(TEXT("and its head 0.050 over the class's"), Plan::RoughHead, Door.GetValue().HeadMeters + 0.05, 1e-9);
		}
		else
		{
			AddWarning(FString::Printf(TEXT("Site's records have no %s yet (KIT-2): %s"), *West->Class.ToString(), *Door.GetError()));
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAtriumAnchorsTest, "Musee.Atrium.Anchors", Flags)
	bool FAtriumAnchorsTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();

		// The sheet's table (§2): every body, control and wall.
		const TCHAR* const Bodies[] = {TEXT("atrium.car"), TEXT("atrium.car.mast.head"), TEXT("atrium.car.leaf.north"), TEXT("atrium.car.leaf.south"),
			TEXT("atrium.landing.leaf.north"), TEXT("atrium.landing.leaf.south")};
		for (const TCHAR* Body : Bodies)
		{
			TestNotNull(*FString::Printf(TEXT("%s is a Body"), Body), FindAnchor(Description, FMuseeKey(Body), TEXT("Body")));
		}
		const FVector Pins[] = {{3.745, 0.0, 7.695}, {2.335, 2.928, 7.691}, {-0.833, 3.651, 7.687}, {-3.374, 1.625, 7.683}, {-3.374, -1.625, 7.679},
			{-0.833, -3.651, 7.675}, {2.335, -2.928, 7.671}};
		for (int32 Blade = 0; Blade < Plan::IrisBlades; ++Blade)
		{
			const FMuseeStageAnchor* Anchor = FindAnchor(Description, IrisBladeBody(Blade), TEXT("Body"));
			if (TestNotNull(*FString::Printf(TEXT("blade %d is a Body"), Blade + 1), Anchor))
			{
				TestTrue(*FString::Printf(TEXT("blade %d turns on its pin"), Blade + 1), Anchor->Frame.OriginMeters.Equals(Pins[Blade], Millimetre));
			}
		}
		const FMuseeStageAnchor* Head = FindAnchor(Description, FMuseeKey(TEXT("atrium.car.mast.head")), TEXT("Body"));
		TestTrue(TEXT("the mast's head rides on the crown's top"), Head && Head->Frame.OriginMeters.Equals(FVector(0.0, 0.0, 2.78), 1e-9));
		for (const TCHAR* Riding : {TEXT("atrium.car.leaf.north"), TEXT("atrium.car.leaf.south"), TEXT("atrium.car.mast.head")})
		{
			const FMuseeKey* Carrier = Description.BodyParents.Find(FMuseeKey(Riding));
			TestTrue(*FString::Printf(TEXT("%s rides the car"), Riding), Carrier && *Carrier == FMuseeKey(TEXT("atrium.car")));
		}
		TestFalse(TEXT("the landing's leaves stay on the stage's frame"), Description.BodyParents.Contains(FMuseeKey(TEXT("atrium.landing.leaf.north"))));
		const FSiteBarrier* Barrier = Description.Barriers.FindByPredicate([](const FSiteBarrier& B) { return B.Key == FMuseeKey(TEXT("atrium.stele.barrier")); });
		TestTrue(TEXT("the stele's barrier is the box of its foot's plan up to 2.200 (section 3.7)"), Barrier
			&& Barrier->Frame.OriginMeters.Equals(FVector(-3.81, -1.9, 1.1), 1e-9) && Barrier->HalfSizeMeters.Equals(FVector(0.2, 0.85, 1.1), 1e-9));
		const FMuseeStageAnchor* Call = FindAnchor(Description, FMuseeKey(TEXT("atrium.call")), TEXT("Use"));
		TestTrue(TEXT("the call is at the waiting mark"), Call && Call->Frame.OriginMeters.Equals(FVector(-3.31, 0.0, 0.0), 1e-9));
		TestNotNull(TEXT("the car's panel is a Use"), FindAnchor(Description, FMuseeKey(TEXT("atrium.car.panel")), TEXT("Use")));
		const FMuseeStageAnchor* Stele = FindAnchor(Description, FMuseeKey(TEXT("atrium.stele")), TEXT("Wall"));
		if (TestNotNull(TEXT("the stele is a Wall"), Stele))
		{
			TestTrue(TEXT("at its glass's west face, north end"), Stele->Frame.OriginMeters.Equals(FVector(-3.84, -2.65, 0.0), 1e-9));
			TestTrue(TEXT("x runs south"), Stele->Frame.DirectionToParent(FVector::ForwardVector).Equals(FVector(0.0, 1.0, 0.0), 1e-9));
			TestTrue(TEXT("y looks west to the viewer"), Stele->Frame.DirectionToParent(FVector::RightVector).Equals(FVector(-1.0, 0.0, 0.0), 1e-9));
			TestTrue(TEXT("z is up"), Stele->Frame.DirectionToParent(FVector::UpVector).Equals(FVector::UpVector, 1e-9));
		}

		// Every anchor the hanging names, and every bay wall where its path starts.
		const TValueOrError<FArtHang, FString> Hang = Musee::ReadRecord<FArtHang>(Schedule(TEXT("Hang.json")));
		if (!TestTrue(TEXT("the hanging schedule reads"), Hang.HasValue()))
		{
			AddError(Hang.GetError());
			return false;
		}
		for (const FArtHangEntry& Work : Hang.GetValue().Works)
		{
			TestNotNull(*FString::Printf(TEXT("%s hangs on a Wall anchor"), *Work.Work), FindAnchor(Description, Work.Anchor, TEXT("Wall")));
		}
		TestEqual(TEXT("eleven bays"), Hang.GetValue().Walls.Num(), Plan::Bays);
		for (const FArtWall& Wall : Hang.GetValue().Walls)
		{
			const FMuseeStageAnchor* Anchor = FindAnchor(Description, Wall.Anchor, TEXT("Wall"));
			if (!TestNotNull(*FString::Printf(TEXT("%s is a Wall"), *Wall.Anchor.ToString()), Anchor))
			{
				continue;
			}
			// The schedule gives its angle to a thousandth of a degree, so to half of one: 0.12 mm on the base's inner face.
			const double Given = Wall.Path.Ellipse[0] * FMath::DegreesToRadians(ScheduleHalfDegree);
			const double From = FMath::DegreesToRadians(Wall.Path.From_Deg);
			const FVector Start(Wall.Path.Ellipse[0] * FMath::Cos(From), Wall.Path.Ellipse[1] * FMath::Sin(From), 0.0);
			TestTrue(*FString::Printf(TEXT("%s starts where its path does"), *Wall.Anchor.ToString()),
				Anchor->Frame.OriginMeters.Equals(Start, Given));
			const FVector In = Anchor->Frame.DirectionToParent(FVector::RightVector);
			TestTrue(*FString::Printf(TEXT("%s looks at the axis"), *Wall.Anchor.ToString()), In.Equals(-Start.GetSafeNormal(), Given / Wall.Path.Ellipse[0]));
		}

		// Every body and control the mechanisms name, as Motion reads them.
		const TValueOrError<FMotionSchedule, FString> Motion = FMotionSchedule::Read(Schedule(TEXT("Mechanisms.json")));
		if (!TestTrue(TEXT("the mechanisms schedule reads"), Motion.HasValue()))
		{
			AddError(Motion.GetError());
			return false;
		}
		TestEqual(TEXT("thirteen mechanisms"), Motion.GetValue().Mechanisms.Num(), 13);
		for (const FMotionMechanismSpec& Mechanism : Motion.GetValue().Mechanisms)
		{
			TestNotNull(*FString::Printf(TEXT("%s moves a Body anchor"), *Mechanism.Key.ToString()), FindAnchor(Description, Mechanism.Body, TEXT("Body")));
			if (!Mechanism.Control.IsNone())
			{
				TestNotNull(*FString::Printf(TEXT("%s is worked from a Use anchor"), *Mechanism.Key.ToString()),
					FindAnchor(Description, Mechanism.Control, TEXT("Use")));
			}
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAtriumDescribedTest, "Musee.Atrium.Described", Flags)
	bool FAtriumDescribedTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		TestEqual(TEXT("it is the atrium stage"), Description.Stage, FMuseeKey(TEXT("atrium")));
		const TArray<FMuseeFinding> Findings = Musee::Site::CheckDescription(Description);
		for (const FMuseeFinding& Finding : Findings)
		{
			AddError(Finding.Summary);
		}
		TestEqual(TEXT("the stage contract holds"), Findings.Num(), 0);
		TestEqual(TEXT("one enclosure, the hall"), Description.Enclosures.Num(), 1);
		TestTrue(TEXT("the hall is bounded by its floor, base, drum, canopy, collar and landing"), Description.Enclosures.Num() == 1
			&& Description.Enclosures[0].Pieces.Num() > 300);
		TestTrue(TEXT("the drum is its one opening to the sky"), Description.Openings.ContainsByPredicate([](const FFormOpening& O)
		{
			return O.Key == FMuseeKey(TEXT("atrium.drum")) && O.Kind == EFormOpeningKind::Glazed;
		}));
		const int32 Glazed = Description.Openings.FilterByPredicate([](const FFormOpening& O) { return O.Kind == EFormOpeningKind::Glazed; }).Num();
		TestEqual(TEXT("and the only glazed one: the canopy is a luminaire's diffuser under the opaque Sphere"), Glazed, 1);

		// Every surface a piece names is in the stage's finish schedule, and every moving piece rides a declared body.
		FString Finishes;
		if (TestTrue(TEXT("the finish schedule reads"), FFileHelper::LoadFileToString(Finishes, *(Musee::Records::DataRoot() / Schedule(TEXT("Finishes.json"))))))
		{
			for (const FFormDrawing& Piece : Description.Pieces)
			{
				const FString Entry = FString::Printf(TEXT("\"surface\": \"%s\""), *Piece.Surface.ToString());
				TestTrue(*FString::Printf(TEXT("%s's surface %s is scheduled"), *Piece.Name.ToString(), *Piece.Surface.ToString()), Finishes.Contains(Entry));
				if (!Piece.Body.IsNone())
				{
					TestNotNull(*FString::Printf(TEXT("%s rides a Body"), *Piece.Name.ToString()), FindAnchor(Description, Piece.Body, TEXT("Body")));
				}
			}
		}

		// §3.4: the canopy's glass is cut on its grid: 72 panes up to the drum, 144 in each band between 72-node rings (13), 108
		// where 72 become 36, and 72 in each band between 36-node rings (5), each bent onto the Sphere's seam.
		const TArray<FFormDrawing> Panes = Description.Pieces.FilterByPredicate([](const FFormDrawing& D)
		{
			return D.Name.ToString().StartsWith(TEXT("canopy.glass.band-"));
		});
		TestEqual(TEXT("2,412 panes"), Panes.Num(), 72 + 13 * 144 + 108 + 5 * 72);
		TestTrue(TEXT("each on the Sphere's seam, R 14.000"), Panes.Num() > 0 && Panes[0].Ground.Kind == EFormGroundKind::Sphere
			&& FMath::IsNearlyEqual(Panes[0].Ground.RadiusMeters, 14.0));
		// §3.2: the portal's arch runs straight through its block and its upper course, springing 5.221 under a crown of 8.000.
		const FFormDrawing* Front = Description.Pieces.FindByPredicate([](const FFormDrawing& D) { return D.Name == FName(TEXT("portal.front")); });
		if (TestTrue(TEXT("the portal is a block with its opening"), Front && Front->Shape.IsType<FFormWall>() && Front->Shape.Get<FFormWall>().Openings.Num() == 1))
		{
			const FFormWall& Block = Front->Shape.Get<FFormWall>();
			TestEqual(TEXT("its arch's crown at 8.000"), Block.BaseMeters + Block.Openings[0].CrownMeters(), 8.0, Millimetre);
			TestTrue(TEXT("a segmental arch, straight through"), Block.Openings[0].bStraight && Block.Openings[0].ArchRadiusMeters == Plan::PortalArchRadius);
		}

		// The description is a plain function of the plan: the same on every call.
		const FSiteDescription Again = Described();
		bool bSame = Again.Pieces.Num() == Description.Pieces.Num();
		for (int32 Index = 0; bSame && Index < Again.Pieces.Num(); ++Index)
		{
			bSame = Again.Pieces[Index].Name == Description.Pieces[Index].Name && Again.Pieces[Index].Frame.OriginMeters == Description.Pieces[Index].Frame.OriginMeters;
		}
		TestTrue(TEXT("the same pieces, in the same order and places"), bSame);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAtriumPlanTest, "Musee.Atrium.Plan", Flags)
	bool FAtriumPlanTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();

		// §1: the footprint from the portal's front round the arc R 15.400.
		const TArray<FVector2D>& Outline = Description.Footprint.OutlineMeters;
		TestTrue(TEXT("the footprint starts at G1"), Outline[0].Equals(FVector2D(-15.0, -6.05), 1e-9));
		TestTrue(TEXT("and meets the arc at G2 (-14.162, -6.050)"), Outline[1].Equals(FVector2D(-14.162, -6.05), Millimetre));
		TestTrue(TEXT("and ends at G4"), Outline.Last().Equals(FVector2D(-15.0, 6.05), 1e-9));

		// §2: the class's jambs meet the port face at x -14.664 and the inner face at x -13.856.
		const double Jamb = 0.5 * Plan::DoorClear;
		TestEqual(TEXT("the jambs meet the port face at x -14.664"), -FMath::Sqrt(FMath::Square(Plan::BaseOuter) - Jamb * Jamb), -14.664, Millimetre);
		TestEqual(TEXT("and the inner face at x -13.856"), -FMath::Sqrt(FMath::Square(Plan::BaseInner) - Jamb * Jamb), -13.856, Millimetre);

		// §3.2: the portal's arch springs from its jambs at 5.221 and crowns at 8.000.
		const double Springing = Plan::PortalArchCentreZ + FMath::Sqrt(FMath::Square(Plan::PortalArchRadius) - FMath::Square(Plan::PortalJamb));
		TestEqual(TEXT("the portal's arch springs at 5.221"), Springing, 5.221, 2.0 * Millimetre);
		TestEqual(TEXT("and crowns at 8.000"), Plan::PortalArchCentreZ + Plan::PortalArchRadius, 8.0, 1e-9);

		// §2: each bay runs 7.030 on the base's inner face between its ribs' side faces.
		const double Bay = FMath::DegreesToRadians(Plan::BayStep) - 2.0 * FMath::Asin(Plan::RibHalf / Plan::BaseInner);
		TestEqual(TEXT("a bay is 7.030 between its ribs"), Plan::BaseInner * Bay, 7.03, Millimetre);

		// §3.4, §3.5: the canopy meets the drum at 21.180; the rib's turn runs on tangent into the canopy at (9.057, 11.293).
		TestEqual(TEXT("the canopy's room face meets the drum at 21.180"),
			Plan::SphereCentreZ - FMath::Sqrt(FMath::Square(Plan::CanopyRadius) - FMath::Square(Plan::BaseInner)), 21.18, Millimetre);
		const double Turn = FMath::DegreesToRadians(Plan::RibTurnEndDeg);
		const FVector2D TurnEnd(Plan::RibTurnCentreR + Plan::RibTurnRadius * FMath::Cos(Turn), Plan::RibTurnZ + Plan::RibTurnRadius * FMath::Sin(Turn));
		TestTrue(TEXT("the rib's turn ends at (9.057, 11.293)"), TurnEnd.Equals(FVector2D(9.057, 11.293), Millimetre));
		TestEqual(TEXT("on the canopy's room face"), FVector2D::Distance(TurnEnd, FVector2D(0.0, Plan::SphereCentreZ)), Plan::CanopyRadius, Millimetre);
		TestEqual(TEXT("the rib reaches the ring at 75.125° under the Sphere"),
			FMath::RadiansToDegrees(FMath::Acos(Plan::RingOuter / Plan::CanopyRadius)), 75.125, 0.01);
		for (int32 Ring = 0; Ring < int32(UE_ARRAY_COUNT(Plan::GridRings)); ++Ring)
		{
			const double Off = FVector2D::Distance(FVector2D(Plan::GridRings[Ring][0], Plan::GridRings[Ring][1]), FVector2D(0.0, Plan::SphereCentreZ));
			TestEqual(*FString::Printf(TEXT("grid ring %d lies on the room face"), Ring + 1), Off, Plan::CanopyRadius, Millimetre);
		}

		// §3.3, §3.4, §3.5: the counts the sheet gives.
		TestEqual(TEXT("228 marble slabs"), CountPieces(Description, TEXT("floor.ring-")), 228);
		TestEqual(TEXT("96 panes in the inner leaf"), CountPieces(Description, TEXT("drum.inner.")), 96);
		TestEqual(TEXT("twelve ribs, each with its shoe"), CountPieces(Description, TEXT("rib.")), 24);
		TestEqual(TEXT("twelve fins, pedestals and mullions"), CountPieces(Description, TEXT("order.fin.")) + CountPieces(Description, TEXT("order.pedestal."))
			+ CountPieces(Description, TEXT("order.mullion.")), 36);
		TestEqual(TEXT("seven iris blades"), CountPieces(Description, TEXT("collar.iris.blade.")), 7);

		// §6 clearances: the car's head (2.940) at the neck stop (8.400) clears the collar iris, and the car's rings clear the lip.
		TestTrue(TEXT("the car's rings clear the floor's lip by 55 mm"), FMath::IsNearlyEqual(Plan::RingRadii[0] - Plan::CarRingOut, 0.055, 1e-9));
		TestTrue(TEXT("the guides clear the lining's bore by 10 mm"), FMath::IsNearlyEqual(Plan::LiningInner - Plan::GuideOut, 0.010, 1e-9));
		const double HeadTop = Plan::CrownTop + Plan::MastHead[6].B;
		TestEqual(TEXT("the car's highest point is 2.940 over its floor"), HeadTop, 2.94, 1e-9);
		return true;
	}
}

#endif
