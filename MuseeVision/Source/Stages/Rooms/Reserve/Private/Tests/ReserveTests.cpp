#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ArtHang.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "MotionSchedule.h"
#include "MuseeRecords.h"
#include "ReserveParts.h"
#include "ReservePlan.h"
#include "ReserveRoom.h"
#include "SiteDescription.h"
#include "SitePortClass.h"

/**
 * The undercroft's description against its sheet and its schedules: its port and its class, its anchors against every
 * anchor the schedules name, the stage contract's own check, a description that is the same on every call, and the
 * sheet's setting-out and clearances. Its shells are proved sealed by Studio's Musee.Studio.RoomsSealed, which makes its
 * drawings.
 */
namespace Musee::Reserve::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		FSiteDescription Described()
		{
			FSiteDescription Description;
			GetDefault<UReserveRoom>()->Describe(Description);
			return Description;
		}

		const FMuseeStageAnchor* FindAnchor(const FSiteDescription& Description, const FMuseeKey& Key, FName Kind)
		{
			return Description.Anchors.FindByPredicate([&](const FMuseeStageAnchor& A) { return A.Key == Key && A.Kind == Kind; });
		}

		FString Schedule(const TCHAR* File)
		{
			return FString::Printf(TEXT("Stages/%s/%s"), Plan::Stage, File);
		}

		int32 CountOf(const FSiteDescription& Description, const TCHAR* Kind)
		{
			return Description.Anchors.FilterByPredicate([Kind](const FMuseeStageAnchor& A) { return A.Kind == FName(Kind); }).Num();
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReservePortsTest, "Musee.Reserve.Ports", Flags)
	bool FReservePortsTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		if (!TestEqual(TEXT("one port"), Description.Ports.Num(), 1))
		{
			return false;
		}
		const FSitePort& Stair = Description.Ports[0];
		TestEqual(TEXT("it is the stair's foot"), Stair.Key, FMuseeKey(TEXT("reserve.stair")));
		TestTrue(TEXT("on the west wall's outer face, on the axis, at the floor"), Stair.Frame.OriginMeters.Equals(FVector(-30.6, 0.0, 0.0), 1e-9));
		TestTrue(TEXT("facing west"), Stair.Facing().Equals(FVector(-1.0, 0.0, 0.0), 1e-9));
		TestEqual(TEXT("it meets the oval's stair"), Stair.Mates, FMuseeKey(TEXT("nympheas.stair")));
		TestTrue(TEXT("it receives the oval's well"), Stair.bHasWell && !Stair.Well.bServing);
		TestTrue(TEXT("the well is the prism under the oval, museum x -90.80…-74.60, z -6.15…0.00"),
			Stair.Well.MinMeters.Equals(FVector(-46.8, -1.6, -0.35), 1e-9) && Stair.Well.MaxMeters.Equals(FVector(-30.6, 1.6, 5.8), 1e-9));

		const TValueOrError<FSitePortClass, FString> Class = Musee::Site::FindPortClass(Stair.Class);
		if (TestTrue(TEXT("its class is in the site's records"), Class.HasValue()))
		{
			TestEqual(TEXT("the door is the class's width"), Plan::DoorWidth, Class.GetValue().ClearWidthMeters);
			TestEqual(TEXT("and springs at its head"), Plan::DoorSpringing, Class.GetValue().HeadMeters);
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReserveAnchorsTest, "Musee.Reserve.Anchors", Flags)
	bool FReserveAnchorsTest::RunTest(const FString& Parameters)
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
			TestTrue(*FString::Printf(TEXT("%s hangs on a Wall or lies on a Mount anchor"), *Work.Work),
				FindAnchor(Description, Work.Anchor, TEXT("Wall")) != nullptr || FindAnchor(Description, Work.Anchor, TEXT("Mount")) != nullptr);
			// Anchors on a rack's body ride it (§2), and so does every work hung on them: a rack's face is keyed under its body.
			const FMuseeStageAnchor* Rack = Description.Anchors.FindByPredicate([&Work](const FMuseeStageAnchor& A)
			{
				return A.Kind == FName(TEXT("Body")) && Work.Anchor.ToString().StartsWith(A.Key.ToString() + TEXT("."));
			});
			TestEqual(*FString::Printf(TEXT("%s rides the body its wall is on"), *Work.Work), Work.Body, Rack ? Rack->Key : FMuseeKey());
			// Works hang on the mesh by S-hooks (§3.7): every face a work hangs on is a mesh of the plan.
			TestTrue(*FString::Printf(TEXT("%s hangs on its rack's mesh"), *Work.Work), !Rack
				|| Hang.GetValue().Meshes.ContainsByPredicate([&Work](const FArtMesh& Mesh) { return Mesh.Anchor == Work.Anchor; }));
		}
		// Each mesh is the rack's own wires as its drawing lays them: level wires on the rack's west side, uprights on its
		// east, Ø 3.0 at 25 mm, the works' back plane 0.012 off the centre plane.
		const double R = 0.5 * Plan::WireDiameter;
		const double Middle = 0.5 * (Plan::KickRailTop + Plan::TopRailBottom);
		const double Levels = FMath::FloorToDouble((0.5 * (Plan::TopRailBottom - Plan::KickRailTop) - R) / Plan::WirePitch) * Plan::WirePitch;
		const double Uprights = FMath::FloorToDouble((Plan::RailEnd - R) / Plan::WirePitch) * Plan::WirePitch;
		const double Reach = Plan::PostY + 0.5 * Plan::PostAlong;
		TestEqual(TEXT("a mesh for each hung face"), Hang.GetValue().Meshes.Num(), int32(UE_ARRAY_COUNT(Plan::LitFaces)));
		for (const FArtMesh& Mesh : Hang.GetValue().Meshes)
		{
			const FString Face = Mesh.Anchor.ToString();
			TestNotNull(*FString::Printf(TEXT("%s is a hung face"), *Face), FindAnchor(Description, Mesh.Anchor, TEXT("Wall")));
			const double West = Face.EndsWith(TEXT(".west")) ? 1.0 : -1.0;
			TestTrue(*FString::Printf(TEXT("%s's level wires are the rack's"), *Face), FMath::IsNearlyEqual(Mesh.Levels.From, Middle - Levels, 1e-9)
				&& FMath::IsNearlyEqual(Mesh.Levels.To, Middle + Levels, 1e-9) && Mesh.Levels.Pitch == Plan::WirePitch);
			TestTrue(*FString::Printf(TEXT("%s's uprights are the rack's"), *Face), FMath::IsNearlyEqual(Mesh.Uprights.From, Reach - Uprights, 1e-9)
				&& FMath::IsNearlyEqual(Mesh.Uprights.To, Reach + Uprights, 1e-9) && Mesh.Uprights.Pitch == Plan::WirePitch);
			TestEqual(*FString::Printf(TEXT("%s's level wires lie behind the works' backs"), *Face), Mesh.Behind, Plan::WorksBack - West * R, 1e-9);
			TestEqual(*FString::Printf(TEXT("%s's wire"), *Face), Mesh.Wire, Plan::WireDiameter);
		}
		TestEqual(TEXT("36 racks"), Motion.GetValue().Mechanisms.Num(), 36);
		for (const FMotionMechanismSpec& Mechanism : Motion.GetValue().Mechanisms)
		{
			const FMuseeStageAnchor* Body = FindAnchor(Description, Mechanism.Body, TEXT("Body"));
			const FMuseeStageAnchor* Use = FindAnchor(Description, Mechanism.Control, TEXT("Use"));
			if (!TestNotNull(*FString::Printf(TEXT("%s moves a Body anchor"), *Mechanism.Key.ToString()), Body)
				|| !TestNotNull(*FString::Printf(TEXT("%s is worked from a Use anchor"), *Mechanism.Key.ToString()), Use))
			{
				continue;
			}
			// Each rack is drawn out towards the nave, and its pull faces the nave 6.592 off the axis at home.
			const FVector Towards(0.0, -FMath::Sign(Body->Frame.OriginMeters.Y), 0.0);
			TestTrue(*FString::Printf(TEXT("%s runs towards the nave"), *Mechanism.Key.ToString()), FVector(Mechanism.Axis[0], Mechanism.Axis[1],
				Mechanism.Axis[2]).Equals(Towards, 1e-6));
			TestEqual(*FString::Printf(TEXT("%s's pull stands 6.592 off the axis"), *Mechanism.Key.ToString()), FMath::Abs(Use->Frame.OriginMeters.Y), 6.592,
				1e-9);
			TestTrue(*FString::Printf(TEXT("%s faces the nave"), *Mechanism.Key.ToString()),
				Use->Frame.DirectionToParent(FVector::ForwardVector).Equals(Towards, 1e-9));
			// While its guard holds for a visitor in its way, the rack says so in the sheet's words (§6).
			const FString Name = Mechanism.OpenUse.Verb.Source.RightChop(FString(TEXT("Draw out screen ")).Len());
			TestEqual(*FString::Printf(TEXT("%s's guard says why it waits"), *Mechanism.Key.ToString()), Mechanism.GuardCaption.Source,
				FString::Printf(TEXT("Screen %s waits: you are in its way"), *Name));
		}

		// The sheet's Wall anchors (§2), to the millimetre.
		const FMuseeStageAnchor* AWest = FindAnchor(Description, FMuseeKey(TEXT("reserve.screen-a.west")), TEXT("Wall"));
		const FMuseeStageAnchor* BEast = FindAnchor(Description, FMuseeKey(TEXT("reserve.screen-b.east")), TEXT("Wall"));
		const FMuseeStageAnchor* Easel = FindAnchor(Description, FMuseeKey(TEXT("reserve.easel")), TEXT("Wall"));
		if (TestNotNull(TEXT("A's west face"), AWest) && TestNotNull(TEXT("B's east face"), BEast) && TestNotNull(TEXT("the easel"), Easel))
		{
			TestTrue(TEXT("A west starts at (-28.162, -11.650)"), AWest->Frame.OriginMeters.Equals(FVector(-28.162, -11.65, 0.0), 1e-9));
			TestTrue(TEXT("and runs south, looking west"), AWest->Frame.DirectionToParent(FVector::ForwardVector).Equals(FVector(0.0, 1.0, 0.0), 1e-9)
				&& AWest->Frame.DirectionToParent(FVector::RightVector).Equals(FVector(-1.0, 0.0, 0.0), 1e-9));
			TestTrue(TEXT("B east starts at (-26.238, -6.650)"), BEast->Frame.OriginMeters.Equals(FVector(-26.238, -6.65, 0.0), 1e-9));
			TestTrue(TEXT("and runs north, looking east"), BEast->Frame.DirectionToParent(FVector::ForwardVector).Equals(FVector(0.0, -1.0, 0.0), 1e-9)
				&& BEast->Frame.DirectionToParent(FVector::RightVector).Equals(FVector(1.0, 0.0, 0.0), 1e-9));
			TestTrue(TEXT("the easel's face looks west from x 28.945"), Easel->Frame.OriginMeters.Equals(FVector(28.945, -1.0, 0.0), 1e-9)
				&& Easel->Frame.DirectionToParent(FVector::RightVector).Equals(FVector(-1.0, 0.0, 0.0), 1e-9));
		}
		TestEqual(TEXT("36 bodies"), CountOf(Description, TEXT("Body")), 36);
		TestEqual(TEXT("36 pulls"), CountOf(Description, TEXT("Use")), 36);
		TestEqual(TEXT("eight hung faces and the easel"), CountOf(Description, TEXT("Wall")), 9);
		TestEqual(TEXT("the two south chests' felts"), CountOf(Description, TEXT("Mount")), 2);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReserveDescribedTest, "Musee.Reserve.Described", Flags)
	bool FReserveDescribedTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		TestEqual(TEXT("it is the reserve stage"), Description.Stage, FMuseeKey(TEXT("reserve")));
		const TArray<FMuseeFinding> Findings = Musee::Site::CheckDescription(Description);
		for (const FMuseeFinding& Finding : Findings)
		{
			AddError(Finding.Summary);
		}
		TestEqual(TEXT("the stage contract holds"), Findings.Num(), 0);
		TestEqual(TEXT("one enclosure"), Description.Enclosures.Num(), 1);
		TestTrue(TEXT("its one opening is the port"), Description.Openings.Num() == 1 && Description.Openings[0].Kind == EFormOpeningKind::Port);

		// Every surface a piece names is in the stage's finish schedule.
		FString Finishes;
		if (TestTrue(TEXT("the finish schedule reads"), FFileHelper::LoadFileToString(Finishes, *(Musee::Records::DataRoot() / Schedule(TEXT("Finishes.json"))))))
		{
			TSet<FName> Checked;
			for (const FFormDrawing& Piece : Description.Pieces)
			{
				TArray<FName> Surfaces;
				Piece.PartSurfaces.GenerateValueArray(Surfaces);
				Surfaces.Add(Piece.Surface);
				for (const FName Surface : Surfaces)
				{
					if (!Checked.Contains(Surface))
					{
						Checked.Add(Surface);
						const FString Entry = FString::Printf(TEXT("\"surface\": \"%s\""), *Surface.ToString());
						TestTrue(*FString::Printf(TEXT("%s's surface %s is scheduled"), *Piece.Name.ToString(), *Surface.ToString()), Finishes.Contains(Entry));
					}
				}
			}
		}

		// Every piece on a moving body rides one of the racks.
		for (const FFormDrawing& Piece : Description.Pieces)
		{
			if (!Piece.Body.IsNone() && !FindAnchor(Description, Piece.Body, TEXT("Body")))
			{
				AddError(FString::Printf(TEXT("%s rides %s, which is no Body anchor"), *Piece.Name.ToString(), *Piece.Body.ToString()));
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

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReservePlanTest, "Musee.Reserve.Plan", Flags)
	bool FReservePlanTest::RunTest(const FString& Parameters)
	{
		// §3: the bays' clear centres and the vaults' crowns.
		const double Centres[] = {-26.425, -18.75, -11.25, -3.75, 3.75, 11.25, 18.75, 26.425};
		for (int32 Bay = 0; Bay < Plan::Bays; ++Bay)
		{
			TestEqual(*FString::Printf(TEXT("bay %d's clear centre"), Bay + 1), BayCentre(Bay), Centres[Bay], 1e-9);
			TestEqual(*FString::Printf(TEXT("bay %d's nave crown at 5.61"), Bay + 1), Soffit(Centres[Bay], 0.0), 5.61, 1e-9);
			TestEqual(*FString::Printf(TEXT("bay %d's aisle crowns at 5.31"), Bay + 1), Soffit(Centres[Bay], Plan::AisleMiddle), 5.31, 1e-9);
		}
		TestEqual(TEXT("the aisles' middles at |y| 8.675"), Plan::AisleMiddle, 8.675, 1e-9);
		TestEqual(TEXT("the easel spot's canopy on the nave web at 5.475"), Soffit(Plan::SpotX, Plan::SpotY), 5.475, 0.0005);

		// §3.7: the racks' places, home and out.
		const TArray<FRack> All = Racks();
		TestEqual(TEXT("36 racks"), All.Num(), 36);
		const double Xs[] = {-28.15, -26.25, -24.35, -20.65, -18.75, -16.85, -13.15, -11.25, -9.35, -5.65, -3.75, -1.85, 1.85, 3.75, 5.65, 9.35, 11.25, 13.15};
		for (int32 Index = 0; Index < All.Num(); ++Index)
		{
			TestEqual(*FString::Printf(TEXT("rack %s's x"), *All[Index].Name), All[Index].X, Xs[Index % 18], 1e-9);
		}
		TestEqual(TEXT("the first is A"), All[0].Name, FString(TEXT("a")));
		TestEqual(TEXT("the seventh N07"), All[6].Name, FString(TEXT("n07")));
		TestEqual(TEXT("the south's first S01"), All[18].Name, FString(TEXT("s01")));
		const double Reach = Plan::PostY + 0.5 * Plan::PostAlong;
		const double OutNaveEnd = Plan::RackHomeY - Plan::RackTravel - Reach;
		TestEqual(TEXT("a rack at home stands 0.35 off the side wall"), Plan::InnerY - Plan::RackHomeY - Reach, 0.35, 1e-9);
		TestEqual(TEXT("drawn out, its nave end stops 1.00 off the axis"), OutNaveEnd, 1.0, 1e-9);

		// §6: the clearances, each at least 5 mm.
		const double Clear = 0.005;
		const double FinSide = 0.5 * (Plan::SlotClear - Plan::FinThickness);
		const double StemSide = Plan::TrackSlotHalf - 0.5 * Plan::StemDiameter;
		TestTrue(TEXT("the fin keeps 5 mm to its track's sides"), FinSide >= Clear);
		TestTrue(TEXT("and 15 mm over its floor"), Plan::FinBottom - (-Plan::SlotDepth) >= 0.015 - 1e-9);
		TestTrue(TEXT("the stem keeps 5 mm to the track's slot"), StemSide >= Clear - 1e-9);
		TestTrue(TEXT("the stem's top stays inside the channel's hollow"), Plan::StemTop > Plan::TrackUnderside + Plan::TrackLip
			&& Plan::StemTop < Plan::TrackTop - Plan::TrackPlate);
		const double FinReach = Plan::ShoeY + 0.5 * Plan::FinAlong;
		TestTrue(TEXT("home, the fins stay inside the floor track"), Plan::RackHomeY + FinReach <= Plan::SlotTo - 0.5 * (Plan::SlotCut - Plan::SlotClear));
		TestTrue(TEXT("out, too"), Plan::RackHomeY - Plan::RackTravel - FinReach >= Plan::SlotFrom + 0.5 * (Plan::SlotCut - Plan::SlotClear));
		const double PullOut = OutNaveEnd - Plan::PullOff - 0.5 * Plan::PullDiameter;
		TestTrue(TEXT("out, the pull keeps 0.70 to the nave's globes"), PullOut - 0.5 * Plan::NaveGlobeDiameter >= 0.70);
		double Lowest = TNumericLimits<double>::Max();
		for (const FRack& Rack : All)
		{
			for (double Y = Plan::TrackEnd; Y <= Plan::InnerY; Y += 0.05)
			{
				Lowest = FMath::Min(Lowest, Soffit(Rack.X, Rack.Side * Y));
			}
		}
		TestEqual(TEXT("the lowest arch over a track stands at 5.018"), Lowest, 5.018, 0.001);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReserveHeightsTest, "Musee.Reserve.Heights", Flags)
	bool FReserveHeightsTest::RunTest(const FString& Parameters)
	{
		// An extrusion rises from its frame's z 0: every box stands at the level the sheet gives it, in the stage (or, on
		// a rack, in the rack's frame, whose origin is on the floor), never at the floor.
		struct FLevel
		{
			const TCHAR* Piece;
			double Bottom;
			double Top;
		};
		const FLevel Levels[] = {
			{TEXT("floor.bed"), -0.350, -0.060},
			{TEXT("door.threshold"), -0.030, 0.000},
			{TEXT("screen-a.floor-track"), -0.060, 0.000},
			{TEXT("vault.springing.1n"), 4.000, 5.750},
			{TEXT("track.a.hanger.1.clamp"), 3.700, 3.742},
			{TEXT("track.a.stop"), 3.574, 3.704},
			{TEXT("track.a.plate"), 3.510, 3.760},
			{TEXT("screen-a.trolley.saddle.north"), 3.400, 3.414},
			{TEXT("screen-a.shoe.north"), 0.050, 0.070},
			{TEXT("screen-a.fin.north"), -0.030, 0.050},
			{TEXT("screen-a.lamp.west.trough"), 3.298, 3.342},
			{TEXT("easel.upright.north"), 0.090, 2.350},
			{TEXT("easel.cap.north"), 2.350, 2.380},
			{TEXT("easel.ledge"), 0.750, 0.800},
			{TEXT("chest.bay7-south.plinth"), 0.000, 0.090},
			{TEXT("chest.bay7-south.top"), 0.865, 0.900},
			{TEXT("chest.bay7-south.felt"), 0.900, 0.905},
		};
		const FSiteDescription Description = Described();
		for (const FLevel& Level : Levels)
		{
			const FFormDrawing* Piece = Description.Pieces.FindByPredicate([&Level](const FFormDrawing& D) { return D.Name == FName(Level.Piece); });
			if (!TestNotNull(*FString::Printf(TEXT("%s is drawn"), Level.Piece), Piece) || !TestTrue(*FString::Printf(TEXT("%s is a box"), Level.Piece),
				Piece->Shape.IsType<FFormExtrusion>()) || !TestTrue(*FString::Printf(TEXT("%s stands upright"), Level.Piece),
				Piece->Frame.DirectionToParent(FVector::UpVector).Equals(FVector::UpVector, 1e-9)))
			{
				continue;
			}
			const double Bottom = Piece->Frame.OriginMeters.Z;
			TestEqual(*FString::Printf(TEXT("%s's underside"), Level.Piece), Bottom, Level.Bottom, 1e-9);
			TestEqual(*FString::Printf(TEXT("%s's top"), Level.Piece), Bottom + Piece->Shape.Get<FFormExtrusion>().DepthMeters, Level.Top, 1e-9);
		}

		// The door's sill (§2): the brick reveal lies the threshold's depth under the floor, and the stone runs from half the
		// joint's strip in from the port face to the hall face, so the strip lies on the mating plane between two floors.
		const TValueOrError<FSitePortClass, FString> Class = Musee::Site::FindPortClass(FMuseeKey(Plan::PortClass));
		const FFormDrawing* Threshold = Description.Pieces.FindByPredicate([](const FFormDrawing& D) { return D.Name == FName(TEXT("door.threshold")); });
		const FFormDrawing* Walls = Description.Pieces.FindByPredicate([](const FFormDrawing& D) { return D.Name == FName(Pieces::Walls); });
		if (TestTrue(TEXT("the port's class reads"), Class.HasValue()) && TestNotNull(TEXT("the threshold"), Threshold) && TestNotNull(TEXT("the walls"), Walls))
		{
			const FFormProfile& Outline = Threshold->Shape.Get<FFormExtrusion>().Outline;
			FBox2D Plan2D(FVector2D(Outline.Start[0], Outline.Start[1]), FVector2D(Outline.Start[0], Outline.Start[1]));
			for (const FFormProfileStep& Step : Outline.Steps)
			{
				Plan2D += FVector2D(Step.Line[0], Step.Line[1]);
			}
			TestEqual(TEXT("the threshold starts half the strip in from the port face"), Plan2D.Min.X, -Plan::OuterX + 0.5 * Class.GetValue().StripMeters, 1e-9);
			TestEqual(TEXT("and ends on the hall face"), Plan2D.Max.X, -Plan::InnerX, 1e-9);
			TestEqual(TEXT("the door's clear width"), Plan2D.GetSize().Y, Class.GetValue().ClearWidthMeters, 1e-9);
			const FFormWall& Wall = Walls->Shape.Get<FFormWall>();
			TestTrue(TEXT("the brick sill lies under the threshold"), Wall.Openings.Num() == 1
				&& FMath::IsNearlyEqual(Wall.BaseMeters + Wall.Openings[0].SillMeters, Threshold->Frame.OriginMeters.Z, 1e-9));
		}
		return true;
	}
}

#endif
