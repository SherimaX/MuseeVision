#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GroundsPlan.h"
#include "GroundsRoom.h"
#include "GroundsSettingOut.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "MuseeRecords.h"
#include "SiteDescription.h"

/**
 * The Grounds' description against its sheet and its schedules: no ports and its placing in the site plan, its anchors
 * against every anchor the schedules name, the stage contract's own check, the sheet's setting-out numbers, and a
 * description that is the same on every call. The schedules are read as text, as the systems that own them keep them,
 * so a field their readers don't take yet doesn't hide an anchor.
 */
namespace Musee::Grounds::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		const FSiteDescription& Described()
		{
			static const FSiteDescription Description = []
			{
				FSiteDescription Made;
				GetDefault<UGroundsRoom>()->Describe(Made);
				return Made;
			}();
			return Description;
		}

		const FMuseeStageAnchor* FindAnchor(const FSiteDescription& Description, const FString& Key, FName Kind)
		{
			return Description.Anchors.FindByPredicate([&](const FMuseeStageAnchor& A) { return A.Key == FMuseeKey(Key) && A.Kind == Kind; });
		}

		TArray<const FMuseeStageAnchor*> AnchorsOf(const FSiteDescription& Description, const TCHAR* Key)
		{
			TArray<const FMuseeStageAnchor*> Found;
			for (const FMuseeStageAnchor& Anchor : Description.Anchors)
			{
				if (Anchor.Key == FMuseeKey(Key))
				{
					Found.Add(&Anchor);
				}
			}
			return Found;
		}

		/** The Seat anchors whose keys run on from a prefix (<coping>.seat.<n>), each key its own. */
		TArray<const FMuseeStageAnchor*> SeatsOf(const FSiteDescription& Description, const FString& Prefix)
		{
			TArray<const FMuseeStageAnchor*> Found;
			for (const FMuseeStageAnchor& Anchor : Description.Anchors)
			{
				if (Anchor.Kind == FName(TEXT("Seat")) && Anchor.Key.ToString().StartsWith(Prefix))
				{
					Found.Add(&Anchor);
				}
			}
			return Found;
		}

		int32 CountPieces(const FSiteDescription& Description, const FString& Prefix, const FString& Suffix = FString())
		{
			return Description.Pieces.FilterByPredicate([&](const FFormDrawing& D)
			{
				const FString Name = D.Name.ToString();
				return Name.StartsWith(Prefix) && (Suffix.IsEmpty() || Name.EndsWith(Suffix));
			}).Num();
		}

		bool ReadText(const FString& Record, FString& OutText)
		{
			return FFileHelper::LoadFileToString(OutText, *(Musee::Records::DataRoot() / Record));
		}

		bool ReadSchedule(const TCHAR* File, FString& OutText)
		{
			return ReadText(FString::Printf(TEXT("Stages/%s/%s"), Plan::Stage, File), OutText);
		}

		/** Every capture of a pattern's groups in a text, one array of groups per match. */
		TArray<TArray<FString>> Matches(const FString& Text, const FString& Pattern, int32 Groups)
		{
			const FRegexPattern Compiled(Pattern);
			FRegexMatcher Matcher(Compiled, Text);
			TArray<TArray<FString>> Found;
			while (Matcher.FindNext())
			{
				TArray<FString>& Match = Found.AddDefaulted_GetRef();
				for (int32 Group = 1; Group <= Groups; ++Group)
				{
					Match.Add(Matcher.GetCaptureGroup(Group));
				}
			}
			return Found;
		}

		/** Every value a schedule gives a string field of this name. */
		TArray<FString> ValuesOf(const FString& Text, const TCHAR* Field)
		{
			TArray<FString> Values;
			for (const TArray<FString>& Match : Matches(Text, FString::Printf(TEXT("\"%s\"\\s*:\\s*\"([^\"]+)\""), Field), 1))
			{
				Values.Add(Match[0]);
			}
			return Values;
		}

		/** Where a profile's drawing ends. */
		FVector2D EndOf(const FFormProfile& Profile)
		{
			if (Profile.Steps.IsEmpty())
			{
				return Profile.StartPoint();
			}
			const FFormProfileStep& Step = Profile.Steps.Last();
			if (!Step.IsArc())
			{
				return FVector2D(Step.Line[0], Step.Line[1]);
			}
			const double T = FMath::DegreesToRadians(Step.Arc.ToDegrees);
			return FVector2D(Step.Arc.CenterMeters[0] + Step.Arc.RadiiMeters[0] * FMath::Cos(T),
				Step.Arc.CenterMeters[1] + Step.Arc.RadiiMeters.Last() * FMath::Sin(T));
		}

		/** The plan box of a laid piece's straight outline. */
		FBox2D BoxOf(const FFormDrawing& Piece)
		{
			FBox2D Box(ForceInit);
			if (const FFormExtrusion* Shape = Piece.Shape.TryGet<FFormExtrusion>())
			{
				Box += Shape->Outline.StartPoint();
				for (const FFormProfileStep& Step : Shape->Outline.Steps)
				{
					if (!Step.IsArc())
					{
						Box += FVector2D(Step.Line[0], Step.Line[1]);
					}
				}
			}
			return Box;
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundsPortsTest, "Musee.Grounds.Ports", Flags)
	bool FGroundsPortsTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription& Description = Described();
		TestEqual(TEXT("the grounds have no port"), Description.Ports.Num(), 0);
		TestEqual(TEXT("and no opening"), Description.Openings.Num(), 0);
		TestTrue(TEXT("its footprint is the Ground"), Description.Footprint.Kind == ESiteFootprintKind::Ground);
		const TArray<FVector2D>& Outline = Description.Footprint.OutlineMeters;
		TestEqual(TEXT("the 800 m square"), Outline.Num(), 4);
		for (const FVector2D& Corner : Outline)
		{
			TestTrue(TEXT("each corner 400 m out on both axes"), FMath::IsNearlyEqual(FMath::Abs(Corner.X), 400.0) && FMath::IsNearlyEqual(FMath::Abs(Corner.Y), 400.0));
		}
		TestEqual(TEXT("from the soil's bottom on the Reserve's top"), Description.Footprint.BottomMeters, -0.33);
		TestEqual(TEXT("to the tallest birch's sway"), Description.Footprint.TopMeters, 13.0);

		// The site plan places it at the anchor's datum (the contract's "placed"), and no mating names a port of it.
		FString SitePlan;
		if (TestTrue(TEXT("the site plan reads"), ReadText(TEXT("Site/SitePlan.json"), SitePlan)))
		{
			const TArray<TArray<FString>> Placed = Matches(SitePlan, TEXT("\"stage\"\\s*:\\s*\"grounds\"\\s*,\\s*\"translationMeters\"\\s*:\\s*\\[\\s*([-0-9.]+)\\s*,\\s*([-0-9.]+)\\s*,\\s*([-0-9.]+)\\s*\\]\\s*,\\s*\"yawDegrees\"\\s*:\\s*([-0-9.]+)"), 4);
			if (TestEqual(TEXT("the site plan places the grounds once"), Placed.Num(), 1))
			{
				for (const FString& Value : Placed[0])
				{
					TestEqual(TEXT("at the anchor's datum, unturned"), FCString::Atod(*Value), 0.0);
				}
			}
			TestFalse(TEXT("no mating names a port of the grounds"), SitePlan.Contains(TEXT("\"grounds.")));
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundsAnchorsTest, "Musee.Grounds.Anchors", Flags)
	bool FGroundsAnchorsTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription& Description = Described();
		FString Planting;
		FString Lighting;
		if (!TestTrue(TEXT("the planting and lighting schedules read"), ReadSchedule(TEXT("Planting.json"), Planting) && ReadSchedule(TEXT("Lighting.json"), Lighting)))
		{
			return false;
		}
		const TArray<FString> Beds = ValuesOf(Planting, TEXT("bed"));
		TestEqual(TEXT("50 beds: the lawn, 37 trees, two box loops, eight balls and two yew arcs"), Beds.Num(), 50);
		for (const FString& Bed : Beds)
		{
			TestNotNull(*FString::Printf(TEXT("%s is a Bed anchor"), *Bed), FindAnchor(Description, Bed, TEXT("Bed")));
		}
		for (const FString& Water : ValuesOf(Planting, TEXT("anchor")))
		{
			const FMuseeStageAnchor* Anchor = FindAnchor(Description, Water, TEXT("Water"));
			if (TestNotNull(*FString::Printf(TEXT("%s is a Water anchor"), *Water), Anchor))
			{
				TestEqual(TEXT("at the still water's level"), Anchor->Frame.OriginMeters.Z, 0.30);
			}
		}

		// A single plant's bed stands at the schedule's point: on the lawn, or on the gravel in the allées.
		for (const TArray<FString>& Point : Matches(Planting, TEXT("\"bed\"\\s*:\\s*\"([^\"]+)\"[^{}]*?\"point\"\\s*:\\s*\\[\\s*([-0-9.]+)\\s*,\\s*([-0-9.]+)\\s*\\]"), 3))
		{
			const FMuseeStageAnchor* Anchor = FindAnchor(Description, Point[0], TEXT("Bed"));
			if (TestNotNull(*Point[0], Anchor))
			{
				const FVector Origin = Anchor->Frame.OriginMeters;
				TestTrue(*FString::Printf(TEXT("%s stands at its point"), *Point[0]),
					FVector2D(Origin).Equals(FVector2D(FCString::Atod(*Point[1]), FCString::Atod(*Point[2])), 1e-9));
				const bool bAllee = Point[0].Contains(TEXT(".allee."));
				TestEqual(TEXT("on the gravel in the allées, else on the lawn"), Origin.Z, bAllee ? 0.018 : -0.03);
			}
		}

		// Each lamp's Mount is its globe's centre, 3.62 over its standard's foot, facing down.
		const TArray<FString> Mounts = ValuesOf(Lighting, TEXT("mount"));
		TestEqual(TEXT("ten lamps"), Mounts.Num(), 10);
		for (int32 Index = 0; Index < Mounts.Num(); ++Index)
		{
			const FMuseeStageAnchor* Anchor = FindAnchor(Description, Mounts[Index], TEXT("Mount"));
			if (TestNotNull(*FString::Printf(TEXT("%s is a Mount anchor"), *Mounts[Index]), Anchor) && Index < int32(UE_ARRAY_COUNT(Plan::Lamps)))
			{
				TestTrue(TEXT("at the globe's centre"), Anchor->Frame.OriginMeters.Equals(FVector(Plan::Lamps[Index].X, Plan::Lamps[Index].Y, 3.62), 1e-9));
				TestTrue(TEXT("facing down"), Anchor->Frame.DirectionToParent(FVector::ForwardVector).Equals(-FVector::UpVector, 1e-9));
			}
		}

		// The copings' seats: every 0.80 on the canal's seat line, clear of corners and lamp bases; round the basin.
		const TArray<const FMuseeStageAnchor*> Canal = SeatsOf(Description, TEXT("grounds.canal.coping.seat."));
		const TArray<const FMuseeStageAnchor*> Basin = SeatsOf(Description, TEXT("grounds.rond-point.coping.seat."));
		TestEqual(TEXT("the canal seats 80: 31 on each long side, 9 on each end"), Canal.Num(), 80);
		TestEqual(TEXT("the round basin seats 21, 0.82 apart"), Basin.Num(), 21);
		TSet<FString> SeatKeys;
		for (const FMuseeStageAnchor* Seat : Canal) { SeatKeys.Add(Seat->Key.ToString()); }
		for (const FMuseeStageAnchor* Seat : Basin) { SeatKeys.Add(Seat->Key.ToString()); }
		TestEqual(TEXT("each seat has its own key"), SeatKeys.Num(), Canal.Num() + Basin.Num());
		const FVector2D CanalCentre(-44.0, 42.0);
		for (const FMuseeStageAnchor* Seat : Canal)
		{
			const FVector2D Place(Seat->Frame.OriginMeters);
			TestEqual(TEXT("a seat is at the coping's top"), Seat->Frame.OriginMeters.Z, 0.45);
			TestTrue(TEXT("on the seat line, 0.25 in from the outer face"),
				FMath::IsNearlyEqual(FMath::Abs(Place.X + 44.0), 4.25) || FMath::IsNearlyEqual(FMath::Abs(Place.Y - 42.0), 14.75));
			TestTrue(TEXT("at least 0.60 from a corner"), FMath::Abs(Place.X + 44.0) <= 4.25 - 0.6 + 1e-9 || FMath::Abs(Place.Y - 42.0) <= 14.75 - 0.6 + 1e-9);
			for (const Plan::FPlanPoint& Lamp : Plan::Lamps)
			{
				TestTrue(TEXT("and 0.60 from a lamp's base"), FVector2D::Distance(Place, FVector2D(Lamp.X, Lamp.Y)) - 0.27 >= 0.6 - 1e-9);
			}
			const FVector Facing = Seat->Frame.DirectionToParent(FVector::ForwardVector);
			TestTrue(TEXT("facing out, away from the water"), FVector2D::DotProduct(FVector2D(Facing), Place - CanalCentre) > 0.0);
		}
		for (const FMuseeStageAnchor* Seat : Basin)
		{
			TestEqual(TEXT("round the basin on r 2.75"), FVector2D::Distance(FVector2D(Seat->Frame.OriginMeters), FVector2D(54.0, 42.0)), 2.75, 1e-9);
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundsDescribedTest, "Musee.Grounds.Described", Flags)
	bool FGroundsDescribedTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription& Description = Described();
		TestEqual(TEXT("it is the grounds stage"), Description.Stage, FMuseeKey(TEXT("grounds")));
		const TArray<FMuseeFinding> Findings = Musee::Site::CheckDescription(Description);
		for (const FMuseeFinding& Finding : Findings)
		{
			AddError(Finding.Summary);
		}
		TestEqual(TEXT("the stage contract holds"), Findings.Num(), 0);
		if (TestEqual(TEXT("one enclosure"), Description.Enclosures.Num(), 1))
		{
			const FFormEnclosure& Open = Description.Enclosures[0];
			TestEqual(TEXT("grounds.open"), Open.Key, FMuseeKey(TEXT("grounds.open")));
			TestTrue(TEXT("under the museum's sky"), Open.Kind == EFormEnclosureKind::Outdoors);
		}

		// Every surface a piece names is in the stage's finish schedule, and every piece collides.
		FString Finishes;
		if (TestTrue(TEXT("the finish schedule reads"), ReadSchedule(TEXT("Finishes.json"), Finishes)))
		{
			const TArray<FString> Scheduled = ValuesOf(Finishes, TEXT("surface"));
			for (const FFormDrawing& Piece : Description.Pieces)
			{
				TestTrue(*FString::Printf(TEXT("%s's surface %s is scheduled"), *Piece.Name.ToString(), *Piece.Surface.ToString()),
					Scheduled.Contains(Piece.Surface.ToString()));
				TestFalse(*FString::Printf(TEXT("%s collides"), *Piece.Name.ToString()), Piece.Collision.IsNone());
			}
		}

		// The planting names species, never retired living.<species> palette keys (the contract).
		FString Planting;
		if (TestTrue(TEXT("the planting schedule reads"), ReadSchedule(TEXT("Planting.json"), Planting)))
		{
			TestFalse(TEXT("no bed names a living.<species> material"), Planting.Contains(TEXT("\"living.")));
			TestEqual(TEXT("every bed names its species"), ValuesOf(Planting, TEXT("species")).Num(), ValuesOf(Planting, TEXT("bed")).Num());
		}

		// The description is a plain function of the plan: the same on every call.
		FSiteDescription Again;
		GetDefault<UGroundsRoom>()->Describe(Again);
		bool bSame = Again.Pieces.Num() == Description.Pieces.Num() && Again.Anchors.Num() == Description.Anchors.Num();
		for (int32 Index = 0; bSame && Index < Again.Pieces.Num(); ++Index)
		{
			bSame = Again.Pieces[Index].Name == Description.Pieces[Index].Name && Again.Pieces[Index].Frame.OriginMeters == Description.Pieces[Index].Frame.OriginMeters;
		}
		TestTrue(TEXT("the same pieces and anchors, in the same order and places"), bSame);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundsPlanTest, "Musee.Grounds.Plan", Flags)
	bool FGroundsPlanTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription& Description = Described();

		// §3.3: seventeen courses fill the parvis, the joint under the portico's first nosing; 27 slabs fill a course.
		const double Reach = Plan::SalonStripEdgeY + Plan::ParvisCourses * Plan::CourseDepth;
		TestTrue(TEXT("17 courses reach under the kerb at y 23.00"), Reach >= Plan::ParvisSouthY - Plan::KerbHalf && Reach <= Plan::ParvisSouthY);
		TestEqual(TEXT("the tenth joint falls on the nosing at y 17.30"), Plan::SalonStripEdgeY + 10 * Plan::CourseDepth, 17.3, 1e-9);
		TestEqual(TEXT("27 slabs span the parvis"), Plan::SlabsPerCourse * Plan::SlabLength, 32.0, 0.001);
		TestEqual(TEXT("nine full courses of 27 and eight offset of 28"), CountPieces(Description, TEXT("parvis.")), 9 * 27 + 8 * 28);
		const FFormDrawing* Middle = Description.Pieces.FindByPredicate([](const FFormDrawing& D) { return D.Name == FName(TEXT("parvis.c01.s14")); });
		if (TestNotNull(TEXT("the first course's middle slab"), Middle))
		{
			TestEqual(TEXT("is centred on the axis x -44.00"), BoxOf(*Middle).GetCenter().X, -44.0, 1e-6);
		}

		// §3.1, §3.5: the walk east's kerbs end on the ring's kerb line, the path's on the ring's and the terrace's.
		TestEqual(TEXT("the walk's kerbs end at x 47.292"), RingWestAt(40.0, Plan::RingRadius).X, 47.292, 0.001);
		TestEqual(TEXT("the ring's gap is +/-16.60 degrees about the west"),
			FMath::RadiansToDegrees(FMath::Asin(2.0 / Plan::RingRadius)), 16.60, 0.01);
		TestEqual(TEXT("the path starts at y 19.297"), TerraceSouthAt(52.0, Plan::TerraceOuterRadius).Y, 19.297, 0.001);
		TestEqual(TEXT("and ends at y 35.292"), RingNorthAt(52.0, Plan::RingRadius).Y, 35.292, 0.001);
		TestEqual(TEXT("the terrace's gap is +/-5.92 degrees about the south"),
			FMath::RadiansToDegrees(FMath::Asin(2.0 / Plan::TerraceOuterRadius)), 5.92, 0.01);
		const double TerraceArea = (2.0 * Plan::TerraceEndDeg / 360.0) * UE_DOUBLE_PI
			* (FMath::Square(Plan::TerraceOuterRadius) - FMath::Square(Plan::TerraceInnerRadius));
		TestEqual(TEXT("the terrace is 309 m2"), TerraceArea, 309.0, 0.5);

		// Each kerb run's stones meet end to end, and every kerb stands on its sheet's line.
		for (const TPair<FString, FRun>& Run : KerbRuns())
		{
			const TArray<FRunStone> Stones = Run.Value.Stones(Plan::KerbStone);
			for (int32 Index = 1; Index < Stones.Num(); ++Index)
			{
				TestTrue(*FString::Printf(TEXT("%s's stones %d and %d meet"), *Run.Key, Index, Index + 1),
					EndOf(Stones[Index - 1].Path).Equals(Stones[Index].Path.StartPoint(), 1e-9));
			}
			for (const FRunStone& Stone : Stones)
			{
				// A leg divides evenly into the nearest whole number of 1.20 stones, so none is under 0.8 or over 1.8 long.
				const double Chord = FVector2D::Distance(Stone.Path.StartPoint(), EndOf(Stone.Path));
				TestTrue(*FString::Printf(TEXT("%s's stones are about 1.20 long"), *Run.Key), Chord >= 0.8 && Chord <= 1.8);
			}
		}
		TestEqual(TEXT("the kerb stones are drawn"), CountPieces(Description, TEXT("kerb.")), [] {
			int32 Count = 0;
			for (const TPair<FString, FRun>& Run : KerbRuns()) { Count += Run.Value.Stones(Plan::KerbStone).Num(); }
			return Count; }());

		// §3.6: the copings' stones and the basins' water.
		TestEqual(TEXT("the canal's coping in 62 stones: 24 on each long side, 7 on each end"), CountPieces(Description, TEXT("canal.coping.")), 62);
		TestEqual(TEXT("the round basin's in 12"), CountPieces(Description, TEXT("rond-point.coping.")), 12);
		const double Wet = Plan::CopingWidth - Plan::WettedFace;
		const double CanalWater = (Plan::CanalEastX - Plan::CanalWestX - 2.0 * Wet) * (Plan::CanalSouthY - Plan::CanalNorthY - 2.0 * Wet);
		TestEqual(TEXT("the canal's water to its wetted faces is 234.97 m2"), CanalWater, 234.97, 0.01);
		TestEqual(TEXT("its bed is 0.55 under the water"), Plan::WaterLevel - Plan::BasinFloorTop, 0.55, 1e-9);

		// §3.7: ten standards, their bases 0.18 off the coping's face, 4.01 overall, the globe seated r 0.116 in the cup.
		TestEqual(TEXT("ten posts"), CountPieces(Description, TEXT("lamp."), TEXT(".post")), 10);
		TestEqual(TEXT("ten globes"), CountPieces(Description, TEXT("lamp."), TEXT(".globe")), 10);
		TestEqual(TEXT("ten finials"), CountPieces(Description, TEXT("lamp."), TEXT(".finial")), 10);
		TestEqual(TEXT("a canal lamp's base stands 0.18 off the coping"), FMath::Abs(Plan::Lamps[0].X - Plan::CanalWestX) - Plan::PostBaseRadius, 0.18, 1e-9);
		TestEqual(TEXT("the lamp is 4.01 overall"), Plan::FinialSection[UE_ARRAY_COUNT(Plan::FinialSection) - 1].Z, 4.01, 1e-9);
		TestEqual(TEXT("the globe's lower opening is r 0.116"),
			FMath::Sqrt(FMath::Square(Plan::GlobeRadius) - FMath::Square(Plan::GlobeCentreZ - Plan::CupTop)), 0.116, 0.001);

		// §3.5: the strip's edge stands at y 9.16 along the Salon and r 12.76 round the drum.
		TestEqual(TEXT("the Salon's strip edge at y 9.16"), Plan::SalonHalf + Plan::StripEdgeOuter, 9.16, 1e-9);
		TestEqual(TEXT("the drum's at r 12.76"), Plan::DrumRadius + Plan::StripEdgeOuter, 12.76, 1e-9);
		return true;
	}
}

#endif
