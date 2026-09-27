#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "MotionSchedule.h"
#include "MuseeRecords.h"
#include "RotundaPlan.h"
#include "RotundaRoom.h"
#include "SiteDescription.h"
#include "SitePortClass.h"

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

		// The dial and the sun clock's drum ride its body; nothing else moves.
		const FMuseeKey Drum(TEXT("rotunda.sun-clock.drum"));
		for (const FFormDrawing& Piece : Description.Pieces)
		{
			const FString Name = Piece.Name.ToString();
			const bool bRides = Name.StartsWith(TEXT("dial.")) || Name.StartsWith(TEXT("sun-clock."));
			TestTrue(*FString::Printf(TEXT("%s rides the drum only if it is the drum's"), *Name), bRides ? Piece.Body == Drum : Piece.Body.IsNone());
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
		if (TestTrue(TEXT("the top landing, the mid landing and the last tread are drawn"), TopLanding && Mid && Last)
			&& TestTrue(TEXT("each a turned stone"), TopLanding->Shape.IsType<FFormLathe>() && Mid->Shape.IsType<FFormLathe>() && Last->Shape.IsType<FFormLathe>()))
		{
			TestEqual(TEXT("the top landing's back edge"), TopLanding->Shape.Get<FFormLathe>().ToDegrees, 194.0, 1e-9);
			TestEqual(TEXT("its nosing"), TopLanding->Shape.Get<FFormLathe>().FromDegrees, 164.0, 1e-9);
			TestEqual(TEXT("the mid landing turns 20 degrees"), Mid->Shape.Get<FFormLathe>().ToDegrees - Mid->Shape.Get<FFormLathe>().FromDegrees, 20.0, 1e-9);
			TestEqual(TEXT("from 69.622 degrees"), Mid->Shape.Get<FFormLathe>().ToDegrees, 69.622, 0.001);
			TestEqual(TEXT("the last riser at -50 degrees"), Last->Shape.Get<FFormLathe>().FromDegrees, -50.0, 1e-9);
		}
		TestEqual(TEXT("forty risers make the 6.00 from the raised sill to the landing"), Plan::Risers * Plan::Riser, 6.0, 1e-12);
		TestEqual(TEXT("the walking line's going makes Blondel's pace"), 2.0 * Plan::Riser + 3.61 * FMath::DegreesToRadians(Plan::TreadDeg), 0.630, 0.001);
		return true;
	}
}

#endif
