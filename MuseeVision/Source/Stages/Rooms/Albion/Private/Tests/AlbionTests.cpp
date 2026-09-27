#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AlbionParts.h"
#include "AlbionPlan.h"
#include "AlbionRoom.h"
#include "ArtHang.h"
#include "Misc/FileHelper.h"
#include "MuseeRecords.h"
#include "SiteDescription.h"
#include "SitePortClass.h"

/**
 * The court's description against its sheet and its schedules: its port and the museum's translation, its anchors
 * against every anchor the schedules name, the stage contract's own check and the openings Light's daylight names, and
 * the plan's measured numbers. Its shells are proved sealed by Studio's Musee.Studio.RoomsSealed, which makes its drawings.
 */
namespace Musee::Albion::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		FSiteDescription Described()
		{
			FSiteDescription Description;
			GetDefault<UAlbionRoom>()->Describe(Description);
			return Description;
		}

		const FMuseeStageAnchor* FindAnchor(const FSiteDescription& Description, const FMuseeKey& AnchorKey, FName Kind)
		{
			return Description.Anchors.FindByPredicate([&](const FMuseeStageAnchor& A) { return A.Key == AnchorKey && A.Kind == Kind; });
		}

		FString Schedule(const TCHAR* File)
		{
			return FString::Printf(TEXT("Stages/%s/%s"), Plan::Stage, File);
		}

		bool ReadSchedule(const TCHAR* File, FString& Out)
		{
			return FFileHelper::LoadFileToString(Out, *(Musee::Records::DataRoot() / Schedule(File)));
		}

		/** Every value a schedule's text gives a field: "field": "<value>". */
		TArray<FString> Values(const FString& Text, const TCHAR* Field)
		{
			TArray<FString> Found;
			const FString Needle = FString::Printf(TEXT("\"%s\": \""), Field);
			int32 From = 0;
			while (true)
			{
				const int32 At = Text.Find(Needle, ESearchCase::CaseSensitive, ESearchDir::FromStart, From);
				if (At == INDEX_NONE)
				{
					break;
				}
				const int32 Start = At + Needle.Len();
				const int32 End = Text.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, Start);
				Found.Add(Text.Mid(Start, End - Start));
				From = End;
			}
			return Found;
		}

		/** The anchor kind a hung entry needs: works on the walls, objects on the cases and the lectern, glass in the band. */
		FName KindFor(const FMuseeKey& Anchor)
		{
			const FString Text = Anchor.ToString();
			if (Text.StartsWith(TEXT("albion.wall.")))
			{
				return TEXT("Wall");
			}
			return Text.StartsWith(TEXT("albion.band.")) || Text.StartsWith(TEXT("albion.lancet.")) ? FName(TEXT("Glazing")) : FName(TEXT("Mount"));
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAlbionPortsTest, "Musee.Albion.Ports", Flags)
	bool FAlbionPortsTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		if (!TestEqual(TEXT("one port: the court is a dead end"), Description.Ports.Num(), 1))
		{
			return false;
		}
		const FSitePort& North = Description.Ports[0];
		TestEqual(TEXT("it is albion.north"), North.Key, FMuseeKey(TEXT("albion.north")));
		TestTrue(TEXT("on the port plane on the axis, at the threshold"), North.Frame.OriginMeters.Equals(FVector(0.0, -24.0, 0.0), 1e-9));
		TestTrue(TEXT("facing north, to the Rotunda"), North.Facing().Equals(FVector(0.0, -1.0, 0.0), 1e-9));
		TestTrue(TEXT("its z is up"), North.Frame.DirectionToParent(FVector::UpVector).Equals(FVector::UpVector, 1e-9));
		TestEqual(TEXT("it meets the Rotunda's south door"), North.Mates, FMuseeKey(TEXT("rotunda.south")));
		TestEqual(TEXT("through the 0.10 lining"), North.WallDepthMeters, 0.1);
		TestEqual(TEXT("on a flat face"), North.FaceRadiusMeters, 0.0);
		TestFalse(TEXT("with no well"), North.bHasWell);
		// museum = local + (0, 35.2, 0): the port stands on rotunda.south's frame, museum (0, 11.2, 0), tangent to the drum.
		TestTrue(TEXT("placed, it stands on the drum's outer face on the axis"),
			(North.Frame.OriginMeters + FVector(0.0, Plan::MuseumOffsetY, 0.0)).Equals(FVector(0.0, 11.2, 0.0), 1e-9));

		// The rough opening is the class's opening grown by its allowance.
		const FArch Rough = RoughOpening();
		TestEqual(TEXT("the rough opening is 4.10 wide"), 2.0 * Rough.Half, 4.1, 1e-9);
		TestEqual(TEXT("its crown is 6.55"), Rough.Crown(), 6.55, 1e-9);
		const TValueOrError<FSitePortClass, FString> Class = Musee::Site::FindPortClass(North.Class);
		if (Class.HasValue())
		{
			TestEqual(TEXT("the class is 4.00 clear"), Class.GetValue().ClearWidthMeters, 2.0 * Plan::PortClearHalf);
			TestEqual(TEXT("and springs at 4.50"), Class.GetValue().HeadMeters, Plan::PortSpringing);
			TestTrue(TEXT("with a semicircular head"), Class.GetValue().Arched);
		}
		else
		{
			// Site's records don't hold the class yet: a gap in Site, reported, not worked round.
			AddWarning(FString::Printf(TEXT("Site's port classes lack %s: %s"), Plan::PortClass, *Class.GetError()));
		}
		const FFormOpening* Port = Description.Openings.FindByPredicate([](const FFormOpening& O) { return O.Kind == EFormOpeningKind::Port; });
		if (TestNotNull(TEXT("the port's rough opening is declared"), Port))
		{
			TestEqual(TEXT("as albion.north"), Port->Key, FMuseeKey(TEXT("albion.north")));
			TestTrue(TEXT("facing out of the court"), Port->Frame.DirectionToParent(FVector(0.0, 0.0, 1.0)).Equals(FVector(0.0, -1.0, 0.0), 1e-9));
			TestTrue(TEXT("the rough opening's size"), Port->SizeMeters.Equals(FVector2D(4.1, 6.6), 1e-9));
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAlbionAnchorsTest, "Musee.Albion.Anchors", Flags)
	bool FAlbionAnchorsTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		const TValueOrError<FArtHang, FString> Hang = Musee::ReadRecord<FArtHang>(Schedule(TEXT("Hang.json")));
		if (!TestTrue(TEXT("the hanging schedule reads"), Hang.HasValue()))
		{
			AddError(Hang.GetError());
			return false;
		}
		for (const FArtHangEntry& Work : Hang.GetValue().Works)
		{
			const FName Kind = KindFor(Work.Anchor);
			TestNotNull(*FString::Printf(TEXT("%s has its %s anchor %s"), *Work.Work, *Kind.ToString(), *Work.Anchor.ToString()),
				FindAnchor(Description, Work.Anchor, Kind));
		}
		FString Lighting;
		if (TestTrue(TEXT("the lighting schedule reads"), ReadSchedule(TEXT("Lighting.json"), Lighting)))
		{
			for (const FString& Mount : Values(Lighting, TEXT("mount")))
			{
				TestNotNull(*FString::Printf(TEXT("the luminaire mount %s is a Mount anchor"), *Mount), FindAnchor(Description, FMuseeKey(Mount), TEXT("Mount")));
			}
		}

		// The walls, as the sheet frames them: the west's origin at its south end, x north along it, y out into the court.
		const FMuseeStageAnchor* West = FindAnchor(Description, FMuseeKey(TEXT("albion.wall.west")), TEXT("Wall"));
		if (TestNotNull(TEXT("the west wall"), West))
		{
			TestTrue(TEXT("its origin"), West->Frame.OriginMeters.Equals(FVector(-14.0, 18.0, 0.0), 1e-9));
			TestTrue(TEXT("its x"), West->Frame.DirectionToParent(FVector(1.0, 0.0, 0.0)).Equals(FVector(0.0, -1.0, 0.0), 1e-9));
			TestTrue(TEXT("its y"), West->Frame.DirectionToParent(FVector(0.0, 1.0, 0.0)).Equals(FVector(1.0, 0.0, 0.0), 1e-9));
		}
		const FMuseeStageAnchor* South = FindAnchor(Description, FMuseeKey(TEXT("albion.wall.south")), TEXT("Wall"));
		if (TestNotNull(TEXT("the south wall"), South))
		{
			TestTrue(TEXT("its origin"), South->Frame.OriginMeters.Equals(FVector(14.0, 18.0, 0.0), 1e-9));
			TestTrue(TEXT("its y faces north into the court"), South->Frame.DirectionToParent(FVector(0.0, 1.0, 0.0)).Equals(FVector(0.0, -1.0, 0.0), 1e-9));
		}

		// Twenty-four Glazing anchors: the band's twelve and the twelve lancets', their y into the court.
		int32 Glazing = 0;
		for (const FMuseeStageAnchor& Anchor : Description.Anchors)
		{
			if (Anchor.Kind == FName(TEXT("Glazing")))
			{
				++Glazing;
				const FVector Into = Anchor.Frame.DirectionToParent(FVector(0.0, 1.0, 0.0));
				TestTrue(*FString::Printf(TEXT("%s's y looks into the court"), *Anchor.Key.ToString()),
					FVector::DotProduct(Into, FVector(-Anchor.Frame.OriginMeters.X, -Anchor.Frame.OriginMeters.Y, 0.0)) > 0.0);
			}
		}
		TestEqual(TEXT("twenty-four glazed lights"), Glazing, 24);
		const FMuseeStageAnchor* T1 = FindAnchor(Description, FMuseeKey(TEXT("albion.band.t1")), TEXT("Glazing"));
		const FMuseeStageAnchor* T12 = FindAnchor(Description, FMuseeKey(TEXT("albion.band.t12")), TEXT("Glazing"));
		if (TestNotNull(TEXT("T1"), T1) && TestNotNull(TEXT("T12"), T12))
		{
			TestTrue(TEXT("T1 at the upper row's east end"), T1->Frame.OriginMeters.Equals(FVector(4.8, 18.3, 10.9), 1e-9));
			TestTrue(TEXT("T12 at the lower row's west end"), T12->Frame.OriginMeters.Equals(FVector(-3.2, 18.3, 9.0), 1e-9));
		}
		const FMuseeStageAnchor* E1 = FindAnchor(Description, FMuseeKey(TEXT("albion.lancet.e1")), TEXT("Glazing"));
		if (TestNotNull(TEXT("lancet e1"), E1))
		{
			TestTrue(TEXT("at its lower left seen from inside"), E1->Frame.OriginMeters.Equals(FVector(14.3, -15.7, 5.0), 1e-9));
		}

		// The lectern's desk: its centre, x up the slope, z its normal.
		const FMuseeStageAnchor* Lectern = FindAnchor(Description, FMuseeKey(TEXT("albion.lectern")), TEXT("Mount"));
		if (TestNotNull(TEXT("the lectern"), Lectern))
		{
			TestTrue(TEXT("at the desk's centre"), Lectern->Frame.OriginMeters.Equals(FVector(12.2, -13.3, 1.107), 0.001));
			const double Slope = FMath::DegreesToRadians(18.0);
			TestTrue(TEXT("its z is the desk's normal"), Lectern->Frame.DirectionToParent(FVector::UpVector).Equals(FVector(-FMath::Sin(Slope), 0.0, FMath::Cos(Slope)), 1e-9));
		}

		// Eight seats: on each bench the outer pair faces the aisle, the inner pair the nave.
		int32 Seats = 0;
		for (const FMuseeStageAnchor& Anchor : Description.Anchors)
		{
			if (Anchor.Kind == FName(TEXT("Seat")))
			{
				++Seats;
				const FVector Facing = Anchor.Frame.DirectionToParent(FVector::ForwardVector);
				const bool bOuter = FMath::IsNearlyEqual(FMath::Abs(Anchor.Frame.OriginMeters.X), Plan::SeatOuter, 1e-9);
				const double Out = FMath::Sign(Anchor.Frame.OriginMeters.X);
				TestTrue(*FString::Printf(TEXT("%s faces %s"), *Anchor.Key.ToString(), bOuter ? TEXT("its aisle") : TEXT("the nave")),
					Facing.Equals(FVector(bOuter ? Out : -Out, 0.0, 0.0), 1e-9));
				TestEqual(TEXT("at the seat's top"), Anchor.Frame.OriginMeters.Z, 0.45, 1e-9);
			}
		}
		TestEqual(TEXT("eight seats"), Seats, 8);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAlbionDescribedTest, "Musee.Albion.Described", Flags)
	bool FAlbionDescribedTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		TestEqual(TEXT("it is the albion stage"), Description.Stage, FMuseeKey(TEXT("albion")));
		const TArray<FMuseeFinding> Findings = Musee::Site::CheckDescription(Description);
		for (const FMuseeFinding& Finding : Findings)
		{
			AddError(Finding.Summary);
		}
		TestEqual(TEXT("the stage contract holds"), Findings.Num(), 0);
		TestEqual(TEXT("one enclosure, the court with its porch"), Description.Enclosures.Num(), 1);
		TestTrue(TEXT("bounded by its pieces"), Description.Enclosures.Num() == 1 && Description.Enclosures[0].Pieces.Num() > 100);
		TestEqual(TEXT("the footprint's envelope"), Description.Footprint.OutlineMeters.Num(), 8);

		// Every opening Light's daylight names is a Glazed opening of the court, with its glass.
		FString Lighting;
		if (TestTrue(TEXT("the lighting schedule reads"), ReadSchedule(TEXT("Lighting.json"), Lighting)))
		{
			const TArray<FString> Daylight = Values(Lighting, TEXT("opening"));
			TestEqual(TEXT("twenty-two daylit openings"), Daylight.Num(), 22);
			for (const FString& Name : Daylight)
			{
				TestTrue(*FString::Printf(TEXT("%s is a glazed opening"), *Name), Description.Openings.ContainsByPredicate([&Name](const FFormOpening& O)
				{
					return O.Key == FMuseeKey(Name) && O.Kind == EFormOpeningKind::Glazed && !O.Glazing.IsNone();
				}));
			}
		}

		// Every surface a piece names is in the stage's finish schedule.
		FString Finishes;
		if (TestTrue(TEXT("the finish schedule reads"), ReadSchedule(TEXT("Finishes.json"), Finishes)))
		{
			TSet<FName> Checked;
			for (const FFormDrawing& Piece : Description.Pieces)
			{
				TArray<FName> Surfaces;
				Piece.PartSurfaces.GenerateValueArray(Surfaces);
				Surfaces.Add(Piece.Surface);
				for (const FName Finish : Surfaces)
				{
					bool bSeen = false;
					Checked.Add(Finish, &bSeen);
					if (!bSeen)
					{
						const FString Entry = FString::Printf(TEXT("\"surface\": \"%s\""), *Finish.ToString());
						TestTrue(*FString::Printf(TEXT("%s's surface %s is scheduled"), *Piece.Name.ToString(), *Finish.ToString()), Finishes.Contains(Entry));
					}
				}
			}
		}

		// The description is a plain function of the plan: the same on every call.
		const FSiteDescription Again = Described();
		bool bSame = Again.Pieces.Num() == Description.Pieces.Num();
		for (int32 Index = 0; bSame && Index < Again.Pieces.Num(); ++Index)
		{
			bSame = Again.Pieces[Index].Name == Description.Pieces[Index].Name && Again.Pieces[Index].Frame.OriginMeters == Description.Pieces[Index].Frame.OriginMeters;
		}
		TestTrue(TEXT("the same pieces in the same order and places"), bSame);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAlbionPlanTest, "Musee.Albion.Plan", Flags)
	bool FAlbionPlanTest::RunTest(const FString& Parameters)
	{
		// The vaults' arcs (§3.5): radius, crown and the half-arc's length.
		const TConstArrayView<FVaultHalf> Halves = VaultHalves();
		TestEqual(TEXT("the nave's radius"), Halves[0].Radius(), 7.0833, 1e-4);
		TestEqual(TEXT("its centres 1.0833 across the axis"), Halves[0].CentreX(), 1.0833, 1e-4);
		TestEqual(TEXT("its half-arc"), Halves[0].Length(), 10.04, 0.005);
		TestEqual(TEXT("the aisles' radius"), Halves[2].Radius(), 5.38, 1e-9);
		TestEqual(TEXT("their half-arc"), Halves[2].Length(), 7.06, 0.005);
		TestEqual(TEXT("seventeen panes a nave half"), Halves[0].PaneStations().Num() - 1, 17);
		TestEqual(TEXT("twelve an aisle half"), Halves[5].PaneStations().Num() - 1, 12);
		TestEqual(TEXT("the glazing bars at 0.747"), 2.0 * Plan::GlassEndY / Plan::GlazingBays, 0.747, 0.0005);
		TestTrue(TEXT("the east halves mirror the west"), FMath::IsNearlyEqual(Halves[1].At(3.0).X, -Halves[0].At(3.0).X, 1e-9)
			&& FMath::IsNearlyEqual(Halves[4].At(3.0).X, -Halves[3].At(3.0).X, 1e-9));

		// The glass's height, as the lighting schedule's end-wall projectors take it: glass(x) - 0.80.
		TestEqual(TEXT("over Shalott"), GlassZ(-10.0) - 0.8, 14.40, 1e-6);
		TestEqual(TEXT("over the Attainment"), GlassZ(0.0) - 0.8, 16.20, 1e-6);
		TestEqual(TEXT("over the Star"), GlassZ(10.5) - 0.8, 14.24, 0.005);

		// The arches: the lancets' springing, the court door's radius, the arcade's radius and soffit.
		TestEqual(TEXT("the lancets spring at 8.199"), Lancet(1).Springing, 8.199, 0.0005);
		TestEqual(TEXT("from centres 0.108 across the axis"), Lancet(1).Offset, 0.108, 1e-9);
		TestEqual(TEXT("the court door's radius"), CourtDoor().Radius, 2.21, 1e-9);
		const FArch Arcade = FArch::Pointed(0.0, Plan::ArcadeHalfSpan, Plan::ArcadeSpringing, Plan::ArcadeSpringing, Plan::ArcadeCrown);
		TestEqual(TEXT("the arcade's radius"), Arcade.Radius, 3.602, 0.0005);
		TestEqual(TEXT("its centres 0.852 beyond the axis"), Arcade.Offset, 0.852, 0.0005);
		TestEqual(TEXT("its soffit at the crown"), Plan::ArcadeCrown - Plan::ArcadeRibDepth, 9.54, 1e-9);

		// Albion.AxisClear, by the plan: from the gilt sun's eye to the Attainment the 0.60 sightline passes the court door
		// and the port under their heads and between their jambs, and every bench stands clear of it.
		const double Radius = 0.5 * Plan::SightlineDiameter;
		const FVector Eye(Plan::SunEye[0], Plan::SunEye[1], Plan::SunEye[2]);
		const FVector Target(Plan::AttainmentCentre[0], Plan::AttainmentCentre[1], Plan::AttainmentCentre[2]);
		auto LineZ = [&](double Y) { return FMath::Lerp(Eye.Z, Target.Z, (Y - Eye.Y) / (Target.Y - Eye.Y)); };
		TestTrue(TEXT("through the port's opening"), Radius < RoughOpening().Half && LineZ(Plan::PortPlaneY) + Radius < RoughOpening().Crown());
		TestTrue(TEXT("through the court door"), Radius < CourtDoor().Half && LineZ(-Plan::EndInnerY) + Radius < CourtDoor().Crown());
		TestTrue(TEXT("under the porch's beams"), LineZ(Plan::BeamY[0]) + Radius < Plan::BeamSoffit);
		TestTrue(TEXT("clear of the benches"), Plan::BenchX - Plan::BenchHalfDepth >= Radius);

		// The lining's porch face stays clear for the joint's surround: the skirting stops at it, the wall plates and the
		// ceiling stand beyond it, the first beam 1.30 into the porch.
		TestTrue(TEXT("the wall plates stand outside the surround"), Plan::WallPlateX - Plan::WallPlateHalf > Plan::SurroundClearHalf);
		TestTrue(TEXT("the ceiling over it"), Plan::PorchCeiling > Plan::SurroundClearTop);
		TestEqual(TEXT("the first beam 1.30 from the lining"), Plan::BeamY[0] - Plan::LiningFaceY, 1.30, 1e-9);

		// The footprint's envelope holds the buttresses on their base courses and the rainwater heads' lips.
		const double Buttress = Plan::SideOuterX + Plan::ButtressDepth + Plan::BaseCourseProud;
		TestEqual(TEXT("the side buttresses reach x 15.56"), Buttress, 15.56, 1e-9);
		TestTrue(TEXT("inside the envelope"), Buttress <= Plan::FootprintPoints[3][0] && Plan::EndOuterY + Plan::ButtressDepth + Plan::BaseCourseProud <= Plan::FootprintPoints[4][1]);
		TestTrue(TEXT("the heads' lips too"), Plan::SideOuterX + Plan::LipOut <= Plan::FootprintPoints[3][0]);
		return true;
	}
}

#endif
