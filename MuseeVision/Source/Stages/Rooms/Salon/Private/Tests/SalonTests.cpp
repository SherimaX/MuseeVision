#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "MuseeRecords.h"
#include "NaturePlanting.h"
#include "SalonParts.h"
#include "SalonPlan.h"
#include "SalonRoom.h"
#include "SiteDescription.h"
#include "SitePortClass.h"

/**
 * The Salon's description against its sheet and its schedules: its ports and their classes, its anchors against every
 * anchor the schedules name, the stage contract's own check, the plan's measured numbers, and a description that is the
 * same on every call. Its shells are proved sealed by Studio's Musee.Studio.RoomsSealed, which makes its drawings.
 */
namespace Musee::Salon::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		FSiteDescription Described()
		{
			FSiteDescription Description;
			GetDefault<USalonRoom>()->Describe(Description);
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

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSalonPortsTest, "Musee.Salon.Ports", Flags)
	bool FSalonPortsTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		TestEqual(TEXT("two ports"), Description.Ports.Num(), 2);
		const FSitePort* East = FindPort(Description, TEXT("salon.east"));
		const FSitePort* West = FindPort(Description, TEXT("salon.west"));
		if (!TestNotNull(TEXT("the east door"), East) || !TestNotNull(TEXT("the west door"), West))
		{
			return false;
		}
		TestTrue(TEXT("the east door stands on the end wall's outer face on the axis"), East->Frame.OriginMeters.Equals(FVector(0.6, 0.0, 0.0), 1e-9));
		TestTrue(TEXT("and faces east"), East->Facing().Equals(FVector(1.0, 0.0, 0.0), 1e-9));
		TestEqual(TEXT("it meets the Rotunda's west door"), East->Mates, FMuseeKey(TEXT("rotunda.west")));
		TestEqual(TEXT("in the Rotunda's class"), East->Class, FMuseeKey(TEXT("doorway.3000x6000.arched")));
		TestTrue(TEXT("the west door stands on the far wall's outer face on the axis"), West->Frame.OriginMeters.Equals(FVector(-60.6, 0.0, 0.0), 1e-9));
		TestTrue(TEXT("and faces west"), West->Facing().Equals(FVector(-1.0, 0.0, 0.0), 1e-9));
		TestEqual(TEXT("it meets the oval's east door"), West->Mates, FMuseeKey(TEXT("nympheas.east")));
		TestEqual(TEXT("both walls are 0.60 deep"), East->WallDepthMeters + West->WallDepthMeters, 1.2);

		// The rough openings the room builds are its ports' classes.
		const TValueOrError<FSitePortClass, FString> Oval = Musee::Site::FindPortClass(West->Class);
		if (TestTrue(TEXT("the west class is in the site's records"), Oval.HasValue()))
		{
			TestEqual(TEXT("its opening is the doors' width"), Oval.GetValue().ClearWidthMeters, Plan::DoorWidth);
			TestEqual(TEXT("and springs at their springing"), Oval.GetValue().HeadMeters, Plan::DoorSpringing);
		}
		const TValueOrError<FSitePortClass, FString> Rotunda = Musee::Site::FindPortClass(East->Class);
		if (Rotunda.HasValue())
		{
			TestEqual(TEXT("the Rotunda's class is the doors' width"), Rotunda.GetValue().ClearWidthMeters, Plan::DoorWidth);
			TestEqual(TEXT("and springs at 4.50"), Rotunda.GetValue().HeadMeters, Plan::EastClassSpringing);
		}
		else
		{
			AddWarning(FString::Printf(TEXT("the east port's class is not yet in the site's records (Site's): %s"), *Rotunda.GetError()));
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSalonAnchorsTest, "Musee.Salon.Anchors", Flags)
	bool FSalonAnchorsTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		// The hanging schedule is read as text: its entries on mounts (the case's documents, the dancer) carry fields
		// Art's hanging record doesn't have yet, so Art's reader can't read the file whole.
		FString Hang;
		const TValueOrError<FNaturePlanting, FString> Planting = Musee::ReadRecord<FNaturePlanting>(Schedule(TEXT("Planting.json")));
		const bool bHang = FFileHelper::LoadFileToString(Hang, *(Musee::Records::DataRoot() / Schedule(TEXT("Hang.json"))));
		if (!TestTrue(TEXT("the hanging and planting schedules read"), bHang && Planting.HasValue()))
		{
			AddError(Planting.HasError() ? Planting.GetError() : TEXT("Hang.json can't be read"));
			return false;
		}
		// Canvases hang on walls; the case's documents and the dancer stand on mounts.
		TArray<FString> Entries;
		Hang.ParseIntoArray(Entries, TEXT("\"work\":"), true);
		int32 Works = 0;
		for (const FString& Entry : Entries)
		{
			FRegexMatcher AnchorMatch(FRegexPattern(TEXT("\"anchor\":\\s*\"([^\"]+)\"")), Entry);
			FRegexMatcher SupportMatch(FRegexPattern(TEXT("\"support\":\\s*\"([^\"]+)\"")), Entry);
			if (!AnchorMatch.FindNext() || !SupportMatch.FindNext())
			{
				continue;
			}
			++Works;
			const FName Kind = SupportMatch.GetCaptureGroup(1) == TEXT("canvas") ? FName(TEXT("Wall")) : FName(TEXT("Mount"));
			const FString Key = AnchorMatch.GetCaptureGroup(1);
			TestNotNull(*FString::Printf(TEXT("%s is a %s anchor"), *Key, *Kind.ToString()), FindAnchor(Description, FMuseeKey(Key), Kind));
		}
		TestEqual(TEXT("39 canvases, the case's two documents and the dancer"), Works, 42);
		for (const FNatureBed& Bed : Planting.GetValue().Beds)
		{
			TestNotNull(*FString::Printf(TEXT("%s is a Bed anchor"), *Bed.Bed.ToString()), FindAnchor(Description, Bed.Bed, TEXT("Bed")));
		}

		// Every hanging wall's frame: x along the face, y into the room (towards the axis or off the wall), z up.
		int32 Walls = 0;
		int32 Seats = 0;
		for (const FMuseeStageAnchor& Each : Description.Anchors)
		{
			if (Each.Kind == FName(TEXT("Wall")))
			{
				++Walls;
				TestTrue(*FString::Printf(TEXT("%s stands upright"), *Each.Key.ToString()),
					Each.Frame.DirectionToParent(FVector::UpVector).Equals(FVector::UpVector, 1e-9));
			}
			if (Each.Kind == FName(TEXT("Seat")))
			{
				++Seats;
				TestEqual(*FString::Printf(TEXT("%s's seat is at 0.44"), *Each.Key.ToString()), Each.Frame.OriginMeters.Z, Plan::SeatTop);
			}
		}
		TestEqual(TEXT("fourteen walls, the stele and the cabinet's three"), Walls, 2 * Plan::BayCount + 1 + 1 + 3);
		TestEqual(TEXT("eight benches, two frames on each face"), Seats, 8 * 4);
		const FMuseeStageAnchor* North = FindAnchor(Description, FMuseeKey(TEXT("salon.bay2.wall.north")), TEXT("Wall"));
		if (TestNotNull(TEXT("bay 2's north wall"), North))
		{
			TestTrue(TEXT("starts at its west end on the silk"), North->Frame.OriginMeters.Equals(FVector(-23.4, -7.0, 0.0), 1e-9));
			TestTrue(TEXT("and looks south"), North->Frame.DirectionToParent(FVector(0.0, 1.0, 0.0)).Equals(FVector(0.0, 1.0, 0.0), 1e-9));
		}
		const FMuseeStageAnchor* Stele = FindAnchor(Description, FMuseeKey(TEXT("salon.bay1.stele.east")), TEXT("Wall"));
		if (TestNotNull(TEXT("the stele's east face"), Stele))
		{
			TestTrue(TEXT("starts at its south end on the glass"), Stele->Frame.OriginMeters.Equals(FVector(-6.125, 2.0, 0.0), 1e-9));
			TestTrue(TEXT("and looks east"), Stele->Frame.DirectionToParent(FVector(0.0, 1.0, 0.0)).Equals(FVector(1.0, 0.0, 0.0), 1e-9));
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSalonDescribedTest, "Musee.Salon.Described", Flags)
	bool FSalonDescribedTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		TestEqual(TEXT("it is the salon stage"), Description.Stage, FMuseeKey(TEXT("salon")));
		const TArray<FMuseeFinding> Findings = Musee::Site::CheckDescription(Description);
		for (const FMuseeFinding& Finding : Findings)
		{
			AddError(Finding.Summary);
		}
		TestEqual(TEXT("the stage contract holds"), Findings.Num(), 0);
		TestEqual(TEXT("three enclosures"), Description.Enclosures.Num(), 3);
		int32 Lanterns = 0;
		for (const FFormOpening& Opening : Description.Openings)
		{
			const bool bLantern = Opening.Kind == EFormOpeningKind::Glazed && Opening.Enclosure == FMuseeKey(TEXT("salon.bays"))
				&& Opening.Glazing == FName(TEXT("salon.lantern.outer.glass"));
			Lanterns += bLantern ? 1 : 0;
		}
		TestEqual(TEXT("the five lanterns are the bays' only openings to the sky"), Lanterns, 5);

		// Every surface a piece names is in the stage's finish schedule.
		FString Finishes;
		if (TestTrue(TEXT("the finish schedule reads"), FFileHelper::LoadFileToString(Finishes, *(Musee::Records::DataRoot() / Schedule(TEXT("Finishes.json"))))))
		{
			TSet<FName> Missing;
			for (const FFormDrawing& Piece : Description.Pieces)
			{
				TArray<FName> Surfaces;
				Piece.PartSurfaces.GenerateValueArray(Surfaces);
				Surfaces.Add(Piece.Surface);
				if (const FFormCeiling* Ceiling = Piece.Shape.TryGet<FFormCeiling>())
				{
					Surfaces.Add(Ceiling->GlazingSurface);
				}
				for (const FName Surface : Surfaces)
				{
					if (!Finishes.Contains(FString::Printf(TEXT("\"surface\": \"%s\""), *Surface.ToString())))
					{
						Missing.Add(Surface);
					}
				}
			}
			for (const FName Surface : Missing)
			{
				AddError(FString::Printf(TEXT("surface %s is not in the finish schedule"), *Surface.ToString()));
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

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSalonPlanTest, "Musee.Salon.Plan", Flags)
	bool FSalonPlanTest::RunTest(const FString& Parameters)
	{
		// The sheet's measured numbers, from the plan's setting-out (§3, §3.4).
		for (int32 Index = 0; Index < Plan::PierCount; ++Index)
		{
			TestEqual(TEXT("each pier's east face is its eastern bay's west face"), Plan::PierX[Index] + Plan::PierHalf, Plan::Bays[Index].West, 1e-9);
			TestEqual(TEXT("and its west face the next bay's east face"), Plan::PierX[Index] - Plan::PierHalf, Plan::Bays[Index + 1].East, 1e-9);
		}
		TestEqual(TEXT("bays 2 to 4 have square coffers, 1.1574"), ColumnX(1, 1) - ColumnX(1, 0), UE_DOUBLE_PI * Plan::VaultRadius / Plan::Rows, 1e-4);
		TestEqual(TEXT("bays 1 and 5 pitch 1.2324"), ColumnX(0, 1) - ColumnX(0, 0), 1.2324, 1e-4);
		const FBox2D Bay1 = Lantern(0);
		TestTrue(TEXT("bay 1's lantern runs from x -6.9324 to -4.4676"), FMath::IsNearlyEqual(Bay1.Min.X, -6.9324, 1e-4) && FMath::IsNearlyEqual(Bay1.Max.X, -4.4676, 1e-4));
		TestTrue(TEXT("across y ±0.5781"), FMath::IsNearlyEqual(Bay1.Max.Y, 0.5781, 1e-4) && FMath::IsNearlyEqual(Bay1.Min.Y, -Bay1.Max.Y, 1e-9));
		const double Centres[Plan::BayCount] = {-5.7, -18.0, -30.0, -42.0, -54.3};
		for (int32 Bay = 0; Bay < Plan::BayCount; ++Bay)
		{
			TestEqual(*FString::Printf(TEXT("bay %d's lantern is centred on the sheet's x"), Bay + 1), Lantern(Bay).GetCenter().X, Centres[Bay], 1e-4);
		}
		const double Ridge = Plan::KerbTop + Bay1.Max.Y * FMath::Tan(FMath::DegreesToRadians(Plan::OuterPitchDeg));
		TestEqual(TEXT("the outer lanterns' ridge is at 14.534"), Ridge, 14.534, 1e-3);
		TestEqual(TEXT("the bands are the bays' length less the columns"), ColumnX(2, Plan::Columns) + Plan::Band, Plan::Bays[2].East, 1e-9);

		const FSiteDescription Description = Described();
		int32 Rosettes = 0;
		for (const FFormDrawing& Piece : Description.Pieces)
		{
			Rosettes += Piece.Name.ToString().StartsWith(TEXT("rosette.")) ? 1 : 0;
		}
		TestEqual(TEXT("750 rosettes, one on every coffer"), Rosettes, 750);
		TestEqual(TEXT("the footprint has the sheet's eight corners"), Description.Footprint.OutlineMeters.Num(), 8);
		return true;
	}
}

#endif
