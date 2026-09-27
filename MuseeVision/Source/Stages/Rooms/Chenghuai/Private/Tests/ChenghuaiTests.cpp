#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ChenghuaiHall.h"
#include "ChenghuaiPlan.h"
#include "ChenghuaiPlanGarden.h"
#include "ChenghuaiPlanHalls.h"
#include "ChenghuaiRoom.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "MuseeRecords.h"
#include "SiteDescription.h"
#include "SitePortClass.h"

/**
 * Chenghuai's description against its sheet and its schedules: its port and its class, its anchors against every anchor
 * the schedules name (read from their text, so the check is the stage's own and needs no system's reader), the stage
 * contract's own check with its openings and surfaces, a description the same on every call, and the section rule's
 * heights, the footprint, the pond and the flights against the sheet's numbers. Its shells are proved sealed by Studio's
 * Musee.Studio.RoomsSealed, which makes its drawings.
 */
namespace Musee::Chenghuai::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		FSiteDescription Described()
		{
			FSiteDescription Description;
			GetDefault<UChenghuaiRoom>()->Describe(Description);
			return Description;
		}

		const FMuseeStageAnchor* FindAnchor(const FSiteDescription& Description, const FString& Key, FName Kind)
		{
			return Description.Anchors.FindByPredicate([&](const FMuseeStageAnchor& A) { return A.Key == FMuseeKey(Key) && A.Kind == Kind; });
		}

		FString Schedule(const TCHAR* File)
		{
			FString Text;
			FFileHelper::LoadFileToString(Text, *(Musee::Records::DataRoot() / FString::Printf(TEXT("Stages/%s/%s"), Plan::Stage, File)));
			return Text;
		}

		/** Every value of a field in a schedule's text: "Field": "value". */
		TArray<FString> Values(const FString& Text, const TCHAR* Field)
		{
			TArray<FString> Found;
			FRegexMatcher Matcher(FRegexPattern(FString::Printf(TEXT("\"%s\"\\s*:\\s*\"([^\"]+)\""), Field)), Text);
			while (Matcher.FindNext())
			{
				Found.AddUnique(Matcher.GetCaptureGroup(1));
			}
			return Found;
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChenghuaiPortsTest, "Musee.Chenghuai.Ports", Flags)
	bool FChenghuaiPortsTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		if (!TestEqual(TEXT("one port"), Description.Ports.Num(), 1))
		{
			return false;
		}
		const FSitePort& South = Description.Ports[0];
		TestEqual(TEXT("it is chenghuai.south"), South.Key, FMuseeKey(TEXT("chenghuai.south")));
		TestTrue(TEXT("a doorway"), South.Kind == ESitePortKind::Doorway);
		TestEqual(TEXT("of the class width × crown"), South.Class, FMuseeKey(TEXT("doorway.4000x6500.arched")));
		TestTrue(TEXT("on the drum's face on the axis, at the threshold"), South.Frame.OriginMeters.Equals(FVector(0.0, 5.8, 0.0), 1e-9));
		TestTrue(TEXT("facing south, to the Rotunda"), South.Facing().Equals(FVector(0.0, 1.0, 0.0), 1e-9));
		TestTrue(TEXT("with z up"), South.Frame.DirectionToParent(FVector::UpVector).Equals(FVector::UpVector, 1e-9));
		TestEqual(TEXT("it meets the Rotunda's north door"), South.Mates, FMuseeKey(TEXT("rotunda.north")));
		TestEqual(TEXT("the porch is open: no wall across its mouth"), South.WallDepthMeters, 0.0);
		TestEqual(TEXT("its face is the drum's cylinder"), South.FaceRadiusMeters, 11.2);
		TestFalse(TEXT("it has no well"), South.bHasWell);
		// museum = local + (0, -17.000, 0): the port lands on rotunda.north's frame, (0, -11.200, 0).
		TestTrue(TEXT("in the museum it stands on the drum's north face"),
			(South.Frame.OriginMeters + FVector(0.0, Plan::MuseumOffsetY, 0.0)).Equals(FVector(0.0, -11.2, 0.0), 1e-9));
		TestEqual(TEXT("the drum's face passes through the port"), FVector2D::Distance(FVector2D(0.0, Plan::PortY), FVector2D(0.0, Plan::DrumCentreY)),
			Plan::DrumRadius, 1e-9);
		const TValueOrError<FSitePortClass, FString> Class = Musee::Site::FindPortClass(South.Class);
		if (Class.HasValue())
		{
			TestEqual(TEXT("the class is 4.000 clear"), Class.GetValue().ClearWidthMeters, Plan::PortClearWidth, 1e-9);
			TestEqual(TEXT("and crowns at 6.500"), Class.GetValue().HeadMeters + 0.5 * Class.GetValue().ClearWidthMeters, Plan::PortCrown, 1e-9);
		}
		else
		{
			AddWarning(FString::Printf(TEXT("Site's port classes have no %s yet: %s"), *South.Class.ToString(), *Class.GetError()));
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChenghuaiAnchorsTest, "Musee.Chenghuai.Anchors", Flags)
	bool FChenghuaiAnchorsTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		const FString Hang = Schedule(TEXT("Hang.json"));
		const FString Planting = Schedule(TEXT("Planting.json"));
		if (!TestTrue(TEXT("the hanging and planting schedules read"), !Hang.IsEmpty() && !Planting.IsEmpty()))
		{
			return false;
		}
		for (const FString& Key : Values(Hang, TEXT("anchor")))
		{
			TestTrue(*FString::Printf(TEXT("%s is a Wall or a Mount"), *Key), FindAnchor(Description, Key, TEXT("Wall")) || FindAnchor(Description, Key, TEXT("Mount")));
		}
		for (const FString& Key : Values(Planting, TEXT("anchor")))
		{
			TestNotNull(*FString::Printf(TEXT("%s is a Water anchor"), *Key), FindAnchor(Description, Key, TEXT("Water")));
		}
		for (const FString& Key : Values(Planting, TEXT("bed")))
		{
			TestNotNull(*FString::Printf(TEXT("%s is a Bed anchor"), *Key), FindAnchor(Description, Key, TEXT("Bed")));
		}
		// The walls stand upright with their y out of the face; the handscroll beds tilt 11° towards their reader.
		for (const FMuseeStageAnchor& Anchor : Description.Anchors)
		{
			const FVector Z = Anchor.Frame.DirectionToParent(FVector::UpVector);
			const bool bBed = Anchor.Key.ToString().EndsWith(TEXT(".bed"));
			if (Anchor.Kind == FName(TEXT("Wall")))
			{
				TestEqual(*FString::Printf(TEXT("%s's z leans as its wall does"), *Anchor.Key.ToString()), Z.Z, bBed ? Plan::BedSin : 1.0, 1e-6);
			}
		}
		const FMuseeStageAnchor* Bed = FindAnchor(Description, TEXT("chenghuai.linchi.case.bed"), TEXT("Wall"));
		if (TestNotNull(TEXT("臨池's bed"), Bed))
		{
			TestTrue(TEXT("its face looks up and north to the reader"), Bed->Frame.DirectionToParent(FVector(0.0, 1.0, 0.0)).Equals(FVector(0.0, -Plan::BedSin, Plan::BedCos), 1e-6));
		}
		// The pine stands on the rockery's surface; the seats face the pavilion's centre at their tops.
		const FMuseeStageAnchor* Pine = FindAnchor(Description, TEXT("chenghuai.garden.tree.pine"), TEXT("Bed"));
		if (TestNotNull(TEXT("the pine"), Pine))
		{
			TestTrue(TEXT("up on the rockery"), Pine->Frame.OriginMeters.Z > 1.0);
		}
		int32 Seats = 0;
		for (const FMuseeStageAnchor& Anchor : Description.Anchors)
		{
			if (Anchor.Kind == FName(TEXT("Seat")))
			{
				++Seats;
				const FVector Facing = Anchor.Frame.DirectionToParent(FVector::ForwardVector);
				const FVector ToCentre = (FVector(Plan::PavilionX, Plan::PavilionY, 0.0) - Anchor.Frame.OriginMeters).GetSafeNormal2D();
				TestTrue(*FString::Printf(TEXT("%s faces the pavilion's centre"), *Anchor.Key.ToString()), Facing.Equals(ToCentre, 1e-6));
				TestEqual(TEXT("at the seat's top"), Anchor.Frame.OriginMeters.Z, Plan::SeatTop, 1e-9);
			}
		}
		TestEqual(TEXT("four seats"), Seats, 4);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChenghuaiDescribedTest, "Musee.Chenghuai.Described", Flags)
	bool FChenghuaiDescribedTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		TestEqual(TEXT("it is the chenghuai stage"), Description.Stage, FMuseeKey(TEXT("chenghuai")));
		const TArray<FMuseeFinding> Findings = Musee::Site::CheckDescription(Description);
		for (const FMuseeFinding& Finding : Findings)
		{
			AddError(Finding.Summary);
		}
		TestEqual(TEXT("the stage contract holds"), Findings.Num(), 0);
		AddInfo(FString::Printf(TEXT("%d pieces, %d anchors, %d openings"), Description.Pieces.Num(), Description.Anchors.Num(), Description.Openings.Num()));
		TestEqual(TEXT("fifteen enclosures"), Description.Enclosures.Num(), 15);
		for (const FFormEnclosure& Enclosure : Description.Enclosures)
		{
			TestTrue(*FString::Printf(TEXT("%s is bounded"), *Enclosure.Key.ToString()), Enclosure.Kind == EFormEnclosureKind::Outdoors || !Enclosure.Pieces.IsEmpty());
		}
		// Every opening the lighting schedule lets daylight through is a Glazed opening of a room, glazed in its paper.
		for (const FString& Key : Values(Schedule(TEXT("Lighting.json")), TEXT("opening")))
		{
			TestTrue(*FString::Printf(TEXT("%s is a Glazed opening"), *Key), Description.Openings.ContainsByPredicate([&Key](const FFormOpening& O)
			{
				return O.Key == FMuseeKey(Key) && O.Kind == EFormOpeningKind::Glazed && O.Glazing == FName(TEXT("chenghuai.lattice.paper"));
			}));
		}
		for (const TCHAR* Court : {TEXT("entry-yard"), TEXT("front-court"), TEXT("inner-court"), TEXT("rear-court")})
		{
			TestTrue(*FString::Printf(TEXT("%s is open to the sky"), Court), Description.Openings.ContainsByPredicate([Court](const FFormOpening& O)
			{
				return O.Key == FMuseeKey(FString::Printf(TEXT("chenghuai.%s.top"), Court)) && O.Kind == EFormOpeningKind::Open;
			}));
		}
		// Every surface a piece names is in the stage's finish schedule.
		const FString Finishes = Schedule(TEXT("Finishes.json"));
		TSet<FName> Checked;
		for (const FFormDrawing& Piece : Description.Pieces)
		{
			TArray<FName> Surfaces;
			Piece.PartSurfaces.GenerateValueArray(Surfaces);
			Surfaces.Add(Piece.Surface);
			for (const FName Surface : Surfaces)
			{
				bool bSeen = false;
				Checked.Add(Surface, &bSeen);
				if (!bSeen)
				{
					TestTrue(*FString::Printf(TEXT("%s's surface %s is scheduled"), *Piece.Name.ToString(), *Surface.ToString()),
						Finishes.Contains(FString::Printf(TEXT("\"surface\": \"%s\""), *Surface.ToString())));
				}
			}
		}
		// The description is a plain function of the plan: the same on every call.
		const FSiteDescription Again = Described();
		bool bSame = Again.Pieces.Num() == Description.Pieces.Num();
		for (int32 Index = 0; bSame && Index < Again.Pieces.Num(); ++Index)
		{
			bSame = Again.Pieces[Index].Name == Description.Pieces[Index].Name
				&& Again.Pieces[Index].Frame.OriginMeters == Description.Pieces[Index].Frame.OriginMeters;
		}
		TestTrue(TEXT("the same pieces in the same order and places"), bSame);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChenghuaiPlanTest, "Musee.Chenghuai.Plan", Flags)
	bool FChenghuaiPlanTest::RunTest(const FString& Parameters)
	{
		// The section rule against the sheet's derived heights (§3.1): Zc, Pr, the seats, the ridge's bed and top.
		struct FExpected
		{
			FHallSpec Spec;
			double Zc, Pr;
			TArray<double> Seats;
			double RidgeBed, RidgeTop;
		};
		const TArray<FExpected> Table = {
			{Plan::FrontRow(), 2.800, 0.117, {3.154, 3.785, 4.694, 3.785, 3.154}, 4.949, 5.409},
			{Plan::GateHall(), 3.850, 0.135, {4.240, 4.853, 5.734, 4.853, 4.240}, 5.989, 6.449},
			{Plan::SideHall(), 3.350, 0.122, {3.713, 4.263, 5.138, 5.138, 4.263, 3.858}, 5.453, 5.528},
			{Plan::MainHall(), 4.650, 0.144, {5.058, 5.708, 6.601, 7.748, 6.623, 5.741, 5.096}, 8.003, 8.463},
			{Plan::EarRoom(), 3.900, 0.117, {4.254, 4.848, 5.703, 4.848, 4.254}, 5.958, 6.418},
			{Plan::RearRow(), 2.800, 0.117, {3.154, 3.773, 4.664, 3.773, 3.154}, 4.919, 5.379},
			{Plan::GalleryG1(), 3.050, 0.099, {3.368, 3.616, 3.616, 3.368}, 3.931, 4.006},
			{Plan::ChuihuaRoof(true), 3.070, 0.108, {3.406, 3.846, 3.406}, 4.101, 4.561},
			{Plan::ChuihuaRoof(false), 3.070, 0.108, {3.406, 3.679, 3.679, 3.337}, 3.994, 4.069},
			{Plan::CoveredWalk(), 2.450, 0.090, {2.750, 3.313}, 0.0, 0.0},
			{Plan::FlowerHall(), 3.450, 0.108, {3.786, 4.361, 4.717, 4.717, 4.361, 3.786}, 5.032, 5.107},
			{Plan::WaterPavilion(), 2.950, 0.090, {3.250, 3.828, 3.828, 3.250}, 4.143, 4.218},
		};
		constexpr double Rounding = 0.0015;
		const double QingshuiTop = Plan::QingshuiRidge[10].B;
		const double RolledTop = Plan::RolledRidge[4].B;
		for (const FExpected& Row : Table)
		{
			const FHallSection S = FHallSection::Of(Row.Spec);
			const FString Name = Row.Spec.Name;
			TestEqual(*FString::Printf(TEXT("%s: Zc"), *Name), S.Zc, Row.Zc, Rounding);
			TestEqual(*FString::Printf(TEXT("%s: Pr"), *Name), S.PurlinR, Row.Pr, Rounding);
			if (TestEqual(*FString::Printf(TEXT("%s: its purlins"), *Name), S.Seat.Num(), Row.Seats.Num()))
			{
				for (int32 Index = 0; Index < Row.Seats.Num(); ++Index)
				{
					TestEqual(*FString::Printf(TEXT("%s: seat %d"), *Name, Index), S.Seat[Index], Row.Seats[Index], Rounding);
				}
			}
			if (Row.Spec.Ridge != ERidge::None)
			{
				const double Bed = S.Bed(S.RidgeV);
				TestEqual(*FString::Printf(TEXT("%s: the ridge's bed"), *Name), Bed, Row.RidgeBed, Rounding);
				TestEqual(*FString::Printf(TEXT("%s: the ridge's top"), *Name), Bed + (Row.Spec.Ridge == ERidge::Qingshui ? QingshuiTop : RolledTop),
					Row.RidgeTop, Rounding);
			}
		}

		// The footprint (§1): the porch's end on the drum's arc, through the port, from the porch walls' outer faces.
		const FSiteDescription Description = Described();
		const TArray<FVector2D>& Outline = Description.Footprint.OutlineMeters;
		TestTrue(TEXT("the footprint starts at the site's south-west corner"), Outline[0].Equals(FVector2D(-20.7, 0.0), 1e-9));
		TestTrue(TEXT("the drum's arc starts at the west porch wall's outer corner"), Outline[2].Equals(FVector2D(-2.95, 6.195), 0.001));
		TestTrue(TEXT("and passes through the port"), Outline.ContainsByPredicate([](const FVector2D& P) { return P.Equals(FVector2D(0.0, 5.8), 1e-9); }));
		TestEqual(TEXT("the footprint's top is the main hall's ridge"), Description.Footprint.TopMeters, 8.463);

		// The pond (§3.9): area 106.6 m², perimeter 41.7.
		double Twice = 0.0;
		double Perimeter = 0.0;
		const int32 N = UE_ARRAY_COUNT(Plan::PondEdge);
		for (int32 Index = 0; Index < N; ++Index)
		{
			const FVector2D A(Plan::PondEdge[Index].X, Plan::PondEdge[Index].Y);
			const FVector2D B(Plan::PondEdge[(Index + 1) % N].X, Plan::PondEdge[(Index + 1) % N].Y);
			Twice += FVector2D::CrossProduct(A, B);
			Perimeter += FVector2D::Distance(A, B);
		}
		TestEqual(TEXT("the pond's area"), 0.5 * FMath::Abs(Twice), 106.6, 0.05);
		TestEqual(TEXT("the pond's perimeter"), Perimeter, 41.7, 0.05);

		// The Classical Hall beneath: the pond's bed and the bridge's piers keep above -0.50 within r 8.30 of the tribune.
		const FVector2D Tribune(Plan::TribuneX, Plan::TribuneY);
		for (const FFormDrawing& Piece : Description.Pieces)
		{
			const FString Name = Piece.Name.ToString();
			const FVector At = Piece.Frame.OriginMeters;
			if (Name.Contains(TEXT(".pier.")) && FVector2D::Distance(FVector2D(At), Tribune) < Plan::TribuneClear)
			{
				TestTrue(*FString::Printf(TEXT("%s stops over the tribune"), *Name), At.Z >= Plan::HallClearZ - 1e-9);
			}
			if (Name == TEXT("pond.bed") && Piece.Shape.IsType<FFormLathe>())
			{
				const FFormProfile& Section = Piece.Shape.Get<FFormLathe>().Section;
				auto Point = [&Section](int32 Index) { return Index == 0 ? Section.StartPoint() : FVector2D(Section.Steps[Index - 1].Line[0], Section.Steps[Index - 1].Line[1]); };
				for (int32 Index = 0; Index <= Section.Steps.Num(); ++Index)
				{
					const FVector2D P = Point(Index);
					if (P.X < Plan::TribuneClear - 1e-9)
					{
						TestTrue(TEXT("the bed over the tribune keeps above -0.50"), P.Y >= Plan::HallClearZ - 1e-9);
					}
				}
			}
		}

		// The flights (§3.2) by Blondel: two risers and a going make a pace of 0.59 … 0.65.
		for (const Plan::FFlightPlan& Flight : Plan::Flights)
		{
			const double Riser = (Flight.Top - Flight.Ground) / Flight.Risers;
			TestTrue(*FString::Printf(TEXT("flight %s keeps Blondel's pace"), Flight.Local), FMath::IsWithinInclusive(2.0 * Riser + Flight.Going, 0.59, 0.65));
		}
		return true;
	}
}

#endif
