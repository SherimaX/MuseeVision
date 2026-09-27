#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "CubeParts.h"
#include "CubePlan.h"
#include "CubeRoom.h"
#include "CubeStrata.h"
#include "Misc/FileHelper.h"
#include "MotionSchedule.h"
#include "MuseeRecords.h"
#include "SiteDescription.h"
#include "SitePortClass.h"

/**
 * The Cube's description against its sheet and its schedules: its port, its well and its class, its anchors against the
 * sheet's table and the mechanisms schedule, the stage contract's own check, the sheet's counts and clearances, the
 * ceiling iris shut and clamped, the ground's rule within its bounds, and a description that is the same on every call.
 * Its shells are proved sealed by Studio's Musee.Studio.RoomsSealed, which makes its drawings.
 */
namespace Musee::Cube::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		/** The sheet's numbers are given to the millimetre. */
		constexpr double Millimetre = 0.001;

		FSiteDescription Described()
		{
			FSiteDescription Description;
			GetDefault<UCubeRoom>()->Describe(Description);
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

		int32 CountPieces(const FSiteDescription& Description, const TCHAR* Prefix)
		{
			int32 Count = 0;
			for (const FFormDrawing& Piece : Description.Pieces)
			{
				Count += Piece.Name.ToString().StartsWith(Prefix, ESearchCase::CaseSensitive) ? 1 : 0;
			}
			return Count;
		}

		/**
		 * Whether a plan point lies on blade k of the ceiling iris turned Turn degrees open from shut: the point taken back
		 * into the blade's open place, then tested against its arc of the middle circle and its two round ends.
		 */
		bool OnBlade(const FVector2D& Point, int32 Blade, double Turn)
		{
			using namespace Plan;
			const FVector2D Pin(BladePin(Blade).OriginMeters);
			const double Back = -FMath::DegreesToRadians(360.0 * Blade / IrisBlades + SwingDeg - Turn);
			const FVector2D D = Point - Pin;
			const FVector2D Local(D.X * FMath::Cos(Back) - D.Y * FMath::Sin(Back), D.X * FMath::Sin(Back) + D.Y * FMath::Cos(Back));
			const FVector2D FromAxis = Local + FVector2D(PinRadius, 0.0);
			const double Half = 0.5 * BladeWidth;
			const double Angle = FMath::RadiansToDegrees(FMath::Atan2(FromAxis.Y, FromAxis.X));
			const double R = FromAxis.Size();
			const double Far = FMath::DegreesToRadians(BladeArcDeg);
			return (Angle >= 0.0 && Angle <= BladeArcDeg && FMath::Abs(R - BladeMiddle) <= Half)
				|| FVector2D::Distance(FromAxis, FVector2D(BladeMiddle, 0.0)) <= Half
				|| FVector2D::Distance(FromAxis, BladeMiddle * FVector2D(FMath::Cos(Far), FMath::Sin(Far))) <= Half;
		}

		bool Covered(const FVector2D& Point, double Turn)
		{
			for (int32 Blade = 0; Blade < Plan::IrisBlades; ++Blade)
			{
				if (OnBlade(Point, Blade, Turn))
				{
					return true;
				}
			}
			return false;
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCubePortsTest, "Musee.Cube.Ports", Flags)
	bool FCubePortsTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		if (!TestEqual(TEXT("one port"), Description.Ports.Num(), 1))
		{
			return false;
		}
		const FSitePort& Up = Description.Ports[0];
		TestEqual(TEXT("it is cube.up"), Up.Key, FMuseeKey(TEXT("cube.up")));
		TestTrue(TEXT("on the Atrium slab's soffit on the axis"), Up.Frame.OriginMeters.Equals(FVector(0.0, 0.0, -0.5), 1e-9));
		TestTrue(TEXT("facing up"), Up.Facing().Equals(FVector(0.0, 0.0, 1.0), 1e-9));
		TestEqual(TEXT("an iris.4600"), Up.Class, FMuseeKey(TEXT("iris.4600")));
		TestEqual(TEXT("7.500 deep, down to the ceiling"), Up.WallDepthMeters, 7.5, 1e-9);
		TestEqual(TEXT("it meets the Atrium's down port"), Up.Mates, FMuseeKey(TEXT("atrium.down")));
		TestTrue(TEXT("the Cube receives its well"), Up.bHasWell && !Up.Well.bServing);
		TestTrue(TEXT("the well runs from the plane to -23.800"), Up.Well.MinMeters.Equals(FVector(-2.3, -2.3, -23.8), 1e-9)
			&& Up.Well.MaxMeters.Equals(FVector(2.3, 2.3, -0.5), 1e-9));
		TestEqual(TEXT("the ceiling iris's top blade lies 7.442 under the plane"), Plan::MatingPlane - Plan::TopBlade, 7.442, 1e-9);

		const TValueOrError<FSitePortClass, FString> Class = Musee::Site::FindPortClass(Up.Class);
		if (Class.HasValue())
		{
			TestEqual(TEXT("the class's clear width is the well's diameter"), Class.GetValue().ClearWidthMeters, 2.0 * Plan::WellRadius, 1e-9);
			TestTrue(TEXT("the class is a shaft"), Class.GetValue().Kind == ESitePortKind::Shaft);
		}
		else
		{
			AddWarning(FString::Printf(TEXT("Site's records have no %s yet (KIT-2's vertical ports): %s"), *Up.Class.ToString(), *Class.GetError()));
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCubeAnchorsTest, "Musee.Cube.Anchors", Flags)
	bool FCubeAnchorsTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();

		// The sheet's table (§2): the blades at their pins on their top faces, with the stage's axes.
		const FVector Pins[] = {{3.745, 0.0, -7.942}, {2.335, 2.928, -7.946}, {-0.833, 3.651, -7.950}, {-3.374, 1.625, -7.954},
			{-3.374, -1.625, -7.958}, {-0.833, -3.651, -7.962}, {2.335, -2.928, -7.966}};
		for (int32 Blade = 0; Blade < Plan::IrisBlades; ++Blade)
		{
			const FMuseeStageAnchor* Anchor = FindAnchor(Description, FMuseeKey(FString::Printf(TEXT("cube.up.iris.blade.%d"), Blade + 1)), TEXT("Body"));
			if (TestNotNull(*FString::Printf(TEXT("blade %d is a Body"), Blade + 1), Anchor))
			{
				TestTrue(*FString::Printf(TEXT("blade %d turns on its pin"), Blade + 1), Anchor->Frame.OriginMeters.Equals(Pins[Blade], Millimetre));
				TestTrue(*FString::Printf(TEXT("blade %d has the stage's axes"), Blade + 1), Anchor->Frame.Rotation.Equals(FQuat::Identity, 1e-12));
			}
		}
		const FMuseeStageAnchor* Use = FindAnchor(Description, FMuseeKey(TEXT("cube.performance")), TEXT("Use"));
		TestTrue(TEXT("the work's control is on the car's floor at the cube stop"), Use && Use->Frame.OriginMeters.Equals(FVector(0.0, 0.0, -23.6), 1e-9));

		// The six panel faces, each facing into the room along its x, towards the centre.
		struct FMount
		{
			const TCHAR* Face;
			FVector Centre;
		};
		const FMount Mounts[] = {{TEXT("ceiling"), FVector(0.0, 0.0, -8.0)}, {TEXT("floor"), FVector(0.0, 0.0, -36.0)}, {TEXT("north"), FVector(0.0, -14.0, -22.0)},
			{TEXT("south"), FVector(0.0, 14.0, -22.0)}, {TEXT("east"), FVector(14.0, 0.0, -22.0)}, {TEXT("west"), FVector(-14.0, 0.0, -22.0)}};
		for (const FMount& Mount : Mounts)
		{
			const FMuseeStageAnchor* Anchor = FindAnchor(Description, FMuseeKey(FString::Printf(TEXT("cube.panels.%s"), Mount.Face)), TEXT("Mount"));
			if (TestNotNull(*FString::Printf(TEXT("the %s's panels are a Mount"), Mount.Face), Anchor))
			{
				TestTrue(*FString::Printf(TEXT("at the %s's centre"), Mount.Face), Anchor->Frame.OriginMeters.Equals(Mount.Centre, 1e-9));
				const FVector Inward = (FVector(0.0, 0.0, Plan::EyesZ) - Mount.Centre).GetSafeNormal();
				TestTrue(*FString::Printf(TEXT("the %s faces the centre"), Mount.Face), Anchor->Frame.DirectionToParent(FVector::ForwardVector).Equals(Inward, 1e-9));
			}
		}
		TestEqual(TEXT("seven bodies, one control and six mounts, nothing else"), Description.Anchors.Num(), 14);

		// Every body the mechanisms name. Motion reads performances and derived channels from the kit's next round; until
		// then its reader can't take the schedule, and that is Motion's to say.
		const TValueOrError<FMotionSchedule, FString> Motion = Musee::ReadRecord<FMotionSchedule>(Schedule(TEXT("Mechanisms.json")));
		if (Motion.HasError())
		{
			AddWarning(FString::Printf(TEXT("Motion can't yet read the Cube's mechanisms (performances and derived channels): %s"), *Motion.GetError()));
			return true;
		}
		TestEqual(TEXT("seven mechanisms, the ceiling iris's blades"), Motion.GetValue().Mechanisms.Num(), Plan::IrisBlades);
		for (const FMotionMechanismSpec& Mechanism : Motion.GetValue().Mechanisms)
		{
			TestNotNull(*FString::Printf(TEXT("%s moves a Body anchor"), *Mechanism.Key.ToString()), FindAnchor(Description, Mechanism.Body, TEXT("Body")));
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCubeDescribedTest, "Musee.Cube.Described", Flags)
	bool FCubeDescribedTest::RunTest(const FString& Parameters)
	{
		const FSiteDescription Description = Described();
		TestEqual(TEXT("it is the cube stage"), Description.Stage, FMuseeKey(TEXT("cube")));
		const TArray<FMuseeFinding> Findings = Musee::Site::CheckDescription(Description);
		for (const FMuseeFinding& Finding : Findings)
		{
			AddError(Finding.Summary);
		}
		TestEqual(TEXT("the stage contract holds"), Findings.Num(), 0);
		TestEqual(TEXT("three enclosures"), Description.Enclosures.Num(), 3);
		for (const FFormEnclosure& Enclosure : Description.Enclosures)
		{
			TestTrue(*FString::Printf(TEXT("%s is bounded by drawn pieces"), *Enclosure.Key.ToString()), Enclosure.Pieces.Num() > 0);
		}
		TestFalse(TEXT("no daylight and no sky: no Glazed or Open opening"), Description.Openings.ContainsByPredicate([](const FFormOpening& O)
		{
			return O.Kind == EFormOpeningKind::Glazed || O.Kind == EFormOpeningKind::Open;
		}));
		TestEqual(TEXT("the port and the iris's housing"), Description.Openings.Num(), 2);

		// Every surface a piece names is in the stage's finish schedule, and every moving piece rides a declared body.
		FString Finishes;
		if (TestTrue(TEXT("the finish schedule reads"), FFileHelper::LoadFileToString(Finishes, *(Musee::Records::DataRoot() / Schedule(TEXT("Finishes.json"))))))
		{
			TSet<FName> Seen;
			for (const FFormDrawing& Piece : Description.Pieces)
			{
				if (!Seen.Contains(Piece.Surface))
				{
					Seen.Add(Piece.Surface);
					const FString Entry = FString::Printf(TEXT("\"surface\": \"%s\""), *Piece.Surface.ToString());
					TestTrue(*FString::Printf(TEXT("%s's surface %s is scheduled"), *Piece.Name.ToString(), *Piece.Surface.ToString()), Finishes.Contains(Entry));
				}
				if (!Piece.Body.IsNone())
				{
					TestNotNull(*FString::Printf(TEXT("%s rides a Body"), *Piece.Name.ToString()), FindAnchor(Description, Piece.Body, TEXT("Body")));
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
		TestTrue(TEXT("the same pieces, in the same order and places"), bSame);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCubePlanTest, "Musee.Cube.Plan", Flags)
	bool FCubePlanTest::RunTest(const FString& Parameters)
	{
		using namespace Plan;
		const FSiteDescription Description = Described();

		// §1: the room 28 m each way, 21,952 m³, its centre the eyes at the stop.
		TestEqual(TEXT("the room holds 21,952 m³"), FMath::Square(2.0 * FaceHalf) * (Ceiling - Floor), 21952.0, 1e-6);
		TestEqual(TEXT("the eyes stop at the room's centre"), 0.5 * (Ceiling + Floor), EyesZ, 1e-9);

		// §3.2: 9,512 modules, 88 omitted in the ceiling's opening, 44 of the ceiling's shaped to it.
		TestEqual(TEXT("9,512 modules"), CountPieces(Description, TEXT("panel.")), Modules);
		TestEqual(TEXT("the ceiling's 1,600 less the 88 inside its opening"), CountPieces(Description, TEXT("panel.ceiling.")),
			ModulesPerEdge * ModulesPerEdge - OmittedModules);
		int32 Shaped = 0;
		for (const FFormDrawing& Piece : Description.Pieces)
		{
			const FFormExtrusion* Module = Piece.Name.ToString().StartsWith(TEXT("panel.")) ? Piece.Shape.TryGet<FFormExtrusion>() : nullptr;
			Shaped += Module && Module->Outline.Steps.ContainsByPredicate([](const FFormProfileStep& Step) { return Step.IsArc(); }) ? 1 : 0;
		}
		TestEqual(TEXT("44 shaped to the opening"), Shaped, ShapedModules);

		// §3.3-§3.7: the counts.
		TestEqual(TEXT("sixteen panes"), CountPieces(Description, TEXT("shaft.glass.")), Tiers * PanesPerTier);
		TestEqual(TEXT("five frames, each two halves round the rails"), CountPieces(Description, TEXT("shaft.frame.")), 10);
		TestEqual(TEXT("sixteen joint strips"), CountPieces(Description, TEXT("shaft.strip.")), 16);
		TestEqual(TEXT("fourteen hangers"), CountPieces(Description, TEXT("housing.hanger.")), Hangers);
		TestEqual(TEXT("seven blades"), CountPieces(Description, TEXT("iris.blade.")), IrisBlades);
		TestEqual(TEXT("five layers, two collars and the ammonite"), CountPieces(Description, TEXT("ground.")), 8);

		// §6 clearances.
		TestEqual(TEXT("rail foot to top blade 5 mm"), RailBottom - TopBlade, 0.005, 1e-9);
		TestEqual(TEXT("stack bottom to tails 5 mm"), StackBottom - TailTop, 0.005, 1e-9);
		TestEqual(TEXT("stack bottom to the soffit ring's top 2 mm"), StackBottom - SoffitRing[0][1], 0.002, 1e-9);
		TestEqual(TEXT("top blade to the housing wall's foot 4 mm"), HousingInnerWall[0] - TopBlade, 0.004, 1e-9);
		TestEqual(TEXT("the stack is 26 mm"), TopBlade - StackBottom, 0.026, 1e-9);
		TestEqual(TEXT("blades in travel to the hangers 30 mm"), HangerInner - BladeSweep, 0.030, 1e-9);
		TestEqual(TEXT("blades in travel to the ceiling's cut 120 mm"), RoughOpening - BladeSweep, 0.120, 1e-9);
		TestEqual(TEXT("open blades stand 10 mm inside the seat"), BladeOpenInner - SeatRadius, 0.010, 1e-9);
		TestEqual(TEXT("open, the blade's inner edge is r 2.470"), BladeMiddle - 0.5 * BladeWidth, BladeOpenInner, 1e-9);
		TestTrue(TEXT("the rails stand in the class's notches"), RailBladeInner < WellRadius && RailFootInner > WellRadius);
		TestTrue(TEXT("the ground's relief stays clear of the webs"), GroundFace - Strata::HighestProud() > WebOuter);
		TestTrue(TEXT("and within the sheet's 0.050"), Strata::HighestProud() <= GroundReliefMax);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCubeIrisTest, "Musee.Cube.Iris", Flags)
	bool FCubeIrisTest::RunTest(const FString& Parameters)
	{
		using namespace Plan;
		// Shut, the seven blades cover the bore to the seat.
		constexpr double Step = 0.05;
		int32 Open = 0;
		for (double X = -SeatRadius; X <= SeatRadius; X += Step)
		{
			for (double Y = -SeatRadius; Y <= SeatRadius; Y += Step)
			{
				Open += FVector2D(X, Y).Size() <= SeatRadius && !Covered(FVector2D(X, Y), 0.0) ? 1 : 0;
			}
		}
		TestEqual(TEXT("shut, the iris closes the bore"), Open, 0);

		// Open, no blade stands inside the seat.
		bool bClear = true;
		for (int32 Index = 0; Index < 720 && bClear; ++Index)
		{
			const double A = 2.0 * UE_DOUBLE_PI * Index / 720;
			bClear = !Covered((SeatRadius - Millimetre) * FVector2D(FMath::Cos(A), FMath::Sin(A)), SwingDeg);
		}
		TestTrue(TEXT("open, the bore is clear to the seat"), bClear);

		// Clamped (the contract's 0.3206 under Motion's linear follow, the sheet's 0.378 eased), the blades leave r 0.197
		// round the mast head's groove r 0.195.
		const double Clamp = 0.3206;
		TestEqual(TEXT("the clamp is the sheet's 0.378 eased"), 3.0 * FMath::Square(0.378) - 2.0 * FMath::Cube(0.378), Clamp, 1e-4);
		double Hole = TNumericLimits<double>::Max();
		for (int32 Index = 0; Index < 720; ++Index)
		{
			const FVector2D Direction(FMath::Cos(2.0 * UE_DOUBLE_PI * Index / 720), FMath::Sin(2.0 * UE_DOUBLE_PI * Index / 720));
			double R = 0.15;
			while (R < 0.3 && !Covered(R * Direction, Clamp * SwingDeg))
			{
				R += 0.0002;
			}
			Hole = FMath::Min(Hole, R);
		}
		TestEqual(TEXT("clamped, the blades leave r 0.197"), Hole, 0.197, Millimetre);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCubeGroundTest, "Musee.Cube.Ground", Flags)
	bool FCubeGroundTest::RunTest(const FString& Parameters)
	{
		using namespace Plan;
		// The rule on a 50 mm grid over the whole face: within its bounds, and the layers in their order top down.
		double Deepest = 0.0;
		double Highest = 0.0;
		bool bOrdered = true;
		const int32 Round = 336;
		for (int32 I = 0; I < Round; ++I)
		{
			const double Theta = 2.0 * UE_DOUBLE_PI * I / Round;
			const Strata::FContacts Contacts = Strata::ContactsAt(Theta);
			bOrdered &= Contacts.Z[0] > Contacts.Z[1] && Contacts.Z[1] > Contacts.Z[2] && Contacts.Z[2] > Contacts.Z[3];
			for (double Z = GroundBottom; Z <= GroundTop; Z += 0.05)
			{
				const double Proud = Strata::ProudAt(Theta, Z);
				Deepest = FMath::Min(Deepest, Proud);
				Highest = FMath::Max(Highest, Proud);
			}
		}
		TestTrue(TEXT("the contacts keep their order round the shaft"), bOrdered);
		TestTrue(TEXT("the face never lies deeper than the rule's deepest recess"), -Deepest <= Strata::DeepestRecess());
		TestTrue(TEXT("nor stands prouder than its highest"), Highest <= Strata::HighestProud());
		TestTrue(TEXT("and it has relief"), Highest > 0.005 && Deepest < -0.002);

		// The pipe carries the clay 0.75 into the chalk at bearing 205.
		const double Pipe = FMath::DegreesToRadians(PipeBearing - 90.0);
		TestTrue(TEXT("the solution pipe drops the clay towards -4.750"), Strata::ContactsAt(Pipe).Z[3] < ClayChalk.Mean - 0.5);
		TestTrue(TEXT("chalk lies at -4.450 across the shaft from it"), Strata::LayerAt(Pipe + UE_DOUBLE_PI, -4.45) == Strata::ELayer::Chalk);
		TestTrue(TEXT("and clay in it"), Strata::LayerAt(Pipe, -4.45) == Strata::ELayer::ClayWithFlints);

		// The ammonite: 0.360 across, its shell no prouder than the sheet's 0.050 of the nominal face.
		const double Behind = Strata::DeepestRecess() + AmmoniteProud;
		double Peak = 0.0;
		double Reach = 0.0;
		for (double X = -0.2; X <= 0.2; X += 0.002)
		{
			for (double Y = -0.2; Y <= 0.2; Y += 0.002)
			{
				const double H = Strata::AmmoniteAt(X, Y, Behind);
				Peak = FMath::Max(Peak, H);
				Reach = H > 0.0 ? FMath::Max(Reach, FVector2D(X, Y).Size()) : Reach;
			}
		}
		TestTrue(TEXT("the shell is half exposed within the sheet's relief"), Peak - Strata::DeepestRecess() <= GroundReliefMax);
		TestTrue(TEXT("its coil reaches 0.180 from its centre, its ribs a little more"), Reach >= 0.5 * AmmoniteDiameter - 0.002
			&& Reach <= 0.5 * AmmoniteDiameter * (1.0 + RibHeight));
		TestTrue(TEXT("its ground holds it"), Reach < 0.5 * AmmoniteField);
		return true;
	}
}

#endif
