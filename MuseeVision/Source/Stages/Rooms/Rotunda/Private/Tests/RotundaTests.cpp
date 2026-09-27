#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "MotionSchedule.h"
#include "MuseeRecords.h"
#include "RotundaParts.h"
#include "RotundaPlan.h"
#include "RotundaStairLine.h"
#include "RotundaRoom.h"
#include "SiteDescription.h"
#include "SitePortClass.h"
#include "TimeMuseumDial.h"

/**
 * The Rotunda's description against its sheet and its schedules: its ports and their classes, its anchors against every
 * anchor the schedules name, the stage contract's own check, its pieces' surfaces against its finish schedule and its
 * moving body, and a description that is the same on every call. Its shells are proved sealed by Studio's
 * Musee.Studio.RoomsSealed, which makes its drawings.
 */
namespace Musee::Rotunda::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		FSiteDescription Described()
		{
			FSiteDescription Description;
			GetDefault<URotundaRoom>()->Describe(Description);
			return Description;
		}

		const FMuseeStageAnchor* FindAnchor(const FSiteDescription& Description, const FMuseeKey& Key, FName Kind)
		{
			return Description.Anchors.FindByPredicate([&](const FMuseeStageAnchor& A) { return A.Key == Key && A.Kind == Kind; });
		}

		const FSitePort* FindPort(const FSiteDescription& Description, const FString& Key)
		{
			return Description.Ports.FindByPredicate([&Key](const FSitePort& P) { return P.Key == FMuseeKey(Key); });
		}

		const FFormDrawing* FindPiece(const FSiteDescription& Description, const TCHAR* Name)
		{
			return Description.Pieces.FindByPredicate([Name](const FFormDrawing& D) { return D.Name == FName(Name); });
		}

		FString Schedule(const TCHAR* File)
		{
			return FString::Printf(TEXT("Stages/%s/%s"), Plan::Stage, File);
		}

		/** A schedule's text, read as the systems read it from Data. */
		bool ReadSchedule(const TCHAR* File, FString& OutText)
		{
			return FFileHelper::LoadFileToString(OutText, *(Musee::Records::DataRoot() / Schedule(File)));
		}

		/** Every value a schedule gives a field, in the order written: "field": "value". */
		TArray<FString> ValuesOf(const FString& Text, const TCHAR* Field)
		{
			const FRegexPattern Pattern(FString::Printf(TEXT("\"%s\"\\s*:\\s*\"([^\"]+)\""), Field));
			FRegexMatcher Matcher(Pattern, Text);
			TArray<FString> Out;
			while (Matcher.FindNext())
			{
				Out.Add(Matcher.GetCaptureGroup(1));
			}
			return Out;
		}

		FVector2D Polar(double Radius, double Degrees)
		{
			const double Angle = FMath::DegreesToRadians(Degrees);
			return FVector2D(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle));
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRotundaPortsTest, "Musee.Rotunda.Ports", Flags)
	bool FRotundaPortsTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		TestEqual(TEXT("five ports"), Description.Ports.Num(), 5);
		for (const Plan::FDoor& Door : Plan::Doors)
		{
			const FSitePort* Port = FindPort(Description, FString::Printf(TEXT("rotunda.%s"), Door.Name));
			if (!TestNotNull(*FString::Printf(TEXT("the %s door"), Door.Name), Port))
			{
				continue;
			}
			const FVector2D Face = Polar(11.2, Door.Deg);
			TestTrue(*FString::Printf(TEXT("the %s door stands on the drum's outer face at the threshold"), Door.Name),
				Port->Frame.OriginMeters.Equals(FVector(Face.X, Face.Y, 0.0), 1e-9));
			TestTrue(TEXT("and faces out along its axis"), Port->Facing().Equals(FVector(Face.X, Face.Y, 0.0) / 11.2, 1e-9));
			TestEqual(TEXT("its face is the drum's cylinder"), Port->FaceRadiusMeters, 11.2);
			TestEqual(TEXT("through the drum's 1.20"), Port->WallDepthMeters, 1.2, 1e-12);
			TestEqual(TEXT("it meets the neighbour the sheet names"), Port->Mates, FMuseeKey(Door.Mates));
			TestEqual(TEXT("its class is width by crown"), Port->Class, FMuseeKey(Door.Class));
		}
		const FSitePort* Stair = FindPort(Description, TEXT("rotunda.stair"));
		if (!TestNotNull(TEXT("the stair's port"), Stair))
		{
			return false;
		}
		TestTrue(TEXT("the stair's port stands on r 7.60 at the landing, due north"), Stair->Frame.OriginMeters.Equals(FVector(0.0, -7.6, -6.0), 1e-9));
		TestTrue(TEXT("and faces north"), Stair->Facing().Equals(FVector(0.0, -1.0, 0.0), 1e-9));
		TestEqual(TEXT("it meets the Classical Hall's stair"), Stair->Mates, FMuseeKey(TEXT("classical-hall.stair")));
		TestTrue(TEXT("the Rotunda serves the well"), Stair->bHasWell && Stair->Well.bServing);
		TestTrue(TEXT("the well is the prism r 7.60, z -6.40 to 0"), Stair->Well.MinMeters.Equals(FVector(-7.6, -7.6, -6.4), 1e-9)
			&& Stair->Well.MaxMeters.Equals(FVector(7.6, 7.6, 0.0), 1e-9));

		// §2: "at the port face r 7.60 the class opening". The Rotunda finishes its side of the stair port itself, so the joint
		// lines none of it: the reveal r 7.05 to 7.60 and the portal through the pocket's masonry and the sleeve are one
		// straight passage at the class's clear opening, its jambs the planes x = +/-1.80, the reveal arched r 1.80 at the
		// class's springing; a cut wider than that, or radial, leaves a slot beside the joint's jambs in the Classical Hall.
		TestEqual(TEXT("the Rotunda finishes the stair port's whole wall depth itself"), Stair->OwnLiningMeters, Stair->WallDepthMeters);
		TestTrue(TEXT("and lays its landing as the port's threshold"), Stair->bOwnThreshold);
		const double LandingBed = Plan::LandingZ - Plan::FinishDepth;
		for (const TCHAR* Layer : {Pieces::Sleeve, Pieces::MasonryOuter})
		{
			const FFormDrawing* Drawing = FindPiece(Description, Layer);
			const FFormWall* Wall = Drawing ? Drawing->Shape.TryGet<FFormWall>() : nullptr;
			if (!TestTrue(*FString::Printf(TEXT("%s is a wall with the portal cut through it"), Layer), Wall && Wall->Openings.Num() == 1))
			{
				continue;
			}
			const FFormWallOpening& Portal = Wall->Openings[0];
			TestTrue(*FString::Printf(TEXT("%s's portal is a straight passage"), Layer), Portal.bStraight);
			TestEqual(*FString::Printf(TEXT("%s's portal is the class's clear width"), Layer), Portal.WidthMeters, Plan::StairPortClear, 1e-12);
			const bool bReveal = FCString::Strcmp(Layer, Pieces::MasonryOuter) == 0;
			TestEqual(*FString::Printf(TEXT("%s's portal is arched as the sheet has it"), Layer), Portal.bArched, bReveal);
			if (bReveal)
			{
				TestEqual(TEXT("the reveal springs where the class does"), Wall->BaseMeters + Portal.HeadMeters, Plan::LandingZ + Plan::StairPortSpringing, 1e-9);
				TestEqual(TEXT("in a semicircle of the class's half-width"), Portal.IntradosRadiusMeters(), 0.5 * Plan::StairPortClear, 1e-12);
				TestEqual(TEXT("from the landing's bed"), Wall->BaseMeters, LandingBed, 1e-12);
			}
		}

		// The rough openings the room builds are its ports' classes, where the site's records carry them.
		TArray<FSitePort> Declared = Description.Ports;
		for (const FSitePort& Port : Declared)
		{
			const TValueOrError<FSitePortClass, FString> Class = Musee::Site::FindPortClass(Port.Class);
			if (Class.HasError())
			{
				AddWarning(FString::Printf(TEXT("%s's class %s is not in Site's PortClasses.json yet: %s"), *Port.Key.ToString(),
					*Port.Class.ToString(), *Class.GetError()));
				continue;
			}
			const bool bStair = Port.Kind == ESitePortKind::Stairwell;
			TestEqual(TEXT("the class springs where the sheet does"), Class.GetValue().HeadMeters, bStair ? Plan::StairPortSpringing : Plan::DoorSpringing);
			TestTrue(TEXT("and is arched"), Class.GetValue().Arched);
			if (bStair)
			{
				TestEqual(TEXT("the stair port's class is the portal's clear width"), Class.GetValue().ClearWidthMeters, 2.0 * Plan::PortalHalfWidth, 1e-12);
			}
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRotundaAnchorsTest, "Musee.Rotunda.Anchors", Flags)
	bool FRotundaAnchorsTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		const TValueOrError<FMotionSchedule, FString> Motion = Musee::ReadRecord<FMotionSchedule>(Schedule(FMotionSchedule::FileName));
		if (!TestTrue(TEXT("the mechanisms schedule reads"), Motion.HasValue()))
		{
			AddError(Motion.GetError());
			return false;
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

		// Every cast stands on a Mount anchor, and every luminaire's body and mount are anchors of their kinds.
		FString Hang;
		FString Lighting;
		if (!TestTrue(TEXT("the hanging and lighting schedules read"), ReadSchedule(TEXT("Hang.json"), Hang) && ReadSchedule(TEXT("Lighting.json"), Lighting)))
		{
			return false;
		}
		const TArray<FString> Casts = ValuesOf(Hang, TEXT("anchor"));
		TestEqual(TEXT("four casts"), Casts.Num(), 4);
		for (const FString& Cast : Casts)
		{
			TestNotNull(*FString::Printf(TEXT("%s is a Mount anchor"), *Cast), FindAnchor(Description, FMuseeKey(Cast), TEXT("Mount")));
		}
		for (const FString& Body : ValuesOf(Lighting, TEXT("body")))
		{
			TestNotNull(*FString::Printf(TEXT("%s is a Body anchor"), *Body), FindAnchor(Description, FMuseeKey(Body), TEXT("Body")));
		}
		for (const FString& Mount : ValuesOf(Lighting, TEXT("mount")))
		{
			TestNotNull(*FString::Printf(TEXT("%s is a Mount anchor"), *Mount), FindAnchor(Description, FMuseeKey(Mount), TEXT("Mount")));
		}

		// Each cast's mount is on its niche's sill at r 10.25, looking at the centre, z up (§2, §8).
		for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::NicheDegrees)); ++Index)
		{
			const FMuseeKey CastKey(FString::Printf(TEXT("rotunda.niche.%s.cast"), Plan::NicheNames[Index]));
			const FMuseeStageAnchor* Mount = FindAnchor(Description, CastKey, TEXT("Mount"));
			if (!TestNotNull(*CastKey.ToString(), Mount))
			{
				continue;
			}
			const FVector2D At = Polar(10.25, Plan::NicheDegrees[Index]);
			TestTrue(TEXT("on the sill at r 10.25"), Mount->Frame.OriginMeters.Equals(FVector(At.X, At.Y, 2.0), 1e-9));
			TestTrue(TEXT("its x towards the centre"), Mount->Frame.DirectionToParent(FVector::ForwardVector).Equals(-FVector(At.X, At.Y, 0.0) / 10.25, 1e-9));
			TestTrue(TEXT("its z up"), Mount->Frame.DirectionToParent(FVector::UpVector).Equals(FVector::UpVector, 1e-9));
		}
		const FMuseeStageAnchor* Control = FindAnchor(Description, FMuseeKey(TEXT("rotunda.sun-clock.control")), TEXT("Use"));
		TestTrue(TEXT("the sun clock is worked from the gilt sun's top"), Control && Control->Frame.OriginMeters.Equals(FVector(0.0, 0.0, 0.085), 1e-9));
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRotundaDescribedTest, "Musee.Rotunda.Described", Flags)
	bool FRotundaDescribedTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		TestEqual(TEXT("it is the rotunda stage"), Description.Stage, FMuseeKey(TEXT("rotunda")));
		const TArray<FMuseeFinding> Findings = Musee::Site::CheckDescription(Description);
		for (const FMuseeFinding& Finding : Findings)
		{
			AddError(Finding.Summary);
		}
		TestEqual(TEXT("the stage contract holds"), Findings.Num(), 0);
		TestEqual(TEXT("two enclosures"), Description.Enclosures.Num(), 2);
		const int32 ToSky = Description.Openings.FilterByPredicate([](const FFormOpening& O)
		{
			return O.Kind == EFormOpeningKind::Glazed || O.Kind == EFormOpeningKind::Open;
		}).FilterByPredicate([](const FFormOpening& O) { return O.Enclosure == FMuseeKey(TEXT("rotunda.hall")); }).Num();
		TestEqual(TEXT("the eye is the hall's one opening to the sky"), ToSky, 1);
		TestTrue(TEXT("and it is glazed with the eye's glass"), Description.Openings.ContainsByPredicate([](const FFormOpening& O)
		{
			return O.Key == FMuseeKey(TEXT("rotunda.eye")) && O.Kind == EFormOpeningKind::Glazed && O.Glazing == FName(TEXT("rotunda.eye.glass"));
		}));

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
				for (const FName Finish : Surfaces)
				{
					TestTrue(*FString::Printf(TEXT("%s's surface %s is scheduled"), *Piece.Name.ToString(), *Finish.ToString()),
						Scheduled.Contains(Finish.ToString()));
				}
			}
		}

		// The dial and the sun clock's drum ride its body, the retracting balustrade's panels theirs; nothing else moves.
		const FMuseeKey Drum(TEXT("rotunda.sun-clock.drum"));
		const FMuseeKey Panels(TEXT("rotunda.stair.balustrade.panels"));
		int32 PanelPieces = 0;
		for (const FFormDrawing& Piece : Description.Pieces)
		{
			const FString Name = Piece.Name.ToString();
			const bool bRidesDrum = Name.StartsWith(TEXT("dial.")) || Name.StartsWith(TEXT("sun-clock."));
			const bool bRidesPanels = Name.StartsWith(TEXT("stair.balustrade.panel."));
			PanelPieces += bRidesPanels ? 1 : 0;
			const FMuseeKey Expected = bRidesDrum ? Drum : (bRidesPanels ? Panels : FMuseeKey());
			TestTrue(*FString::Printf(TEXT("%s rides a body only if it is that body's"), *Name), Piece.Body == Expected);
		}
		TestTrue(TEXT("the two retracting panels are drawn"), PanelPieces > 0);

		// The sun clock's door is a passage from the stair to the hall, on the drum.
		const FFormOpening* Door = Description.Openings.FindByPredicate([](const FFormOpening& O) { return O.Key == FMuseeKey(TEXT("rotunda.sun-clock.door")); });
		if (TestNotNull(TEXT("the sun clock's door is declared"), Door))
		{
			TestTrue(TEXT("a passage from the stair into the hall, riding the drum"), Door->Kind == EFormOpeningKind::Passage
				&& Door->Enclosure == FMuseeKey(TEXT("rotunda.stair")) && Door->Beyond == FMuseeKey(TEXT("rotunda.hall")) && Door->Body == Drum);
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

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRotundaPlanTest, "Musee.Rotunda.Plan", Flags)
	bool FRotundaPlanTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();

		// §1: the footprint is the drum's outer circle on 320 stations, through every 22.5° reference point.
		const TArray<FVector2D>& Outline = Description.Footprint.OutlineMeters;
		TestEqual(TEXT("320 stations"), Outline.Num(), 320);
		bool bOnCircle = true;
		for (const FVector2D& Point : Outline)
		{
			bOnCircle &= FMath::IsNearlyEqual(Point.Size(), 11.2, 1e-9);
		}
		TestTrue(TEXT("all on r 11.20"), bOnCircle);
		TestTrue(TEXT("through (10.347, 4.286) at 22.5 degrees"), Outline.Num() > 20 && Outline[20].Equals(FVector2D(10.347, 4.286), 0.001));
		TestEqual(TEXT("down to the well's floor"), Description.Footprint.BottomMeters, -6.4);
		// Its two prisms: the well r 7.60 from -6.40 to 0.00 on the Classical Hall's 1.5° stations, and the room on the drum's
		// circle from the floor structure's underside, -0.55, to 21.00; the hall below is outside both.
		const TArray<FSitePrism>& Prisms = Description.Footprint.Prisms;
		if (TestEqual(TEXT("the well and the room"), Prisms.Num(), 2))
		{
			TestTrue(TEXT("the well from -6.40 to 0.00"), Prisms[0].BottomMeters == -6.4 && Prisms[0].TopMeters == 0.0);
			TestTrue(TEXT("on 240 stations of r 7.60"), Prisms[0].OutlineMeters.Num() == 240 && Prisms[0].OutlineMeters[15].Equals(FVector2D(7.6 * FMath::Cos(
				FMath::DegreesToRadians(22.5)), 7.6 * FMath::Sin(FMath::DegreesToRadians(22.5))), 1e-9));
			TestTrue(TEXT("the room from -0.55 to 21.00 on the drum's circle"), Prisms[1].BottomMeters == -0.55 && Prisms[1].TopMeters == 21.0
				&& Prisms[1].OutlineMeters == Outline);
		}

		// §3.4: the coffers' steps from their cells; ring 1's rib is 0.243, its bevel 0.110; ring 5's rib 0.131.
		TestEqual(TEXT("the dome's lower band springs from the cornice's top at 2.866 degrees"),
			FMath::RadiansToDegrees(FMath::Asin(0.05)), 2.866, 0.001);
		const double Cell1 = 2.0 * UE_DOUBLE_PI * 10.0 * FMath::Cos(FMath::DegreesToRadians(10.0)) / 28.0;
		const double Cell5 = 2.0 * UE_DOUBLE_PI * 10.0 * FMath::Cos(FMath::DegreesToRadians(58.0)) / 28.0;
		TestEqual(TEXT("ring 1's cell"), Cell1, 2.210, 0.001);
		TestEqual(TEXT("ring 1's rib"), Plan::RibOfCell * Cell1, 0.243, 0.001);
		TestEqual(TEXT("ring 1's bevel"), Plan::BevelOfCell * Cell1, 0.110, 0.001);
		TestEqual(TEXT("ring 5's rib"), Plan::RibOfCell * Cell5, 0.131, 0.001);

		// §3.8: the stair turns from 194° to its last riser at -50°, the mid landing 20°, forty risers of 0.150.
		const FFormDrawing* TopLanding = FindPiece(Description, TEXT("stair.tread.01"));
		const FFormDrawing* Mid = FindPiece(Description, TEXT("stair.tread.20"));
		const FFormDrawing* Last = FindPiece(Description, TEXT("stair.tread.39"));
		auto Span = [](const FFormDrawing* Tread, double& OutFrom, double& OutTo)
		{
			// A tread's plan: the annular sector whose first step is its outer arc, from its nosing to its back.
			const FFormArc& Arc = Tread->Shape.Get<FFormHeightfield>().Outline.Steps[0].Arc;
			OutFrom = Arc.FromDegrees;
			OutTo = Arc.ToDegrees;
		};
		if (TestTrue(TEXT("the top landing, the mid landing and the last tread are drawn"), TopLanding && Mid && Last)
			&& TestTrue(TEXT("each a stone dressed to the soffit"), TopLanding->Shape.IsType<FFormHeightfield>() && Mid->Shape.IsType<FFormHeightfield>()
				&& Last->Shape.IsType<FFormHeightfield>()))
		{
			double From = 0.0;
			double To = 0.0;
			Span(TopLanding, From, To);
			TestEqual(TEXT("the top landing's back edge"), To, 194.0, 1e-9);
			TestEqual(TEXT("its nosing"), From, 164.0, 1e-9);
			Span(Mid, From, To);
			TestEqual(TEXT("the mid landing turns 20 degrees"), To - From, 20.0, 1e-9);
			TestEqual(TEXT("from 69.622 degrees"), To, 69.622, 0.001);
			Span(Last, From, To);
			TestEqual(TEXT("the last riser at -50 degrees"), From, -50.0, 1e-9);
			const FFormHeightfield& Stone = Last->Shape.Get<FFormHeightfield>();
			TestTrue(TEXT("the last tread's underside runs into the landing no lower than -6.02"),
				Stone.BottomMeters(6.0 * FMath::Cos(FMath::DegreesToRadians(-47.0)), 6.0 * FMath::Sin(FMath::DegreesToRadians(-47.0))) >= -6.02 - 1e-12);
		}
		// §3.9: at the split the fixed balustrade's handrail tops out at -0.23, under the lowered dial's ribs (-0.13) with 0.10 to
		// spare; the wall rail's top end at -0.18.
		const double Split = Stair::Front(Plan::SplitTread);
		TestEqual(TEXT("the split's handrail top"), Stair::StringTop(Split) + Plan::HandrailHeight + 0.5 * Plan::HandrailHigh, -0.2325, 1e-9);
		TestEqual(TEXT("the wall rail's top"), Stair::Pitch(Stair::Front(Plan::WallRailFromTread)) + Plan::WallRailOverPitch + 0.5 * Plan::WallRailDiameter,
			-0.18, 1e-9);
		// §3.4: 5 rings of 28 coffers, each run round its cell.
		const int32 Coffers = Description.Pieces.FilterByPredicate([](const FFormDrawing& D) { return D.Name.ToString().StartsWith(TEXT("dome.coffer.ring-")); }).Num();
		TestEqual(TEXT("140 coffers"), Coffers, 140);
		const FFormDrawing* Motto = FindPiece(Description, TEXT("floor.motto"));
		TestTrue(TEXT("the motto is lettered"), Motto && Motto->Shape.IsType<FFormLettering>() && Motto->Shape.Get<FFormLettering>().Text.StartsWith(TEXT("HORAS")));
		TestEqual(TEXT("forty risers make the 6.00 from the raised sill to the landing"), Plan::Risers * Plan::Riser, 6.0, 1e-12);
		TestEqual(TEXT("the walking line's going makes Blondel's pace"), 2.0 * Plan::Riser + 3.61 * FMath::DegreesToRadians(Plan::TreadDeg), 0.630, 0.001);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRotundaDialLaidTest, "Musee.Rotunda.DialLaid", Flags)
	bool FRotundaDialLaidTest::RunTest(const FString& Parameters)
	{
		const TValueOrError<Musee::Time::FMuseumDial, FString> Dial = Musee::Time::FMuseumDial::Read();
		if (!TestTrue(TEXT("Time's dial reads"), Dial.HasValue()))
		{
			return false;
		}
		const FSiteDescription Description = Described();
		int32 Ticks = 0;
		int32 Lines = 0;
		double LargestMiss = 0.0;
		for (const Musee::Time::FDialMark& Mark : Dial.GetValue().Marks(1))
		{
			const FString Name = Mark.IsHour() ? (Mark.Minute == 12 * 60 ? FString(TEXT("dial.meridian")) : FString::Printf(TEXT("dial.hour-line.%02d"), Mark.Minute / 60))
				: FString::Printf(TEXT("dial.tick.%04d"), Mark.Minute);
			const FFormDrawing* Piece = FindPiece(Description, *Name);
			if (!Piece)
			{
				AddError(FString::Printf(TEXT("the dial has no %s"), *Name));
				continue;
			}
			(Mark.IsHour() ? Lines : Ticks) += 1;
			// Each mark runs out from the centre along the direction where Time's sun casts the shadow at its minute.
			const FVector Along = Piece->Frame.DirectionToParent(FVector::UpVector);
			LargestMiss = FMath::Max(LargestMiss, (FVector2D(Along) - Mark.Direction).Size());
			LargestMiss = FMath::Max(LargestMiss, FMath::Abs(FMath::Atan2(Piece->Frame.OriginMeters.Y, Piece->Frame.OriginMeters.X)
				- FMath::Atan2(Mark.Direction.Y, Mark.Direction.X)) * Dial.GetValue().ShadowRadiusMeters());
		}
		TestEqual(TEXT("an hour line or the meridian at each of the thirteen hours, VI to VI"), Lines, 13);
		TestEqual(TEXT("a tick at every other minute"), Ticks, 12 * 60 + 1 - 13);
		TestTrue(TEXT("every one laid where the shadow falls, within a micron"), LargestMiss < 1.0e-6);
		const FFormDrawing* Ring = FindPiece(Description, TEXT("dial.hour-ring"));
		TestTrue(TEXT("the hour ring rides the drum on the shadow circle"), Ring && Ring->Body == FMuseeKey(TEXT("rotunda.sun-clock.drum"))
			&& Ring->Shape.IsType<FFormLathe>());
		const FFormDrawing* Meridian = FindPiece(Description, TEXT("dial.meridian"));
		TestTrue(TEXT("the meridian is gilt"), Meridian && Meridian->Surface == FName(TEXT("rotunda.dial.meridian")));

		// The thirteen numerals, lettered in Centaur on the arc, each centred on its hour's line: XII on the meridian.
		int32 Numerals = 0;
		for (const Musee::Time::FDialMark& Mark : Dial.GetValue().Marks(60))
		{
			const FFormDrawing* Numeral = Mark.IsHour() ? FindPiece(Description, *FString::Printf(TEXT("dial.numeral.%02d"), Mark.Minute / 60)) : nullptr;
			if (Numeral && Numeral->Shape.IsType<FFormLettering>())
			{
				const FFormLettering& Letters = Numeral->Shape.Get<FFormLettering>();
				Numerals += FMath::IsNearlyEqual(FMath::DegreesToRadians(Letters.CentreDegrees), FMath::Atan2(Mark.Direction.Y, Mark.Direction.X), 1e-12)
					&& Letters.BaselineRadiusMeters == 6.815 && !Letters.bTopsInward ? 1 : 0;
			}
		}
		TestEqual(TEXT("a numeral on each of the thirteen hours, on its line"), Numerals, 13);
		const FFormDrawing* Noon = FindPiece(Description, TEXT("dial.numeral.12"));
		TestTrue(TEXT("XII at noon"), Noon && Noon->Shape.Get<FFormLettering>().Text == TEXT("XII"));
		return true;
	}
}

#endif
