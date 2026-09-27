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
		// (0.10 lining): the wedges, one strip on the port plane, the threshold either side of it, linings and Albion's surround.
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
		for (const TCHAR* Part : {TEXT("strip.0"), TEXT("threshold.0"), TEXT("threshold.1"), TEXT("lining.jamb.north"), TEXT("lining.jamb.south"),
			TEXT("lining.voussoir.00"), TEXT("masonry"), TEXT("surround.b")})
		{
			TestTrue(*FString::Printf(TEXT("the porch's joint draws its %s"), Part), Parts.Contains(Part));
		}
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
		TestTrue(TEXT("one strip, the threshold either side"), WrappedParts.Contains(TEXT("strip.0")) && !WrappedParts.Contains(TEXT("strip.1"))
			&& WrappedParts.Contains(TEXT("threshold.1")));

		// The Rotunda's north door and Chenghuai's open porch, which ends on the drum's face with no wall of its own
		// (chenghuai.md section 2): no wedge and no masonry, and the strip lies in the drum, its far edge on the mating arc
		// where the porch's paving ends, with the threshold behind it.
		const FSitePort North = PortAt(TEXT("rotunda.north"), TEXT("doorway.4000x6500.arched"), FVector(0.0, -11.2, 0.0), FRotator(0.0, -90.0, 0.0), 1.2, 11.2, true);
		const FSitePort OpenPorch = PortAt(TEXT("chenghuai.south"), TEXT("doorway.4000x6500.arched"), FVector(0.0, -11.2, 0.0), FRotator(0.0, 90.0, 0.0), 0.0, -11.2, false);
		FSiteJoint Open;
		if (!TestTrue(TEXT("the north door's joint is drawn"), Joint(North, OpenPorch, 0.0, South.GetValue(), Record.GetValue(), Open, Why)))
		{
			AddError(Why);
		}
		const TSet<FString> OpenParts = PartsOf(Open, TEXT("joint.rotunda.north."));
		TestFalse(TEXT("a porch on the drum's face needs no masonry"), OpenParts.Contains(TEXT("masonry")));
		TestTrue(TEXT("one strip, and the threshold behind it only"), OpenParts.Contains(TEXT("strip.0")) && OpenParts.Contains(TEXT("threshold.0"))
			&& !OpenParts.Contains(TEXT("threshold.1")));
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

		// The Rotunda's west door and the Salon's flat end wall 2.20 beyond it: a passage, a strip at each face, the
		// threshold through, and the passage's masonry within the collar.
		const FSitePort RotundaWest = PortAt(TEXT("rotunda.west"), TEXT("doorway.3000x6000.arched"), FVector(-11.2, 0.0, 0.0), FRotator(0.0, 180.0, 0.0), 1.2, 11.2, true);
		const FSitePort Salon = PortAt(TEXT("salon.east"), TEXT("doorway.3000x6000.arched"), FVector(-13.4, 0.0, 0.0), FRotator(0.0, 0.0, 0.0), 0.6, 0.0, true);
		FSiteJoint Passage;
		TestTrue(TEXT("the west passage is drawn"), Joint(RotundaWest, Salon, 2.2, West.GetValue(), Record.GetValue(), Passage, Why));
		const TSet<FString> PassageParts = PartsOf(Passage, TEXT("joint.rotunda.west."));
		TestTrue(TEXT("two strips and three threshold stones, and the masonry"), PassageParts.Contains(TEXT("strip.1")) && PassageParts.Contains(TEXT("threshold.2"))
			&& PassageParts.Contains(TEXT("masonry")));
		TestFalse(TEXT("both stages draw their own surrounds"), PassageParts.Contains(TEXT("surround.a")) || PassageParts.Contains(TEXT("surround.b")));

		FSiteJoint Apart;
		TestFalse(TEXT("ports that don't meet across the passage are refused"), Joint(RotundaWest, Salon, 1.0, West.GetValue(), Record.GetValue(), Apart, Why));
		return true;
	}
}

#endif
