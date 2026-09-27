#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "HAL/FileManager.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "MuseeRecords.h"
#include "SiteBlind.h"
#include "SiteJoint.h"
#include "SitePlan.h"
#include "SitePortClass.h"

/**
 * The joints and the vertical ports: every class a sheet names is a port class; a vertical port mates when its well
 * coincides with its neighbour's, and a downward one left unmated is shut by its class's iris; and the joint between two
 * mated ports draws the doorcase each meeting of faces needs (the wedges to a flat face tangent to the drum, a passage
 * between the drum and a flat face, nothing between two faces on one cylinder).
 */
namespace Musee::Site::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags JointFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		/** A port placed in the museum's frame, facing a way given as a yaw in degrees (a pitch for a vertical one). */
		FSitePort PortAt(const TCHAR* Key, const TCHAR* Class, const FVector& Origin, const FRotator& Facing, double Depth, double FaceRadius,
			bool bOwnSurround)
		{
			FSitePort P;
			P.Key = FMuseeKey(Key);
			P.Class = FMuseeKey(Class);
			P.Frame = FMuseeFrame(Origin, Facing.Quaternion());
			P.WallDepthMeters = Depth;
			P.FaceRadiusMeters = FaceRadius;
			P.bOwnSurround = bOwnSurround;
			return P;
		}

		TSet<FString> PartsOf(const FSiteJoint& Joint, const FString& Prefix)
		{
			TSet<FString> Parts;
			for (const FFormDrawing& Drawing : Joint.Drawings)
			{
				FString Part = Drawing.Name.ToString();
				Part.RemoveFromStart(Prefix);
				Parts.Add(Part);
			}
			return Parts;
		}

		const FFormDrawing* Find(const FSiteJoint& Joint, const FString& Name)
		{
			return Joint.Drawings.FindByPredicate([&Name](const FFormDrawing& D) { return D.Name == FName(*Name); });
		}

		/**
		 * The corners a drawn outline passes through (its start, its lines' ends and both ends of each arc, which a line
		 * joins where it starts elsewhere), in the drawing's (A, B).
		 */
		TArray<FVector2D> Corners(const FFormProfile& Outline)
		{
			TArray<FVector2D> Out = {Outline.StartPoint()};
			for (const FFormProfileStep& Step : Outline.Steps)
			{
				if (!Step.IsArc())
				{
					Out.Add(FVector2D(Step.Line[0], Step.Line[1]));
					continue;
				}
				const FFormArc& Arc = Step.Arc;
				const FVector2D Centre(Arc.CenterMeters[0], Arc.CenterMeters[1]);
				const FVector2D Radii(Arc.RadiiMeters[0], Arc.RadiiMeters.Num() > 1 ? Arc.RadiiMeters[1] : Arc.RadiiMeters[0]);
				for (const double Degrees : {Arc.FromDegrees, Arc.ToDegrees})
				{
					const double Angle = FMath::DegreesToRadians(Degrees);
					Out.Add(Centre + Radii * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)));
				}
			}
			return Out;
		}

		/** How far a joint's slab reaches across the opening (its outline's widest |y|), and how far along it (x), in the joint's frame. */
		FVector2D Reach(const FSiteJoint& Joint, const FString& Name)
		{
			FVector2D Out(-TNumericLimits<double>::Max(), 0.0);
			if (const FFormDrawing* Drawing = Find(Joint, Name))
			{
				for (const FVector2D& Corner : Corners(Drawing->Shape.Get<FFormExtrusion>().Outline))
				{
					Out = FVector2D(FMath::Max(Out.X, Corner.X), FMath::Max(Out.Y, FMath::Abs(Corner.Y)));
				}
			}
			return Out;
		}

		/**
		 * Every strip of a joint runs the clear width, between the finished jambs (the sheets' "across the clear width",
		 * albion.md and chenghuai.md section 2), so no bronze lies under a jamb's foot; where the joint draws a lining, a
		 * stone lies under each of its feet beside every strip, between the clear and the rough jamb.
		 */
		void TestStripsClearWidth(FAutomationTestBase& Test, const FSiteJoint& Joint, const FString& Prefix, const FSitePortClass& Class)
		{
			const double Half = 0.5 * Class.ClearWidthMeters;
			const bool bLined = Joint.Drawings.ContainsByPredicate([](const FFormDrawing& D) { return D.Name.ToString().Contains(TEXT(".lining.jamb.")); });
			int32 Strips = 0;
			for (const FFormDrawing& Drawing : Joint.Drawings)
			{
				const FString Name = Drawing.Name.ToString();
				if (Name.StartsWith(Prefix + TEXT("strip.")))
				{
					++Strips;
					Test.TestEqual(*FString::Printf(TEXT("%s runs the clear width"), *Name), Reach(Joint, Name).Y, Half, 1e-9);
				}
			}
			Test.TestTrue(*FString::Printf(TEXT("%s lays a strip"), *Prefix), Strips > 0);
			if (!bLined)
			{
				return;
			}
			int32 Feet = 0;
			for (const FFormDrawing& Drawing : Joint.Drawings)
			{
				const FFormExtrusion* Slab = Drawing.Name.ToString().Contains(TEXT(".threshold.")) ? Drawing.Shape.TryGet<FFormExtrusion>() : nullptr;
				if (!Slab)
				{
					continue;
				}
				double Low = TNumericLimits<double>::Max();
				double High = -TNumericLimits<double>::Max();
				for (const FVector2D& Corner : Corners(Slab->Outline))
				{
					Low = FMath::Min(Low, Corner.Y);
					High = FMath::Max(High, Corner.Y);
				}
				const bool bBeside = FMath::IsNearlyEqual(Low, Half, 1e-9) || FMath::IsNearlyEqual(High, -Half, 1e-9);
				Feet += bBeside && FMath::IsNearlyEqual(High - Low, Class.AllowanceMeters, 1e-9) ? 1 : 0;
			}
			Test.TestEqual(*FString::Printf(TEXT("%s: a stone under each of the lining's feet beside each strip"), *Prefix), Feet, 2 * Strips);
		}

		/** Every class named in the ports tables of the design sheets (the third column of a row naming a port). */
		TSet<FString> ClassesTheSheetsName()
		{
			const FString Sheets = FPaths::ConvertRelativePathToFull(Musee::Records::DataRoot() / TEXT("../Docs/Stages"));
			TArray<FString> Files;
			IFileManager::Get().FindFiles(Files, *(Sheets / TEXT("*.md")), true, false);
			const FRegexPattern Row(TEXT("^\\|\\s*`[a-z0-9-]+\\.[a-z0-9.-]+`\\s*\\|\\s*(?:doorway|stair|vertical|shaft|gate)[^|]*\\|\\s*\\**`([a-z0-9.x-]+)`\\**\\s*\\|"));
			TSet<FString> Classes;
			for (const FString& File : Files)
			{
				TArray<FString> Lines;
				FFileHelper::LoadFileToStringArray(Lines, *(Sheets / File));
				for (const FString& Line : Lines)
				{
					FRegexMatcher Match(Row, Line);
					if (Match.FindNext())
					{
						Classes.Add(Match.GetCaptureGroup(1));
					}
				}
			}
			return Classes;
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSitePortClassesTest, "Musee.Site.PortClasses", JointFlags)
	bool FSitePortClassesTest::RunTest(const FString& Parameters)
	{
		const TSet<FString> Named = ClassesTheSheetsName();
		TestTrue(TEXT("the sheets name the museum's classes"), Named.Contains(TEXT("iris.4600")) && Named.Contains(TEXT("doorway.4000x4500.square"))
			&& Named.Contains(TEXT("doorway.3000x6000.arched")));
		for (const FString& Key : Named)
		{
			const TValueOrError<FSitePortClass, FString> Class = FindPortClass(FMuseeKey(Key));
			if (!TestTrue(*FString::Printf(TEXT("%s is a port class"), *Key), Class.HasValue()))
			{
				AddError(Class.GetError());
				continue;
			}
			const FSitePortClass& C = Class.GetValue();
			TestTrue(*FString::Printf(TEXT("%s's rough opening, surround and collar are sound"), *Key), C.AllowanceMeters >= 0.0 && C.SurroundMeters >= 0.0
				&& (C.CollarHalfWidthMeters == 0.0 || (C.CollarHalfWidthMeters > 0.5 * C.ClearWidthMeters + C.AllowanceMeters
				&& C.CollarTopMeters > C.CrownMeters() + C.AllowanceMeters && C.CollarBottomMeters < -C.AllowanceMeters)));
			TestTrue(*FString::Printf(TEXT("%s: an iris class has a well and a stacked iris"), *Key), !C.IsIris() || (C.WellRadiusMeters > 0.0
				&& C.Iris.IsStacked() && FMath::IsNearlyEqual(2.0 * C.WellRadiusMeters, C.ClearWidthMeters)));
		}
		const TValueOrError<FSiteJointRecord, FString> Joints = ReadJoints();
		TestTrue(TEXT("the joints' record reads"), Joints.HasValue() && Joints.GetValue().SurroundSection.Closed);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSiteVerticalPortsTest, "Musee.Site.VerticalPorts", JointFlags)
	bool FSiteVerticalPortsTest::RunTest(const FString& Parameters)
	{
		// The Atrium serves the well below its slab to -23.80 (atrium.md section 2); the Cube, below it, receives it.
		FSitePort Down = PortAt(TEXT("atrium.down"), TEXT("iris.4600"), FVector(0.0, 0.0, -0.5), FRotator(-90.0, 0.0, 0.0), 0.5, 0.0, false);
		Down.bHasWell = true;
		Down.Well.bServing = true;
		Down.Well.MinMeters = FVector(-2.3, -2.3, -23.8);
		Down.Well.MaxMeters = FVector(2.3, 2.3, -0.5);
		Down.Well.RadiusMeters = 2.3;
		FSitePort Up = PortAt(TEXT("cube.up"), TEXT("iris.4600"), FVector(0.0, 0.0, 12.0), FRotator(90.0, 0.0, 0.0), 0.0, 0.0, false);
		Up.bHasWell = true;
		Up.Well = Down.Well;
		Up.Well.bServing = false;
		Up.Well.MinMeters += FVector(0.0, 0.0, 12.5);
		Up.Well.MaxMeters += FVector(0.0, 0.0, 12.5);
		TMap<FMuseeKey, TArray<FSitePort>> Ports;
		Ports.Add(FMuseeKey(TEXT("atrium")), {Down});
		Ports.Add(FMuseeKey(TEXT("cube")), {Up});
		FSitePlan Plan;
		Plan.Anchor = FMuseeKey(TEXT("atrium"));
		FSiteMating& Mating = Plan.Matings.AddDefaulted_GetRef();
		Mating.A = FMuseeKey(TEXT("atrium.down"));
		Mating.B = FMuseeKey(TEXT("cube.up"));
		TArray<FMuseeFinding> Findings;
		const FSiteLayout Layout = Solve(Plan, Ports, Findings);
		TestEqual(TEXT("the ports mate with their wells coinciding"), Findings.Num(), 0);
		TestTrue(TEXT("the Cube stands under the Atrium's plane"), Layout.TranslationsMeters.FindRef(FMuseeKey(TEXT("cube"))).Equals(FVector(0.0, 0.0, -12.5), 1e-9));

		Ports[FMuseeKey(TEXT("cube"))][0].Well.RadiusMeters = 2.2;
		Findings.Reset();
		const FSiteLayout Narrow = Solve(Plan, Ports, Findings);
		TestTrue(TEXT("a well of another radius is found"), Findings.Num() == 1 && Findings[0].Check == FName(TEXT("Site.PortsMate")));
		TestFalse(TEXT("and doesn't place the stage"), Narrow.TranslationsMeters.Contains(FMuseeKey(TEXT("cube"))));

		// Unmated, the downward port is shut by its class's iris on its plane; the upward one by its own stage's.
		const TValueOrError<FSitePortClass, FString> Iris = FindPortClass(FMuseeKey(TEXT("iris.4600")));
		const TValueOrError<FSiteBlindDoors, FString> Doors = ReadBlindDoors();
		if (!TestTrue(TEXT("the records read"), Iris.HasValue() && Doors.HasValue()))
		{
			return false;
		}
		TArray<FFormDrawing> Blades;
		FString Why;
		TestTrue(TEXT("the downward port is blinded"), Blind(Down, Iris.GetValue(), Doors.GetValue(), Blades, Why));
		TestEqual(TEXT("by the iris's seven blades"), Blades.Num(), Iris.GetValue().Iris.Blades);
		for (int32 Blade = 0; Blade < Blades.Num(); ++Blade)
		{
			const double Top = Blades[Blade].Frame.OriginMeters.Z + Iris.GetValue().Iris.BladeMeters;
			TestEqual(TEXT("stacked at the pitch under the plane"), Top, -0.5 - Iris.GetValue().Iris.PitchMeters * Blade, 1e-9);
			TestEqual(TEXT("in the palette's pearl steel"), Blades[Blade].Material, FName(TEXT("white.pearl-steel")));
		}
		FSitePort Above = Up;
		Above.Key = FMuseeKey(TEXT("atrium.up"));
		TArray<FFormDrawing> None;
		TestTrue(TEXT("the upward port needs no blind"), Blind(Above, Iris.GetValue(), Doors.GetValue(), None, Why) && None.IsEmpty());
		FSiteJoint Seam;
		TestTrue(TEXT("mated, a vertical port's joint draws nothing"), Joint(Down, Placed(Up, FVector(0.0, 0.0, -12.5)), 0.0, Iris.GetValue(),
			ReadJoints().GetValue(), Seam, Why) && Seam.Drawings.IsEmpty());
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSiteJointTest, "Musee.Site.Joint", JointFlags)
	bool FSiteJointTest::RunTest(const FString& Parameters)
	{
		const TValueOrError<FSiteJointRecord, FString> Record = ReadJoints();
		const TValueOrError<FSitePortClass, FString> South = FindPortClass(FMuseeKey(TEXT("doorway.4000x6500.arched")));
		const TValueOrError<FSitePortClass, FString> West = FindPortClass(FMuseeKey(TEXT("doorway.3000x6000.arched")));
		if (!TestTrue(TEXT("the records read"), Record.HasValue() && South.HasValue() && West.HasValue()))
		{
			return false;
		}
		FString Why;

		// The Rotunda's south door on the drum (R 11.20, 1.20 deep, its own archivolt) and Albion's porch plane tangent to it
		// (0.10 lining): the wedges, one strip on the port plane across the clear width (albion.md section 2), the threshold
		// either side of it and under the lining's feet beside it, linings and Albion's surround.
		const FSitePort Rotunda = PortAt(TEXT("rotunda.south"), TEXT("doorway.4000x6500.arched"), FVector(0.0, 11.2, 0.0), FRotator(0.0, 90.0, 0.0), 1.2, 11.2, true);
		const FSitePort Albion = PortAt(TEXT("albion.north"), TEXT("doorway.4000x6500.arched"), FVector(0.0, 11.2, 0.0), FRotator(0.0, -90.0, 0.0), 0.1, 0.0, false);
		FSiteJoint Porch;
		if (!TestTrue(TEXT("the south door's joint is drawn"), Joint(Rotunda, Albion, 0.0, South.GetValue(), Record.GetValue(), Porch, Why)))
		{
			AddError(Why);
			return false;
		}
		const FString Prefix = TEXT("joint.rotunda.south.");
		const TSet<FString> Parts = PartsOf(Porch, Prefix);
		for (const TCHAR* Part : {TEXT("strip.0"), TEXT("threshold.0"), TEXT("threshold.1"), TEXT("threshold.2"), TEXT("threshold.3"),
			TEXT("lining.jamb.north"), TEXT("lining.jamb.south"), TEXT("lining.voussoir.00"), TEXT("masonry"), TEXT("surround.b")})
		{
			TestTrue(*FString::Printf(TEXT("the porch's joint draws its %s"), Part), Parts.Contains(Part));
		}
		TestFalse(TEXT("and no other stone"), Parts.Contains(TEXT("threshold.4")));
		TestEqual(TEXT("the south door's strip is 4.00 long, as the north door's"), 2.0 * Reach(Porch, Prefix + TEXT("strip.0")).Y, 4.0, 1e-9);
		TestStripsClearWidth(*this, Porch, Prefix, South.GetValue());
		TestFalse(TEXT("the Rotunda's own archivolt stands for its surround"), Parts.Contains(TEXT("surround.a")));
		if (const FFormDrawing* Masonry = Find(Porch, Prefix + TEXT("masonry")))
		{
			const FFormWall& Wall = Masonry->Shape.Get<FFormWall>();
			TestTrue(TEXT("the wedges fill between the port plane and the drum"), Wall.HasBack() && Wall.Openings.Num() == 1 && Wall.Openings[0].bStraight);
			TestEqual(TEXT("round the rough opening, 4.10 wide"), Wall.Openings[0].WidthMeters, 4.1, 1e-9);
			TestEqual(TEXT("within the collar, from -1.20 to 7.50"), Wall.BaseMeters + Wall.HeightMeters, 7.5, 1e-9);
		}
		// The voussoirs keep a chamfered end within a millimetre of the drum's inner face (r 10.00): each spans at most
		// sqrt(8 x 0.001 x 10) across the opening.
		int32 Voussoirs = 0;
		for (const FFormDrawing& Drawing : Porch.Drawings)
		{
			if (const FFormSweep* Stone = Drawing.Shape.TryGet<FFormSweep>(); Stone && Drawing.Name.ToString().Contains(TEXT("voussoir")))
			{
				++Voussoirs;
				TestFalse(TEXT("each voussoir's ends are chamfered to the faces"), Stone->StartCut.IsNearlyZero() || Stone->EndCut.IsNearlyZero());
			}
		}
		TestTrue(TEXT("the arch's lining is laid in voussoirs short enough for the drum"), Voussoirs >= FMath::CeilToInt32(UE_DOUBLE_PI * 2.05 / FMath::Sqrt(0.08)));
		for (const FFormDrawing& Drawing : Porch.Drawings)
		{
			TestTrue(TEXT("every part is drawn by Site in a palette material"), Drawing.Drawer == FName(TEXT("Site")) && !Drawing.Material.IsNone());
		}

		// The Rotunda's east door and the Hall of Light's west lining wrapped round the drum: the faces are one cylinder, so
		// there is no masonry and the one strip follows the mating arc.
		const FSitePort East = PortAt(TEXT("rotunda.east"), TEXT("doorway.3000x6000.arched"), FVector(11.2, 0.0, 0.0), FRotator(0.0, 0.0, 0.0), 1.2, 11.2, true);
		const FSitePort Hall = PortAt(TEXT("hall-of-light.west"), TEXT("doorway.3000x6000.arched"), FVector(11.2, 0.0, 0.0), FRotator(0.0, 180.0, 0.0), 0.05, -11.2, false);
		FSiteJoint Wrapped;
		TestTrue(TEXT("the east door's joint is drawn"), Joint(East, Hall, 0.0, West.GetValue(), Record.GetValue(), Wrapped, Why));
		const TSet<FString> WrappedParts = PartsOf(Wrapped, TEXT("joint.rotunda.east."));
		TestFalse(TEXT("faces on one cylinder need no masonry"), WrappedParts.Contains(TEXT("masonry")));
		TestTrue(TEXT("one strip, the threshold either side and under the lining's feet beside it"), WrappedParts.Contains(TEXT("strip.0"))
			&& !WrappedParts.Contains(TEXT("strip.1")) && WrappedParts.Contains(TEXT("threshold.3")) && !WrappedParts.Contains(TEXT("threshold.4")));
		TestStripsClearWidth(*this, Wrapped, TEXT("joint.rotunda.east."), West.GetValue());

		// The Rotunda's north door and Chenghuai's open porch, which ends on the drum's face with no wall of its own
		// (chenghuai.md section 2): no wedge and no masonry, and the strip lies in the drum, its far edge on the mating arc
		// where the porch's paving ends, with the threshold behind it. The strip runs the clear width, for no lining reaches
		// the porch's side, and the drum's lining stands over it: the threshold runs on beside it under the lining's feet.
		const FSitePort North = PortAt(TEXT("rotunda.north"), TEXT("doorway.4000x6500.arched"), FVector(0.0, -11.2, 0.0), FRotator(0.0, -90.0, 0.0), 1.2, 11.2, true);
		const FSitePort OpenPorch = PortAt(TEXT("chenghuai.south"), TEXT("doorway.4000x6500.arched"), FVector(0.0, -11.2, 0.0), FRotator(0.0, 90.0, 0.0), 0.0, -11.2, false);
		FSiteJoint Open;
		if (!TestTrue(TEXT("the north door's joint is drawn"), Joint(North, OpenPorch, 0.0, South.GetValue(), Record.GetValue(), Open, Why)))
		{
			AddError(Why);
		}
		const TSet<FString> OpenParts = PartsOf(Open, TEXT("joint.rotunda.north."));
		TestFalse(TEXT("a porch on the drum's face needs no masonry"), OpenParts.Contains(TEXT("masonry")));
		TestTrue(TEXT("one strip, the threshold behind it, and a stone under each of the lining's feet beside the strip"),
			OpenParts.Contains(TEXT("strip.0")) && OpenParts.Contains(TEXT("threshold.0")) && OpenParts.Contains(TEXT("threshold.1"))
			&& OpenParts.Contains(TEXT("threshold.2")) && !OpenParts.Contains(TEXT("threshold.3")));
		if (const FFormDrawing* Strip = Find(Open, TEXT("joint.rotunda.north.strip.0")))
		{
			// Both its edges are arcs about the drum's axis, the far one the drum's face itself (R 11.20), none beyond it.
			double Widest = 0.0;
			for (const FFormProfileStep& Step : Strip->Shape.Get<FFormExtrusion>().Outline.Steps)
			{
				Widest = Step.IsArc() ? FMath::Max(Widest, Step.Arc.RadiiMeters[0]) : Widest;
			}
			TestEqual(TEXT("the strip stays in the drum, its far edge on the mating arc"), Widest, 11.2, 1e-9);
		}
		TestStripsClearWidth(*this, Open, TEXT("joint.rotunda.north."), South.GetValue());

		// The Rotunda's west door and the Salon's flat end wall 2.20 beyond it: a passage, a strip at each face, the
		// threshold through, and the passage's masonry within the collar.
		const FSitePort RotundaWest = PortAt(TEXT("rotunda.west"), TEXT("doorway.3000x6000.arched"), FVector(-11.2, 0.0, 0.0), FRotator(0.0, 180.0, 0.0), 1.2, 11.2, true);
		const FSitePort Salon = PortAt(TEXT("salon.east"), TEXT("doorway.3000x6000.arched"), FVector(-13.4, 0.0, 0.0), FRotator(0.0, 0.0, 0.0), 0.6, 0.0, true);
		FSiteJoint Passage;
		TestTrue(TEXT("the west passage is drawn"), Joint(RotundaWest, Salon, 2.2, West.GetValue(), Record.GetValue(), Passage, Why));
		const TSet<FString> PassageParts = PartsOf(Passage, TEXT("joint.rotunda.west."));
		TestTrue(TEXT("two strips, three threshold stones and two beside each strip under the lining's feet, and the masonry"),
			PassageParts.Contains(TEXT("strip.1")) && PassageParts.Contains(TEXT("threshold.6")) && !PassageParts.Contains(TEXT("threshold.7"))
			&& PassageParts.Contains(TEXT("masonry")));
		TestStripsClearWidth(*this, Passage, TEXT("joint.rotunda.west."), West.GetValue());
		TestFalse(TEXT("both stages draw their own surrounds"), PassageParts.Contains(TEXT("surround.a")) || PassageParts.Contains(TEXT("surround.b")));

		FSiteJoint Apart;
		TestFalse(TEXT("ports that don't meet across the passage are refused"), Joint(RotundaWest, Salon, 1.0, West.GetValue(), Record.GetValue(), Apart, Why));
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSiteJointOwnershipTest, "Musee.Site.JointOwnership", JointFlags)
	bool FSiteJointOwnershipTest::RunTest(const FString& Parameters)
	{
		const TValueOrError<FSiteJointRecord, FString> Record = ReadJoints();
		const TValueOrError<FSitePortClass, FString> West = FindPortClass(FMuseeKey(TEXT("doorway.3000x6000.arched")));
		const TValueOrError<FSitePortClass, FString> Oval = FindPortClass(FMuseeKey(TEXT("doorway.3000x4800.arched")));
		const TValueOrError<FSitePortClass, FString> Stair = FindPortClass(FMuseeKey(TEXT("stairwell.straight.2400x3200.arched")));
		if (!TestTrue(TEXT("the records read"), Record.HasValue() && West.HasValue() && Oval.HasValue() && Stair.HasValue()))
		{
			return false;
		}
		FString Why;

		// The Salon finishes its own door behind its step, the inner 0.300 of its 0.600 wall (salon.md section 2): the joint's
		// lining stops against the step, and its threshold runs on to the wall's inner face the clear width.
		const FSitePort RotundaWest = PortAt(TEXT("rotunda.west"), TEXT("doorway.3000x6000.arched"), FVector(-11.2, 0.0, 0.0), FRotator(0.0, 180.0, 0.0), 1.2, 11.2, true);
		FSitePort Salon = PortAt(TEXT("salon.east"), TEXT("doorway.3000x6000.arched"), FVector(-13.4, 0.0, 0.0), FRotator(0.0, 0.0, 0.0), 0.6, 0.0, true);
		Salon.OwnLiningMeters = 0.3;
		FSiteJoint Stepped;
		TestTrue(TEXT("the stepped door's joint is drawn"), Joint(RotundaWest, Salon, 2.2, West.GetValue(), Record.GetValue(), Stepped, Why));
		const FString Prefix = TEXT("joint.rotunda.west.");
		TestEqual(TEXT("its lining stops against the step, 2.20 + 0.30 on"), Reach(Stepped, Prefix + TEXT("lining.jamb.north")).X, 2.5, 1e-9);
		TestEqual(TEXT("the threshold before the step is the rough width"), Reach(Stepped, Prefix + TEXT("threshold.2")).Y, 1.55, 1e-9);
		TestEqual(TEXT("and behind it the clear width, to the wall's inner face"), Reach(Stepped, Prefix + TEXT("threshold.3")).Y, 1.5, 1e-9);
		TestEqual(TEXT("which it reaches"), Reach(Stepped, Prefix + TEXT("threshold.3")).X, 2.8, 1e-9);
		TestStripsClearWidth(*this, Stepped, Prefix, West.GetValue());

		// The oval's passage (its own reveal, floor and architrave) against the Salon's far wall, cut to the rough opening
		// with its own architrave: the joint lines the Salon's wall and lays its threshold there, and its strip on the mating
		// plane runs the clear width, between the oval's finished jambs.
		FSitePort Passage = PortAt(TEXT("nympheas.east"), TEXT("doorway.3000x4800.arched"), FVector(-74.6, 0.0, 0.0), FRotator(0.0, 0.0, 0.0), 0.9, 0.0, true);
		Passage.OwnLiningMeters = Passage.WallDepthMeters;
		Passage.bOwnThreshold = true;
		const FSitePort FarWall = PortAt(TEXT("salon.west"), TEXT("doorway.3000x4800.arched"), FVector(-74.6, 0.0, 0.0), FRotator(0.0, 180.0, 0.0), 0.6, 0.0, true);
		FSiteJoint Lined;
		TestTrue(TEXT("the oval's joint is drawn"), Joint(Passage, FarWall, 0.0, Oval.GetValue(), Record.GetValue(), Lined, Why));
		const FString OvalPrefix = TEXT("joint.nympheas.east.");
		const TSet<FString> Parts = PartsOf(Lined, OvalPrefix);
		TestTrue(TEXT("a strip, a stone through the Salon's wall and one under each of its lining's feet beside the strip, the Salon's lining"),
			Parts.Contains(TEXT("strip.0")) && Parts.Contains(TEXT("threshold.0")) && Parts.Contains(TEXT("threshold.1")) && Parts.Contains(TEXT("threshold.2"))
			&& !Parts.Contains(TEXT("threshold.3")) && Parts.Contains(TEXT("lining.jamb.south")) && Parts.Contains(TEXT("lining.voussoir.00")));
		TestFalse(TEXT("and no surround: both stages draw their own"), Parts.Contains(TEXT("surround.a")) || Parts.Contains(TEXT("surround.b")));
		TestEqual(TEXT("the strip runs the clear width"), Reach(Lined, OvalPrefix + TEXT("strip.0")).Y, 1.5, 1e-9);
		TestStripsClearWidth(*this, Lined, OvalPrefix, Oval.GetValue());
		TestEqual(TEXT("the stone the rough width, under the lining"), Reach(Lined, OvalPrefix + TEXT("threshold.0")).Y, 1.55, 1e-9);
		TestEqual(TEXT("through the Salon's wall"), Reach(Lined, OvalPrefix + TEXT("threshold.0")).X, 0.6, 1e-9);
		// The lining stands on the mating plane, over the Salon's half of the strip, whose ends are the clear opening's: the
		// stone runs on under its feet there, from the plane to the strip's far edge, between the clear and rough jambs.
		for (const TCHAR* Foot : {TEXT("threshold.1"), TEXT("threshold.2")})
		{
			const FFormDrawing* Stone = Find(Lined, OvalPrefix + Foot);
			const FFormExtrusion* Slab = Stone ? Stone->Shape.TryGet<FFormExtrusion>() : nullptr;
			if (TestNotNull(TEXT("a stone under a foot of the lining"), Slab))
			{
				FVector2D Low(TNumericLimits<double>::Max()), High(-TNumericLimits<double>::Max());
				for (const FVector2D& Corner : Corners(Slab->Outline))
				{
					Low = FVector2D::Min(Low, Corner);
					High = FVector2D::Max(High, Corner);
				}
				TestEqual(TEXT("from the mating plane"), Low.X, 0.0, 1e-9);
				TestEqual(TEXT("to the strip's far edge"), High.X, 0.5 * Oval.GetValue().StripMeters, 1e-9);
				TestEqual(TEXT("between the clear and the rough jamb"), High.Y - Low.Y, Oval.GetValue().AllowanceMeters, 1e-9);
				TestEqual(TEXT("under the lining's clear face"), FMath::Min(FMath::Abs(Low.Y), FMath::Abs(High.Y)), 1.5, 1e-9);
			}
		}

		// The foot of the oval's stair at the Reserve's door: both finish their own openings and lay their own sills, so the
		// joint lays only the strip on the plane where they touch.
		FSitePort Foot = PortAt(TEXT("nympheas.stair"), TEXT("stairwell.straight.2400x3200.arched"), FVector(-74.6, 0.0, -5.8), FRotator(0.0, 0.0, 0.0), 0.4, 0.0, true);
		FSitePort Door = PortAt(TEXT("reserve.stair"), TEXT("stairwell.straight.2400x3200.arched"), FVector(-74.6, 0.0, -5.8), FRotator(0.0, 180.0, 0.0), 0.6, 0.0, true);
		for (FSitePort* Port : {&Foot, &Door})
		{
			Port->OwnLiningMeters = Port->WallDepthMeters;
			Port->bOwnThreshold = true;
		}
		FSiteJoint Sill;
		TestTrue(TEXT("the stair's joint is drawn"), Joint(Foot, Door, 0.0, Stair.GetValue(), Record.GetValue(), Sill, Why));
		const TSet<FString> SillParts = PartsOf(Sill, TEXT("joint.nympheas.stair."));
		TestTrue(TEXT("it is the strip alone"), SillParts.Num() == 1 && SillParts.Contains(TEXT("strip.0")));
		return true;
	}
}

#endif
