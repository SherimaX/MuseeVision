#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ClassicalHallParts.h"
#include "ClassicalHallPlan.h"
#include "ClassicalHallRoom.h"
#include "Misc/FileHelper.h"
#include "MuseeRecords.h"
#include "SiteDescription.h"
#include "SitePortClass.h"

/**
 * The hall's description against its sheet and its schedules: its port and the well it receives, its Mount anchors
 * against every anchor the hanging plan names, the stage contract's own check, and the sheet's numbers from its setting
 * out. Its shells are proved sealed by Studio's Musee.Studio.RoomsSealed, which makes its drawings.
 */
namespace Musee::ClassicalHall::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		/** The museum's frame is the hall's moved by this (§1): museum = local + (0, −39.30, −6.00). */
		const FVector ToMuseum(0.0, -39.3, -6.0);

		FSiteDescription Described()
		{
			FSiteDescription Description;
			GetDefault<UClassicalHallRoom>()->Describe(Description);
			return Description;
		}

		FString Schedule(const TCHAR* File)
		{
			return Musee::Records::DataRoot() / FString::Printf(TEXT("Stages/%s/%s"), Plan::Stage, File);
		}

		/** One work of the hanging plan as its text gives it: the anchor, the carrier's top centre and the facing. */
		struct FHung
		{
			FString Anchor;
			FVector Centre;
			double FacingDeg = 0.0;
		};

		/** The number after a field's name in a JSON text, from a position on. */
		double NumberAfter(const FString& Text, const TCHAR* Field, int32& At)
		{
			At = Text.Find(Field, ESearchCase::CaseSensitive, ESearchDir::FromStart, At) + FCString::Strlen(Field);
			return FCString::Atod(*Text.Mid(At, 32).TrimStart());
		}

		/**
		 * The hanging plan's works, read from its text: each entry's anchor, its centre and its facing. (Hang.json's scans
		 * carry their turn as a matrix, which no record type reads, so the check reads the fields it needs.)
		 */
		TArray<FHung> ReadHang(const FString& Text)
		{
			TArray<FHung> Out;
			const TCHAR* AnchorField = TEXT("\"anchor\": \"");
			for (int32 At = Text.Find(AnchorField); At != INDEX_NONE; At = Text.Find(AnchorField, ESearchCase::CaseSensitive, ESearchDir::FromStart, At + 1))
			{
				FHung& Work = Out.AddDefaulted_GetRef();
				const int32 From = At + FCString::Strlen(AnchorField);
				Work.Anchor = Text.Mid(From, Text.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, From) - From);
				int32 Cursor = From;
				Work.Centre.X = NumberAfter(Text, TEXT("\"centre\": ["), Cursor);
				Work.Centre.Y = NumberAfter(Text, TEXT(","), Cursor);
				Work.Centre.Z = NumberAfter(Text, TEXT(","), Cursor);
				Work.FacingDeg = NumberAfter(Text, TEXT("\"facing_deg\":"), Cursor);
			}
			return Out;
		}

		const FMuseeStageAnchor* FindMount(const FSiteDescription& Description, const FString& Key)
		{
			return Description.Anchors.FindByPredicate([&Key](const FMuseeStageAnchor& A) { return A.Key == FMuseeKey(Key) && A.Kind == FName(TEXT("Mount")); });
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClassicalHallPortsTest, "Musee.ClassicalHall.Ports", Flags)
	bool FClassicalHallPortsTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		if (!TestEqual(TEXT("one port"), Description.Ports.Num(), 1))
		{
			return false;
		}
		const FSitePort& Stair = Description.Ports[0];
		TestEqual(TEXT("it is the stair"), Stair.Key, FMuseeKey(TEXT("classical-hall.stair")));
		TestEqual(TEXT("a stairwell"), Stair.Kind, ESitePortKind::Stairwell);
		TestEqual(TEXT("of the Rotunda's class"), Stair.Class, FMuseeKey(TEXT("stairwell.spiral.3600x4400.arched")));
		TestEqual(TEXT("meeting the Rotunda's stair"), Stair.Mates, FMuseeKey(TEXT("rotunda.stair")));
		TestTrue(TEXT("on the port face on the axis, at the threshold"), Stair.Frame.OriginMeters.Equals(FVector(0.0, 31.7, 0.0), 1e-9));
		TestTrue(TEXT("facing south, into the well"), Stair.Facing().Equals(FVector(0.0, 1.0, 0.0), 1e-9));
		TestTrue(TEXT("its frame in the museum is the Rotunda's rotunda.stair"), (Stair.Frame.OriginMeters + ToMuseum).Equals(FVector(0.0, -7.6, -6.0), 1e-9));
		TestEqual(TEXT("0.600 deep on the axis"), Stair.WallDepthMeters, 0.6, 1e-9);
		TestEqual(TEXT("its face the well's cylinder, R 7.60"), Stair.FaceRadiusMeters, 7.6, 1e-9);
		TestTrue(TEXT("the hall receives the well"), Stair.bHasWell && !Stair.Well.bServing);
		TestTrue(TEXT("the well is the Rotunda's prism, r 7.60 about W, museum z −6.40 to 0.00"),
			(Stair.Well.MinMeters + ToMuseum).Equals(FVector(-7.6, -7.6, -6.4), 1e-9) && (Stair.Well.MaxMeters + ToMuseum).Equals(FVector(7.6, 7.6, 0.0), 1e-9));

		// The class is the Rotunda's record in the site's port classes; the rough opening is its opening grown by the lining.
		const TValueOrError<FSitePortClass, FString> Class = Musee::Site::FindPortClass(Stair.Class);
		if (Class.HasValue())
		{
			TestEqual(TEXT("the opening is the class's clear width"), 2.0 * Plan::PortClearHalf, Class.GetValue().ClearWidthMeters, 1e-9);
			TestEqual(TEXT("and springs at its head"), Plan::PortSpringing, Class.GetValue().HeadMeters, 1e-9);
		}
		else
		{
			AddWarning(FString::Printf(TEXT("the port's class isn't in the site's records yet: %s"), *Class.GetError()));
		}
		const FFormOpening* Rough = Description.Openings.FindByPredicate([](const FFormOpening& O) { return O.Kind == EFormOpeningKind::Port; });
		if (TestNotNull(TEXT("the rough opening is declared"), Rough))
		{
			TestTrue(TEXT("3.70 wide and 4.50 high (clear + 0.05 on every side)"), Rough->SizeMeters.Equals(FVector2D(3.7, 4.5), 1e-9));
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClassicalHallAnchorsTest, "Musee.ClassicalHall.Anchors", Flags)
	bool FClassicalHallAnchorsTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		TestEqual(TEXT("46 Mount anchors, one per carrier"), Description.Anchors.Num(), 46);
		TSet<FMuseeKey> Keys;
		for (const FMuseeStageAnchor& Anchor : Description.Anchors)
		{
			TestEqual(*FString::Printf(TEXT("%s is a Mount"), *Anchor.Key.ToString()), Anchor.Kind, FName(TEXT("Mount")));
			TestFalse(*FString::Printf(TEXT("%s is declared once"), *Anchor.Key.ToString()), Keys.Contains(Anchor.Key));
			Keys.Add(Anchor.Key);
			TestTrue(*FString::Printf(TEXT("%s's z is up"), *Anchor.Key.ToString()), Anchor.Frame.DirectionToParent(FVector::UpVector).Equals(FVector::UpVector, 1e-9));
			const FString Carrier = Anchor.Key.ToString().RightChop(FCString::Strlen(Plan::Stage) + 1) + TEXT(".carrier");
			TestTrue(*FString::Printf(TEXT("%s stands on its carrier"), *Anchor.Key.ToString()),
				Description.Pieces.ContainsByPredicate([&Carrier](const FFormDrawing& Piece) { return Piece.Name.ToString().StartsWith(Carrier); }));
		}

		FString Hang;
		if (!TestTrue(TEXT("the hanging plan reads"), FFileHelper::LoadFileToString(Hang, *Schedule(TEXT("Hang.json")))))
		{
			return false;
		}
		const TArray<FHung> Works = ReadHang(Hang);
		TestEqual(TEXT("41 casts"), Works.Num(), 41);
		for (const FHung& Work : Works)
		{
			const FMuseeStageAnchor* Anchor = FindMount(Description, Work.Anchor);
			if (!TestNotNull(*FString::Printf(TEXT("%s is a Mount anchor"), *Work.Anchor), Anchor))
			{
				continue;
			}
			TestTrue(*FString::Printf(TEXT("%s stands at its carrier's top centre"), *Work.Anchor), Anchor->Frame.OriginMeters.Equals(Work.Centre, 1e-6));
			const FVector Facing = Anchor->Frame.DirectionToParent(FVector::ForwardVector);
			TestTrue(*FString::Printf(TEXT("%s faces as its work does"), *Work.Anchor), Facing.Equals(Direction(Work.FacingDeg), 1e-6));
		}

		// The five empty consoles face O.
		for (const TCHAR* Empty : {TEXT("n-b"), TEXT("se-a"), TEXT("se-b"), TEXT("sw-a"), TEXT("sw-b")})
		{
			const FMuseeStageAnchor* Anchor = FindMount(Description, FString::Printf(TEXT("classical-hall.tribune.bust.%s"), Empty));
			if (TestNotNull(*FString::Printf(TEXT("the empty console %s"), Empty), Anchor))
			{
				const FVector ToO = (-Anchor->Frame.OriginMeters * FVector(1.0, 1.0, 0.0)).GetSafeNormal();
				TestTrue(*FString::Printf(TEXT("%s faces O"), Empty), Anchor->Frame.DirectionToParent(FVector::ForwardVector).Equals(ToO, 1e-9));
			}
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClassicalHallDescribedTest, "Musee.ClassicalHall.Described", Flags)
	bool FClassicalHallDescribedTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		TestEqual(TEXT("it is the classical-hall stage"), Description.Stage, FMuseeKey(TEXT("classical-hall")));
		const TArray<FMuseeFinding> Findings = Musee::Site::CheckDescription(Description);
		for (const FMuseeFinding& Finding : Findings)
		{
			AddError(Finding.Summary);
		}
		TestEqual(TEXT("the stage contract holds"), Findings.Num(), 0);
		if (TestEqual(TEXT("one enclosure"), Description.Enclosures.Num(), 1))
		{
			const FFormEnclosure& Interior = Description.Enclosures[0];
			TestEqual(TEXT("the interior"), Interior.Key, FMuseeKey(TEXT("classical-hall.interior")));
			TestEqual(TEXT("enclosed"), Interior.Kind, EFormEnclosureKind::Enclosed);
			TestTrue(TEXT("bounded by the shell, slab to roof"), Interior.Pieces.Contains(FName(TEXT("shell.slab"))) && Interior.Pieces.Contains(FName(TEXT("shell.tribune.vault"))));
		}
		int32 Housings = 0;
		for (const FFormOpening& Opening : Description.Openings)
		{
			TestTrue(*FString::Printf(TEXT("%s lets no daylight in"), *Opening.Key.ToString()),
				Opening.Kind != EFormOpeningKind::Glazed && Opening.Kind != EFormOpeningKind::Open);
			Housings += Opening.Kind == EFormOpeningKind::Housing ? 1 : 0;
		}
		TestEqual(TEXT("the five laylights' housings"), Housings, 5);

		// Every surface a piece names is in the stage's finish schedule.
		FString Finishes;
		if (TestTrue(TEXT("the finish schedule reads"), FFileHelper::LoadFileToString(Finishes, *Schedule(TEXT("Finishes.json")))))
		{
			TSet<FName> Surfaces;
			for (const FFormDrawing& Piece : Description.Pieces)
			{
				Surfaces.Add(Piece.Surface);
			}
			for (const FName Surface : Surfaces)
			{
				TestTrue(*FString::Printf(TEXT("%s is scheduled"), *Surface.ToString()),
					Finishes.Contains(FString::Printf(TEXT("\"surface\": \"%s\""), *Surface.ToString())));
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

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClassicalHallPlanTest, "Musee.ClassicalHall.Plan", Flags)
	bool FClassicalHallPlanTest::RunTest(const FString& Parameters)
	{
		// The footprint (§1): from the south-east corner, round the port cylinder through (0, 31.70), and round the tribune's shell.
		const FSiteDescription Description = Described();
		const TArray<FVector2D>& Outline = Description.Footprint.OutlineMeters;
		TestTrue(TEXT("the footprint starts at the south-east corner"), Outline[0].Equals(FVector2D(4.2, 31.4), 1e-9));
		FBox2D Bounds(Outline);
		TestEqual(TEXT("the tribune's shell reaches x −7.90"), Bounds.Min.X, -7.9, 1e-9);
		TestEqual(TEXT("and y −7.90 by the north"), Bounds.Min.Y, -7.9, 1e-9);
		TestEqual(TEXT("the sleeve meets the port cylinder at y 31.996"), Bounds.Max.Y, 31.996, 0.001);
		double Crown = Bounds.Max.Y;
		for (const FVector2D& Point : Outline)
		{
			Crown = Point.Y > Plan::ShellSouthY ? FMath::Min(Crown, Point.Y) : Crown;
		}
		TestEqual(TEXT("which bulges into the hall to y 31.70 on the axis (stations 1.5° apart)"), Crown, 31.7, 0.001);
		TestTrue(TEXT("from the slab's underside to the roof's top"), Description.Footprint.BottomMeters == -0.4 && Description.Footprint.TopMeters == 5.45);

		// The section (§3.3): the barrel springs from the ledge at x 3.00 and crowns under the roof slab.
		TestTrue(TEXT("the barrel springs at (3.00, 3.70)"), Barrel(Plan::VaultSpringingS, Plan::VaultRadius).Equals(FVector2D(3.0, 3.7), 0.001));
		TestEqual(TEXT("and crowns at 5.15"), Barrel(0.0, Plan::VaultRadius).Y, 5.15, 1e-9);
		TestTrue(TEXT("its coffer field's edge at (2.7023, 4.0335)"), Barrel(Plan::CofferFieldS, Plan::VaultRadius).Equals(FVector2D(2.7023, 4.0335), 0.001));
		TestTrue(TEXT("the lantern's inner edge at (1.1232, 4.9815)"),
			Barrel(Plan::LanternHalf - Plan::LanternFrame, Plan::VaultRadius).Equals(FVector2D(1.1232, 4.9815), 0.001));
		const FVector2D Annular(Plan::AnnularCentreR, Plan::AnnularCentreZ);
		TestEqual(TEXT("the annular vault springs from (6.60, 3.70)"), FVector2D::Distance(Annular, FVector2D(6.6, 3.7)), Plan::AnnularRadius, 0.001);
		TestEqual(TEXT("and from (3.60, 3.70)"), FVector2D::Distance(Annular, FVector2D(3.6, 3.7)), Plan::AnnularRadius, 0.001);
		TestEqual(TEXT("and crowns at 5.15"), Plan::AnnularCentreZ + Plan::AnnularRadius, 5.15, 1e-9);
		const FVector2D Sphere(0.0, Plan::DomeCentreZ);
		TestEqual(TEXT("the dome springs from (3.20, 3.70)"), FVector2D::Distance(Sphere, FVector2D(3.2, 3.7)), Plan::DomeRadius, 0.001);
		TestEqual(TEXT("and meets the eye at (1.20, 4.94)"), FVector2D::Distance(Sphere, FVector2D(1.2, 4.94)), Plan::DomeRadius, 0.001);
		for (const auto& Edge : Plan::DomeRingEdges)
		{
			TestEqual(TEXT("each coffer ring's edge lies on the cap"), FVector2D::Distance(Sphere, FVector2D(Edge[0], Edge[1])), Plan::DomeRadius, 0.001);
		}
		TestEqual(TEXT("the throat meets the tribune's wall at y 5.8788"), FMath::Sqrt(6.6 * 6.6 - 9.0), 5.8788, 0.0001);

		// The coffers: 256 in the gallery (each bay's 10 × 8 less its lantern's 4 × 4), 420 in the annular vault, 72 in the dome.
		const int32 Rows = FMath::RoundToInt32(2.0 * Plan::CofferFieldS / Plan::CofferCell);
		const int32 Columns = FMath::RoundToInt32(2.0 * Plan::BayCofferHalf / Plan::CofferCell);
		const int32 Lantern = FMath::RoundToInt32(2.0 * Plan::LanternHalf / Plan::CofferCell);
		TestEqual(TEXT("256 coffers in the gallery"), Plan::Bays * (Rows * Columns - Lantern * Lantern), 256);
		int32 Annulars = 0;
		int32 Domes = 0;
		for (const FFormDrawing& Piece : Description.Pieces)
		{
			Annulars += Piece.Name.ToString().StartsWith(TEXT("tribune.vault.annular.rosette.")) ? 1 : 0;
			Domes += Piece.Name.ToString().StartsWith(TEXT("tribune.vault.dome.rosette.")) ? 1 : 0;
		}
		TestEqual(TEXT("420 coffers in the annular vault"), Annulars, 420);
		TestEqual(TEXT("72 in the dome"), Domes, 72);
		return true;
	}
}

#endif
