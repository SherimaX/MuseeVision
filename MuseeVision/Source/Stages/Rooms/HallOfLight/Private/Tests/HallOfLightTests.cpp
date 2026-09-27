#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "HallOfLightPlan.h"
#include "HallOfLightRoom.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "MuseeRecords.h"
#include "SiteDescription.h"

/**
 * The hall's description against its sheet and its schedules: its ports on the neighbours' cylinders, its anchors
 * against every anchor the schedules name, the stage contract's own check, the sheet's measured numbers, and a
 * description that is the same on every call. Its shell is proved sealed by Studio's Musee.Studio.RoomsSealed, which
 * makes its drawings.
 */
namespace Musee::HallOfLight::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		FSiteDescription Described()
		{
			FSiteDescription Description;
			GetDefault<UHallOfLightRoom>()->Describe(Description);
			return Description;
		}

		const FMuseeStageAnchor* FindAnchor(const FSiteDescription& Description, const FString& Key, FName Kind)
		{
			return Description.Anchors.FindByPredicate([&](const FMuseeStageAnchor& A) { return A.Key == FMuseeKey(Key) && A.Kind == Kind; });
		}

		const FSitePort* FindPort(const FSiteDescription& Description, const TCHAR* Key)
		{
			return Description.Ports.FindByPredicate([Key](const FSitePort& P) { return P.Key == FMuseeKey(Key); });
		}

		const FFormDrawing* FindPiece(const FSiteDescription& Description, const TCHAR* Name)
		{
			return Description.Pieces.FindByPredicate([Name](const FFormDrawing& D) { return D.Name == FName(Name); });
		}

		/** A schedule's text, as the systems that own it keep it. */
		bool ReadSchedule(const TCHAR* File, FString& OutText)
		{
			const FString Path = Musee::Records::DataRoot() / FString::Printf(TEXT("Stages/%s/%s"), Plan::Stage, File);
			return FFileHelper::LoadFileToString(OutText, *Path);
		}

		/** Every value a schedule gives a field of this name, wherever it appears. */
		TArray<FString> ValuesOf(const FString& Text, const TCHAR* Field)
		{
			const FRegexPattern Pattern(FString::Printf(TEXT("\"%s\"\\s*:\\s*\"([^\"]+)\""), Field));
			FRegexMatcher Matcher(Pattern, Text);
			TArray<FString> Values;
			while (Matcher.FindNext())
			{
				Values.Add(Matcher.GetCaptureGroup(1));
			}
			return Values;
		}

		int32 CountPieces(const FSiteDescription& Description, const FString& Prefix, const FString& Suffix = FString())
		{
			return Description.Pieces.FilterByPredicate([&](const FFormDrawing& D)
			{
				const FString Name = D.Name.ToString();
				return Name.StartsWith(Prefix) && (Suffix.IsEmpty() || Name.EndsWith(Suffix));
			}).Num();
		}

		/** The least distance from a point to a closed outline. */
		double DistanceToOutline(const TArray<FVector2D>& Outline, const FVector2D& Point)
		{
			double Least = TNumericLimits<double>::Max();
			for (int32 Index = 0; Index < Outline.Num(); ++Index)
			{
				const FVector2D A = Outline[Index];
				const FVector2D B = Outline[(Index + 1) % Outline.Num()];
				const double Along = FMath::Clamp(FVector2D::DotProduct(Point - A, B - A) / FMath::Max(UE_DOUBLE_SMALL_NUMBER, (B - A).SizeSquared()), 0.0, 1.0);
				Least = FMath::Min(Least, FVector2D::Distance(Point, A + Along * (B - A)));
			}
			return Least;
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHallOfLightPortsTest, "Musee.HallOfLight.Ports", Flags)
	bool FHallOfLightPortsTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		TestEqual(TEXT("two ports"), Description.Ports.Num(), 2);
		const FSitePort* West = FindPort(Description, TEXT("hall-of-light.west"));
		const FSitePort* East = FindPort(Description, TEXT("hall-of-light.east"));
		if (!TestNotNull(TEXT("the west door"), West) || !TestNotNull(TEXT("the east door"), East))
		{
			return false;
		}
		const FVector Museum(Plan::MuseumX, 0.0, 0.0);
		TestTrue(TEXT("the west door stands on the Rotunda's east port, museum (11.200, 0, 0)"),
			(West->Frame.OriginMeters + Museum).Equals(FVector(11.2, 0.0, 0.0), 1e-9));
		TestTrue(TEXT("and faces west, into the Rotunda"), West->Facing().Equals(FVector(-1.0, 0.0, 0.0), 1e-9));
		TestTrue(TEXT("with z up"), West->Frame.DirectionToParent(FVector::UpVector).Equals(FVector::UpVector, 1e-9));
		TestEqual(TEXT("it meets rotunda.east"), West->Mates, FMuseeKey(TEXT("rotunda.east")));
		TestEqual(TEXT("its class is the Rotunda's door"), West->Class, FMuseeKey(TEXT("doorway.3000x6000.arched")));
		TestTrue(TEXT("the east door stands at museum (39.200, 0, 0), 14.800 west of the Atrium's axis"),
			(East->Frame.OriginMeters + Museum).Equals(FVector(54.0 - 14.8, 0.0, 0.0), 1e-9));
		TestTrue(TEXT("and faces east, into the Atrium"), East->Facing().Equals(FVector(1.0, 0.0, 0.0), 1e-9));
		TestEqual(TEXT("it meets atrium.west"), East->Mates, FMuseeKey(TEXT("atrium.west")));
		TestEqual(TEXT("its class is the Atrium's west door"), East->Class, FMuseeKey(TEXT("doorway.4000x4500.square")));

		// Both faces are the neighbours' cylinders, and each port's wall is its 50 mm lining.
		TestEqual(TEXT("the west face is the Rotunda's drum, R 11.20 about its axis"),
			FVector2D::Distance(FVector2D(West->Frame.OriginMeters), FVector2D(-11.2, 0.0)), West->FaceRadiusMeters, 1e-9);
		TestEqual(TEXT("the east face is the Atrium's base, R 14.80 about its axis"),
			FVector2D::Distance(FVector2D(East->Frame.OriginMeters), FVector2D(42.8, 0.0)), East->FaceRadiusMeters, 1e-9);
		TestEqual(TEXT("the west wall is the lining"), West->WallDepthMeters, 0.050);
		TestEqual(TEXT("the east wall is the lining"), East->WallDepthMeters, 0.050);

		// The rough openings through the linings: the classes' clear openings with Site's 0.050 round them.
		for (const TPair<const TCHAR*, FVector>& Rough : {TPair<const TCHAR*, FVector>(TEXT("lining.west"), FVector(3.10, 4.50, 1.0)),
			TPair<const TCHAR*, FVector>(TEXT("lining.east"), FVector(4.10, 4.55, 0.0))})
		{
			const FFormDrawing* Lining = FindPiece(Description, Rough.Key);
			const FFormWall* Wall = Lining ? Lining->Shape.TryGet<FFormWall>() : nullptr;
			const bool bEast = Rough.Key == FString(TEXT("lining.east"));
			if (!TestTrue(*FString::Printf(TEXT("%s is a wall with its port's rough opening"), Rough.Key), Wall && Wall->Openings.Num() == (bEast ? 2 : 1)))
			{
				continue;
			}
			const FFormWallOpening& Opening = Wall->Openings[0];
			TestTrue(TEXT("its rough opening runs straight through, jambs parallel"), Opening.bStraight);
			TestEqual(TEXT("its jambs stand 1/2 the rough width off the axis"), 0.5 * Opening.WidthMeters, 0.5 * Rough.Value.X, 1e-9);
			TestTrue(TEXT("its top follows the vault's arc"), Wall->HasDrawnTop());
			if (bEast)
			{
				// Sec. 3.1: the lunette above the door, the arc R 5.19 about the vault's axis over its sill at 6.20.
				const FFormWallOpening& Lunette = Wall->Openings[1];
				TestEqual(TEXT("the lunette's sill"), Wall->BaseMeters + Lunette.SillMeters, 6.2, 1e-9);
				TestEqual(TEXT("its arc's crown"), Wall->BaseMeters + Lunette.CrownMeters(), 2.2 + 5.19, 1e-9);
			}
			TestEqual(TEXT("its head springs from the sheet's height"), Wall->BaseMeters + Opening.HeadMeters, Rough.Value.Y, 1e-9);
			TestEqual(TEXT("its sill is 0.050 under the threshold"), Wall->BaseMeters + Opening.SillMeters, -0.050, 1e-9);
			TestTrue(TEXT("its head is arched as its class"), Opening.bArched == (Rough.Value.Z > 0.5));
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHallOfLightAnchorsTest, "Musee.HallOfLight.Anchors", Flags)
	bool FHallOfLightAnchorsTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		FString Hang;
		FString Planting;
		if (!TestTrue(TEXT("the hanging and planting schedules read"), ReadSchedule(TEXT("Hang.json"), Hang) && ReadSchedule(TEXT("Planting.json"), Planting)))
		{
			return false;
		}
		const TArray<FString> Hung = ValuesOf(Hang, TEXT("anchor"));
		TestEqual(TEXT("24 works are hung"), Hung.Num(), 24);
		for (const FString& Anchor : Hung)
		{
			TestTrue(*FString::Printf(TEXT("%s is a Wall or a reader's Mount"), *Anchor),
				FindAnchor(Description, Anchor, TEXT("Wall")) != nullptr || FindAnchor(Description, Anchor, TEXT("Mount")) != nullptr);
		}
		for (const FString& Bed : ValuesOf(Planting, TEXT("bed")))
		{
			TestNotNull(*FString::Printf(TEXT("%s is a Bed anchor"), *Bed), FindAnchor(Description, Bed, TEXT("Bed")));
		}

		// The hanging walls: origin at the viewer's left on the hanging plane, x to the viewer's right, y towards the viewer.
		const TPair<const TCHAR*, FVector> Walls[] = {{TEXT("hall-of-light.prints.north"), FVector(0.0, -4.35, 0.0)},
			{TEXT("hall-of-light.prints.south"), FVector(28.0, 4.35, 0.0)}, {TEXT("hall-of-light.autochromes.north"), FVector(0.0, -4.44, 0.0)},
			{TEXT("hall-of-light.autochromes.south"), FVector(28.0, 4.44, 0.0)}};
		for (const TPair<const TCHAR*, FVector>& Wall : Walls)
		{
			const FMuseeStageAnchor* Anchor = FindAnchor(Description, Wall.Key, TEXT("Wall"));
			if (!TestNotNull(Wall.Key, Anchor))
			{
				continue;
			}
			const double Side = Wall.Value.Y < 0.0 ? -1.0 : 1.0;
			TestTrue(*FString::Printf(TEXT("%s stands on its plane"), Wall.Key), Anchor->Frame.OriginMeters.Equals(Wall.Value, 1e-9));
			TestTrue(TEXT("its x runs west to east on the north wall, east to west on the south"),
				Anchor->Frame.DirectionToParent(FVector::ForwardVector).Equals(FVector(-Side, 0.0, 0.0), 1e-9));
			TestTrue(TEXT("its y looks into the hall"), Anchor->Frame.DirectionToParent(FVector::RightVector).Equals(FVector(0.0, -Side, 0.0), 1e-9));
			TestTrue(TEXT("its z is up"), Anchor->Frame.DirectionToParent(FVector::UpVector).Equals(FVector::UpVector, 1e-9));
		}

		// The stereo readers: over their stones at 1.320, facing the axis, tilted back 28.65 deg; the anchor on the plate's face.
		for (int32 Index = 0; Index < Plan::StereoStones; ++Index)
		{
			const FMuseeStageAnchor* Reader = FindAnchor(Description, FString::Printf(TEXT("hall-of-light.stereo.%d"), Index + 1), TEXT("Mount"));
			if (!TestNotNull(TEXT("a reader's Mount"), Reader))
			{
				continue;
			}
			const double Y = Index % 2 == 0 ? -2.4 : 2.4;
			const FVector Normal = Reader->Frame.DirectionToParent(FVector::RightVector);
			TestTrue(TEXT("it is centred over its stone, on the plate's face"),
				Reader->Frame.OriginMeters.Equals(FVector(5.8 + 6.0 * Index, Y, 1.32) + Normal * (0.5 * Plan::ReaderThickness), 1e-9));
			TestTrue(TEXT("it faces the axis, tilted up 28.65 deg"), Normal.Equals(FVector(0.0, -FMath::Sign(Y) * 0.8776, 0.4794), 1e-4));
			TestTrue(TEXT("its x is the viewer's right"), Reader->Frame.DirectionToParent(FVector::ForwardVector).Equals(FVector(-FMath::Sign(Y), 0.0, 0.0), 1e-9));
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHallOfLightDescribedTest, "Musee.HallOfLight.Described", Flags)
	bool FHallOfLightDescribedTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		TestEqual(TEXT("it is the hall-of-light stage"), Description.Stage, FMuseeKey(TEXT("hall-of-light")));
		const TArray<FMuseeFinding> Findings = Musee::Site::CheckDescription(Description);
		for (const FMuseeFinding& Finding : Findings)
		{
			AddError(Finding.Summary);
		}
		TestEqual(TEXT("the stage contract holds"), Findings.Num(), 0);
		TestEqual(TEXT("the hall and its two gardens"), Description.Enclosures.Num(), 3);
		const FFormEnclosure* Hall = Description.Enclosures.FindByPredicate([](const FFormEnclosure& E) { return E.Key == FMuseeKey(TEXT("hall-of-light.hall")); });
		if (TestNotNull(TEXT("the hall"), Hall))
		{
			TestTrue(TEXT("which is enclosed"), Hall->Kind == EFormEnclosureKind::Enclosed);
			for (const TCHAR* Bound : {TEXT("floor"), TEXT("lining.west"), TEXT("lining.east"), TEXT("eaves.north"), TEXT("vault.glass.0.0"),
				TEXT("wall.glass.south.10"), TEXT("lunette.glass.1")})
			{
				TestTrue(*FString::Printf(TEXT("the hall is bounded by %s"), Bound), Hall->Pieces.Contains(FName(Bound)));
			}
		}

		// The openings: the two ports, the four glazings with their surfaces, the twenty housings.
		auto Count = [&Description](EFormOpeningKind Kind) { return Description.Openings.FilterByPredicate([Kind](const FFormOpening& O) { return O.Kind == Kind; }).Num(); };
		TestEqual(TEXT("two port openings"), Count(EFormOpeningKind::Port), 2);
		TestEqual(TEXT("twenty housings"), Count(EFormOpeningKind::Housing), 20);
		const TPair<const TCHAR*, const TCHAR*> Glazings[] = {{TEXT("hall-of-light.wall.north"), TEXT("hall-of-light.wall.glass")},
			{TEXT("hall-of-light.wall.south"), TEXT("hall-of-light.wall.glass")}, {TEXT("hall-of-light.vault"), TEXT("hall-of-light.vault.glass")},
			{TEXT("hall-of-light.lunette"), TEXT("hall-of-light.lunette.glass")}};
		TestEqual(TEXT("four glazed openings"), Count(EFormOpeningKind::Glazed), 4);
		for (const TPair<const TCHAR*, const TCHAR*>& Glazing : Glazings)
		{
			TestTrue(*FString::Printf(TEXT("%s is glazed in %s"), Glazing.Key, Glazing.Value), Description.Openings.ContainsByPredicate([&Glazing](const FFormOpening& O)
			{
				return O.Key == FMuseeKey(Glazing.Key) && O.Kind == EFormOpeningKind::Glazed && O.Glazing == FName(Glazing.Value);
			}));
		}
		TestTrue(TEXT("the footprint is a closed outline"), Description.Footprint.OutlineMeters.Num() > 18);

		// Every surface a piece names is in the stage's finish schedule.
		FString Finishes;
		if (TestTrue(TEXT("the finish schedule reads"), ReadSchedule(TEXT("Finishes.json"), Finishes)))
		{
			const TArray<FString> Scheduled = ValuesOf(Finishes, TEXT("surface"));
			for (const FFormDrawing& Piece : Description.Pieces)
			{
				TArray<FName> Surfaces;
				Piece.PartSurfaces.GenerateValueArray(Surfaces);
				Surfaces.Add(Piece.Surface);
				for (const FName Surface : Surfaces)
				{
					TestTrue(*FString::Printf(TEXT("%s's surface %s is scheduled"), *Piece.Name.ToString(), *Surface.ToString()), Scheduled.Contains(Surface.ToString()));
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

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHallOfLightPlanTest, "Musee.HallOfLight.Plan", Flags)
	bool FHallOfLightPlanTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();

		// The footprint passes the sheet's vertices (to its millimetres) and both port faces on the axis.
		const TArray<FVector2D>& Outline = Description.Footprint.OutlineMeters;
		TestTrue(TEXT("the footprint starts at F1"), Outline.Num() > 0 && Outline[0].Equals(FVector2D(-0.3, -14.25), 1e-9));
		const FVector2D Vertices[] = {{29.9, -8.227}, {28.747, -6.050}, {27.8, -6.05}, {28.8, -4.8}, {28.8, 4.8}, {28.747, 6.050}, {29.9, 8.227},
			{-1.081, 4.8}, {-1.081, -4.8}, {-0.3, -4.8}};
		for (const FVector2D& Vertex : Vertices)
		{
			TestTrue(*FString::Printf(TEXT("the footprint passes (%.3f, %.3f)"), Vertex.X, Vertex.Y), DistanceToOutline(Outline, Vertex) < 0.0006);
		}
		TestTrue(TEXT("the east port face crosses the axis at x 28.000"), DistanceToOutline(Outline, FVector2D(28.0, 0.0)) <= Plan::ChordToleranceMeters);
		TestTrue(TEXT("the west port face crosses the axis at x 0.000"), DistanceToOutline(Outline, FVector2D(0.0, 0.0)) <= Plan::ChordToleranceMeters);

		// The vault's datums (sec. 1): crown 7.50; the springing on the glass line at 58.11 deg; the eaves at 56.83 deg;
		// the purlins at 14.73, 29.46 and 44.19 deg.
		TestEqual(TEXT("the crown"), Plan::VaultCrown, 7.5, 1e-9);
		TestEqual(TEXT("the springing on the glass line"), FMath::RadiansToDegrees(FMath::Asin(Plan::GlassLine / Plan::VaultRadius)), 58.11, 0.005);
		TestEqual(TEXT("the eaves"), FMath::RadiansToDegrees(FMath::Acos((Plan::EavesTop - Plan::VaultAxisZ) / Plan::VaultRadius)), 56.83, 0.005);
		const double Coping = FMath::RadiansToDegrees(FMath::Acos((Plan::CopingLineZ - Plan::VaultAxisZ) / Plan::VaultRadius));
		TestEqual(TEXT("the outer purlins, on the Atrium's coping"), Coping, 44.19, 0.005);
		TestEqual(TEXT("the first purlins"), Coping / Plan::PurlinThirds, 14.73, 0.005);
		TestEqual(TEXT("the linings' crown"), Plan::LiningCrown, 7.62, 1e-9);
		TestEqual(TEXT("the lunette's sill clears the Atrium's coping (6.05-6.20)"), Plan::LunetteSill, 6.2, 1e-9);
		TestTrue(TEXT("the west collar's underside clears the joint's surround, z >= 7.05 within |y| 1.85"),
			Plan::VaultAxisZ + FMath::Sqrt(FMath::Square(Plan::LunetteRadius) - FMath::Square(1.85)) >= 7.049);

		// The pieces by count (sec. 3).
		TestEqual(TEXT("22 wall panes"), CountPieces(Description, TEXT("wall.glass.")), 22);
		TestEqual(TEXT("152 vault panes, 8 across and 19 along"), CountPieces(Description, TEXT("vault.glass.")), 152);
		TestEqual(TEXT("6 lunette panes"), CountPieces(Description, TEXT("lunette.glass.")), 6);
		TestEqual(TEXT("5 lunette mullions"), CountPieces(Description, TEXT("lunette.mullion.")), 5);
		TestEqual(TEXT("20 posts"), CountPieces(Description, TEXT("post.")), 20);
		TestEqual(TEXT("4 end posts"), CountPieces(Description, TEXT("end-post.")), 4);
		TestEqual(TEXT("18 arches, three plates each"), CountPieces(Description, TEXT("arch."), TEXT(".web")), 18);
		TestEqual(TEXT("7 purlin lines in 19 lengths"), CountPieces(Description, TEXT("purlin.")), 7 * 19);
		TestEqual(TEXT("136 crosses"), CountPieces(Description, TEXT("cross."), TEXT(".a")), 136);
		TestEqual(TEXT("2 collars, each one flat bent round its lining"), CountPieces(Description, TEXT("collar.")), 2);
		TestEqual(TEXT("32 print stems"), CountPieces(Description, TEXT("print.stem."), TEXT("west")) + CountPieces(Description, TEXT("print.stem."), TEXT("east")), 32);
		TestEqual(TEXT("8 autochrome bars"), CountPieces(Description, TEXT("autochrome.bar.")), 8);
		TestEqual(TEXT("5 viewing stones"), CountPieces(Description, TEXT("viewing-stone."), TEXT(".ring")), 5);
		TestEqual(TEXT("4 stereo stones"), CountPieces(Description, TEXT("stereo-stone.")), 4);
		TestEqual(TEXT("4 stereo readers"), CountPieces(Description, TEXT("stereo.reader."), TEXT(".stem")), 4);
		return true;
	}
}

#endif
