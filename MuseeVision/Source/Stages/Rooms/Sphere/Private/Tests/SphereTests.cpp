#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "MotionSchedule.h"
#include "MuseeRecords.h"
#include "SiteDescription.h"
#include "SitePortClass.h"
#include "SpherePlan.h"
#include "SphereRoom.h"

/**
 * The Sphere's description against its sheet and its schedules: its vertical port and the well it receives, its anchors
 * against every body the mechanisms move and the mount the lighting names, the stage contract's own check, the sheet's
 * measured numbers, and a description that is the same on every call. Its shells are proved sealed by Studio's
 * Musee.Studio.RoomsSealed, which makes its drawings.
 */
namespace Musee::Sphere::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		FSiteDescription Described()
		{
			FSiteDescription Description;
			GetDefault<USphereRoom>()->Describe(Description);
			return Description;
		}

		const FMuseeStageAnchor* FindAnchor(const FSiteDescription& Description, const FMuseeKey& Key, FName Kind)
		{
			return Description.Anchors.FindByPredicate([&](const FMuseeStageAnchor& A) { return A.Key == Key && A.Kind == Kind; });
		}

		const FFormDrawing* FindPiece(const FSiteDescription& Description, const TCHAR* Name)
		{
			return Description.Pieces.FindByPredicate([Name](const FFormDrawing& P) { return P.Name == FName(Name); });
		}

		FString Schedule(const TCHAR* File)
		{
			return FString::Printf(TEXT("Stages/%s/%s"), Plan::Stage, File);
		}

		bool ReadSchedule(const TCHAR* File, FString& Out)
		{
			return FFileHelper::LoadFileToString(Out, *(Musee::Records::DataRoot() / Schedule(File)));
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpherePortsTest, "Musee.Sphere.Ports", Flags)
	bool FSpherePortsTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		if (!TestEqual(TEXT("one port"), Description.Ports.Num(), 1))
		{
			return false;
		}
		const FSitePort& Down = Description.Ports[0];
		TestEqual(TEXT("it is sphere.down"), Down.Key, FMuseeKey(TEXT("sphere.down")));
		TestTrue(TEXT("a lift's port"), Down.Kind == ESitePortKind::Shaft);
		TestEqual(TEXT("of the class iris.4600"), Down.Class, FMuseeKey(TEXT("iris.4600")));
		TestTrue(TEXT("on the axis at the mating plane"), Down.Frame.OriginMeters.Equals(FVector(0.0, 0.0, 7.715), 1e-9));
		TestTrue(TEXT("facing down"), Down.Facing().Equals(FVector(0.0, 0.0, -1.0), 1e-9));
		TestEqual(TEXT("the neck is its depth, to the flange's top"), Down.WallDepthMeters, 3.685, 1e-9);
		TestEqual(TEXT("it meets the Atrium's up port"), Down.Mates, FMuseeKey(TEXT("atrium.up")));
		TestTrue(TEXT("it receives the Atrium's well"), Down.bHasWell && !Down.Well.bServing);
		TestTrue(TEXT("the well is the class's circle r 2.300 from the plane to 23.390"), Down.Well.MinMeters.Equals(FVector(-2.3, -2.3, 7.715), 1e-9)
			&& Down.Well.MaxMeters.Equals(FVector(2.3, 2.3, 23.39), 1e-9));

		// The port's class is the site's record; its clear opening is the well's circle.
		const TValueOrError<FSitePortClass, FString> Class = Musee::Site::FindPortClass(Down.Class);
		if (TestTrue(TEXT("iris.4600 is in the site's records"), Class.HasValue()))
		{
			TestEqual(TEXT("its clear opening is the well's diameter"), Class.GetValue().ClearWidthMeters, 2.0 * Plan::WellRadius, 1e-9);
		}
		else
		{
			AddError(Class.GetError());
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSphereAnchorsTest, "Musee.Sphere.Anchors", Flags)
	bool FSphereAnchorsTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		const TValueOrError<FMotionSchedule, FString> Motion = Musee::ReadRecord<FMotionSchedule>(Schedule(TEXT("Mechanisms.json")));
		if (!TestTrue(TEXT("the mechanisms schedule reads"), Motion.HasValue()))
		{
			AddError(Motion.GetError());
			return false;
		}
		TestEqual(TEXT("the mast, its foot, four dogs and fourteen blades"), Motion.GetValue().Mechanisms.Num(), 20);
		for (const FMotionMechanismSpec& Mechanism : Motion.GetValue().Mechanisms)
		{
			const FString Name = Mechanism.Key.ToString();
			const FMuseeStageAnchor* Body = FindAnchor(Description, Mechanism.Body, TEXT("Body"));
			if (!TestNotNull(*FString::Printf(TEXT("%s moves a Body anchor"), *Name), Body) || Mechanism.Axis.Num() != 3)
			{
				continue;
			}
			const FVector Axis = FVector(Mechanism.Axis[0], Mechanism.Axis[1], Mechanism.Axis[2]).GetSafeNormal();
			const FVector At = Body->Frame.OriginMeters;
			if (Mechanism.Joint == FName(TEXT("revolute")))
			{
				// A blade turns about its pin, its body's origin, on the pins' circle.
				TestEqual(*FString::Printf(TEXT("%s's body stands on its pin"), *Name), FVector2D(At.X, At.Y).Size(), Plan::IrisPin, 1e-9);
				TestTrue(*FString::Printf(TEXT("%s turns about the vertical"), *Name), FMath::Abs(Axis.Z) > 1.0 - 1e-9);
			}
			else if (Name.StartsWith(TEXT("sphere.mast.dogs.")))
			{
				// A dog slides along its body's x, inward from its retracted tip by the stroke to r 0.300.
				TestTrue(*FString::Printf(TEXT("%s slides along its body's x"), *Name), Body->Frame.DirectionToParent(FVector::ForwardVector).Equals(Axis, 1e-3));
				TestEqual(*FString::Printf(TEXT("%s's tip stands retracted at r 2.560"), *Name), FVector2D(At.X, At.Y).Size(), Plan::DogRetracted, 1e-9);
				TestEqual(*FString::Printf(TEXT("%s's travel is the stroke"), *Name), double(Mechanism.Travel), Plan::DogRetracted - Plan::DogExtended, 1e-5);
			}
			else
			{
				// The mast's head and foot slide on the axis, from the foot plane.
				TestTrue(*FString::Printf(TEXT("%s stands on the axis at the foot plane"), *Name), At.Equals(FVector(0.0, 0.0, Plan::Mouth), 1e-9));
				TestTrue(*FString::Printf(TEXT("%s slides along the axis"), *Name), FMath::Abs(Axis.Z) > 1.0 - 1e-9);
			}
		}

		// The sheet's anchors table: the blades' pins, the dogs' tips, the use and the mount.
		const FMuseeStageAnchor* Blade2 = FindAnchor(Description, FMuseeKey(TEXT("sphere.neck.iris.blade.2")), TEXT("Body"));
		const FMuseeStageAnchor* Blade7 = FindAnchor(Description, FMuseeKey(TEXT("sphere.platform.iris.blade.7")), TEXT("Body"));
		const FMuseeStageAnchor* Dog1 = FindAnchor(Description, FMuseeKey(TEXT("sphere.mast.dogs.1")), TEXT("Body"));
		if (TestNotNull(TEXT("neck blade 2"), Blade2) && TestNotNull(TEXT("platform blade 7"), Blade7) && TestNotNull(TEXT("dog 1"), Dog1))
		{
			TestTrue(TEXT("neck blade 2 at (2.335, 2.928, 11.392)"), Blade2->Frame.OriginMeters.Equals(FVector(2.335, 2.928, 11.392), 0.0005));
			TestTrue(TEXT("platform blade 7 at (2.335, -2.928, 20.372)"), Blade7->Frame.OriginMeters.Equals(FVector(2.335, -2.928, 20.372), 0.0005));
			TestTrue(TEXT("dog 1 at (1.810, -1.810, 11.150)"), Dog1->Frame.OriginMeters.Equals(FVector(1.810, -1.810, 11.150), 0.0005));
		}
		TestNotNull(TEXT("the call is a Use at the platform way's centre"), FindAnchor(Description, FMuseeKey(TEXT("sphere.call")), TEXT("Use")));
		const FMuseeStageAnchor* Mount = FindAnchor(Description, FMuseeKey(TEXT("sphere.image.mount")), TEXT("Mount"));
		if (TestNotNull(TEXT("the image's mount"), Mount))
		{
			TestTrue(TEXT("at the centre C"), Mount->Frame.OriginMeters.Equals(FVector(0.0, 0.0, 22.0), 1e-9));
		}
		FString Lighting;
		if (TestTrue(TEXT("the lighting schedule reads"), ReadSchedule(TEXT("Lighting.json"), Lighting)))
		{
			TestTrue(TEXT("the image luminaire hangs on the mount"), Lighting.Contains(TEXT("\"mount\": \"sphere.image.mount\"")));
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSphereDescribedTest, "Musee.Sphere.Described", Flags)
	bool FSphereDescribedTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		TestEqual(TEXT("it is the sphere stage"), Description.Stage, FMuseeKey(TEXT("sphere")));
		const TArray<FMuseeFinding> Findings = Musee::Site::CheckDescription(Description);
		for (const FMuseeFinding& Finding : Findings)
		{
			AddError(Finding.Summary);
		}
		TestEqual(TEXT("the stage contract holds"), Findings.Num(), 0);
		TestEqual(TEXT("two enclosures"), Description.Enclosures.Num(), 2);
		TestFalse(TEXT("it has no opening to the sky: it shows the sky, never sees it"), Description.Openings.ContainsByPredicate([](const FFormOpening& O)
		{
			return O.Kind == EFormOpeningKind::Glazed || O.Kind == EFormOpeningKind::Open;
		}));
		TestEqual(TEXT("the footprint is the 96-gon"), Description.Footprint.OutlineMeters.Num(), Plan::FootprintSides);

		// Every surface a piece names is in the stage's finish schedule, and every body it rides is a Body anchor.
		FString Finishes;
		if (TestTrue(TEXT("the finish schedule reads"), ReadSchedule(TEXT("Finishes.json"), Finishes)))
		{
			for (const FFormDrawing& Piece : Description.Pieces)
			{
				const FString Entry = FString::Printf(TEXT("\"surface\": \"%s\""), *Piece.Surface.ToString());
				TestTrue(*FString::Printf(TEXT("%s's surface %s is scheduled"), *Piece.Name.ToString(), *Piece.Surface.ToString()), Finishes.Contains(Entry));
				TestTrue(*FString::Printf(TEXT("%s rides a declared body"), *Piece.Name.ToString()),
					Piece.Body.IsNone() || FindAnchor(Description, Piece.Body, TEXT("Body")) != nullptr);
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

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpherePlanTest, "Musee.Sphere.Plan", Flags)
	bool FSpherePlanTest::RunTest(const FString& Parameters)
	{
		const double C = Plan::CentreZ;
		auto Below = [C](double R, double r) { return C - FMath::Sqrt(R * R - r * r); };
		auto Above = [C](double R, double r) { return C + FMath::Sqrt(R * R - r * r); };

		// §3.1, §3.3: the plate and the image surface meet the throat's outer face where the sheet says.
		TestEqual(TEXT("the plate's outer face meets the throat at 8.372"), Below(Plan::PlateOuter, Plan::ThroatOuter), 8.372, 0.0005);
		TestEqual(TEXT("its inner face at 8.382"), Below(Plan::PlateInner, Plan::ThroatOuter), 8.382, 0.0005);
		TestEqual(TEXT("the image surface meets the throat at 8.628"), Below(Plan::ImageInner, Plan::ThroatOuter), 8.628, 0.0005);
		TestTrue(TEXT("the plate stays 50 mm clear of the Atrium's canopy sphere R 14.000"), 14.000 - Plan::PlateOuter >= 0.050 - 1e-9);

		// §3.2: the skin's foot on the ring, the ring bedded on the wash, the boss on the skin, the ball in its seat.
		const double FootInner = FMath::Sqrt(FMath::Square(Plan::SkinInner) - FMath::Square(Plan::SkinFoot - C));
		TestEqual(TEXT("the skin's foot starts at r 14.2263"), FootInner, 14.2263, 0.00005);
		auto Wash = [](double r) { return Plan::WashInnerZ - Plan::WashFallPerMeter * (r - Plan::WashInnerR); };
		TestEqual(TEXT("the ring beds on the wash at 22.5716"), Wash(FootInner), 22.5716, 0.00005);
		TestEqual(TEXT("and out to 22.5679"), Wash(Plan::RingFlatOuter), 22.5679, 0.00005);
		TestEqual(TEXT("the ring's half-round tops out at the skin's foot"), Plan::RingRoundZ + Plan::RingRoundRadius, Plan::SkinFoot, 1e-9);
		TestTrue(TEXT("the half-round's underside stands clear of the wash (the drip)"), Plan::RingRoundZ - Plan::RingRoundRadius > Wash(Plan::RingFlatOuter));
		TestEqual(TEXT("the boss's rim sits on the skin at 36.2894"), Above(Plan::SkinOuter, Plan::BossRim), 36.2894, 0.00005);
		TestTrue(TEXT("the spigot passes the crown hole"), Plan::SpigotRadius < Plan::CrownHole);
		TestEqual(TEXT("the ball's seat meets the cone at 36.5869"), Plan::BallCentreZ - FMath::Sqrt(FMath::Square(Plan::BallRadius) - FMath::Square(Plan::BossSeatR)),
			36.5869, 0.00005);
		TestEqual(TEXT("the ball's top is the stage's top"), Plan::BallCentreZ + Plan::BallRadius, Plan::FootprintTop, 1e-9);
		TestEqual(TEXT("the footprint's 96-gon holds the drip ring, r 14.318"),
			Plan::FootprintCircumradius * FMath::Cos(UE_DOUBLE_PI / Plan::FootprintSides), 14.318, 0.0005);

		// §3.4-3.6: the seats, the irises and the mast's footing.
		const double Seat = Plan::SeatCircumradius * FMath::Cos(UE_DOUBLE_PI / Plan::SeatSides);
		TestEqual(TEXT("the seats' 96-gon is inscribed at 2.4587"), Seat, 2.4587, 0.00005);
		TestTrue(TEXT("an open blade's inner edge (2.470) stands in the seat"), Plan::IrisMiddle - 0.5 * Plan::IrisWidth >= Plan::SeatCircumradius);
		const double NeckStack = Plan::NeckIrisTop - (Plan::IrisBlades - 1) * Plan::IrisPitch - Plan::IrisBlade;
		TestEqual(TEXT("the neck iris's stack is 11.370 … 11.396"), NeckStack, 11.370, 1e-9);
		TestTrue(TEXT("inside its pocket"), NeckStack >= Plan::NeckPocketBottom && Plan::NeckIrisTop <= Plan::NeckPocketTop);
		TestEqual(TEXT("the rails stop 30 mm under it"), NeckStack - Plan::RailTop, 0.030, 1e-9);
		const double PlatformStack = Plan::PlatformIrisTop - (Plan::IrisBlades - 1) * Plan::IrisPitch - Plan::IrisBlade;
		TestTrue(TEXT("the platform iris's stack lies in its pocket"), PlatformStack >= Plan::PlatformPocketBottom && Plan::PlatformIrisTop <= Plan::PlatformPocketTop);
		TestEqual(TEXT("the dogs' housings' floor is their plate"), Plan::Mouth - Plan::DogDepth - Plan::HousingBottom, Plan::HousingPlate, 1e-9);
		TestTrue(TEXT("a retracted dog stands behind the bore and short of its housing's end"), Plan::DogRetracted > Plan::ThroatBore
			&& Plan::DogRetracted + Plan::DogLength < Plan::HousingOuterEnd - Plan::HousingPlate);
		TestTrue(TEXT("an extended dog keeps 0.800 of its bar in the housing"), Plan::DogExtended + Plan::DogLength - Plan::ThroatBore >= 0.800);
		TestTrue(TEXT("the foot ring is drawn 2 mm over the dogs"), FMath::IsNearlyEqual(Plan::FootRingBottom - Plan::Mouth, 0.002, 1e-9));

		// §3.5: the balustrade's panes stand 5 mm clear of the posts, and the struts are 8.827 long, 6.5° off vertical.
		const double PostSideDeg = FMath::RadiansToDegrees(0.5 * Plan::PostSide / Plan::BalustradeRadius);
		const double Clear = FMath::DegreesToRadians(Plan::PostFirstBearing - PostSideDeg - 0.5 * Plan::PaneDeg) * Plan::BalustradeRadius;
		TestEqual(TEXT("each pane stands 5 mm clear of its posts"), Clear, 0.005, 0.0005);
		const FVector2D Strut(Plan::StrutHeadR - Plan::StrutFootR, Plan::DeckSoffit - Plan::NeckTop);
		TestEqual(TEXT("a strut is 8.827 long"), Strut.Size(), 8.827, 0.0005);
		TestEqual(TEXT("and 6.5° off vertical"), FMath::RadiansToDegrees(FMath::Atan2(Strut.X, Strut.Y)), 6.5, 0.05);
		TestTrue(TEXT("its feet stand on the flange's rim"), Plan::StrutFootR + 0.5 * Plan::StrutPlateDiameter <= Plan::FlangeOuter);

		// The pieces the sheet's first-pass cut asks for are drawn.
		const FSiteDescription Description = Described();
		for (const TCHAR* Name : {TEXT("shell.plate"), TEXT("image.surface"), TEXT("dome.skin.01.01"), TEXT("dome.skin.15.24"), TEXT("dome.drip-ring"), TEXT("dome.boss"), TEXT("dome.ball"),
			TEXT("neck.throat"), TEXT("neck.flange"), TEXT("neck.rail.north"), TEXT("platform.deck"), TEXT("platform.sill"), TEXT("platform.handrail"),
			TEXT("mast.foot-ring"), TEXT("mast.dog.4"), TEXT("neck.iris.blade.7"), TEXT("platform.iris.blade.7")})
		{
			TestNotNull(*FString::Printf(TEXT("%s is drawn"), Name), FindPiece(Description, Name));
		}
		return true;
	}
}

#endif
