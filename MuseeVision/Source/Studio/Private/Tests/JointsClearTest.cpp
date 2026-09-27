#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "DynamicMesh/DynamicMeshAABBTree3.h"
#include "FormMake.h"
#include "Make/EngineUnits.h"
#include "SiteDescription.h"
#include "SiteJoint.h"
#include "SitePlan.h"
#include "SitePortClass.h"
#include "SiteRoom.h"
#include "Spatial/FastWinding.h"

/**
 * Every joint of the site plan stands clear of the two stages it joins: no solid of the joint (its threshold, strips,
 * linings, surrounds, masonry) and no solid of either stage share any space, so no two faces lie in one plane where they
 * overlap (they would flicker) and nothing is buried in a wall. Each joint is drawn between its stages placed as the
 * plan solves them, and made with them; every piece near it is sampled a millimetre inside each of its faces, and each
 * sample is asked whether it lies inside a piece on the other side (halfway through a solid thinner than two millimetres there).
 * One test per mating: Musee.Studio.JointsClear.<A>.
 *
 * And every joint closes on them: it stands clear of its stages but against them, so what it lays is carried and nothing
 * gapes beside it. The back of each lining (its jambs' and voussoirs' faces towards the rough opening) must have a
 * stage's wall or another piece of the joint right behind it, as must each end of a strip or a threshold stone across the
 * opening: a stage that cut its wall wider than the rough opening behind the joint's lining leaves a slot open to the
 * passage, and one that stopped its floor short of a strip's end leaves a hole. One test per mating:
 * Musee.Studio.JointsClosed.<A>.
 */
namespace Musee::Studio::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags JointsFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		/** How far inside a face a sample is taken: a millimetre, the museum's chord tolerance, in engine units. */
		constexpr double InsideEngine = 0.1;

		/** How far round the joint the stages' pieces are looked at, engine units: 10 cm. */
		constexpr double NearEngine = 10.0;

		/**
		 * How far behind a face a joint's piece must meet something, engine units: 2 mm, twice the chord tolerance, so the
		 * chords two neighbours divide one curve into never open a gap that isn't there.
		 */
		constexpr double BehindEngine = 0.2;

		/** How finely a face is sampled for what stands behind it: each of its triangles cut this many times along a side. */
		constexpr int32 FaceDivisions = 6;

		/** A face turns the way it is looked for when the cosine between its normal and that way is at least this (about 25°). */
		constexpr double FacingCosine = 0.9;

		/** A made solid placed in the museum, with what it takes to ask whether a point lies inside it. */
		struct FSolid
		{
			FString Name;
			const FFormMadePiece* Piece = nullptr;
			FTransform ToMuseum;
			FBox Bounds;
			TUniquePtr<UE::Geometry::FDynamicMeshAABBTree3> Tree;
			TUniquePtr<UE::Geometry::TFastWindingTree<UE::Geometry::FDynamicMesh3>> Winding;

			bool Contains(const FVector& Museum) const
			{
				return Bounds.IsInside(Museum) && Winding->IsInside(ToMuseum.InverseTransformPosition(Museum));
			}

			/** How far a point inside the solid lies from its nearest face, engine units. */
			double Depth(const FVector& Museum) const
			{
				const FVector Local = ToMuseum.InverseTransformPosition(Museum);
				return FVector::Distance(ToMuseum.TransformPosition(Tree->FindNearestPoint(Local)), Museum);
			}
		};

		FTransform EngineTransform(const FMuseeFrame& Frame)
		{
			return FTransform(Frame.Rotation, EngineFromMeters(Frame.OriginMeters));
		}

		TUniquePtr<FSolid> PlaceSolid(const FString& Name, const FFormMadePiece& Piece, const FTransform& ToMuseum)
		{
			TUniquePtr<FSolid> Solid = MakeUnique<FSolid>();
			Solid->Name = Name;
			Solid->Piece = &Piece;
			Solid->ToMuseum = ToMuseum;
			Solid->Bounds = FBox(ForceInit);
			for (const int32 V : Piece.Mesh->VertexIndicesItr())
			{
				Solid->Bounds += ToMuseum.TransformPosition(Piece.Mesh->GetVertex(V));
			}
			return Solid;
		}

		void IndexSolid(FSolid& Solid)
		{
			Solid.Tree = MakeUnique<UE::Geometry::FDynamicMeshAABBTree3>(&*Solid.Piece->Mesh);
			Solid.Winding = MakeUnique<UE::Geometry::TFastWindingTree<UE::Geometry::FDynamicMesh3>>(Solid.Tree.Get());
		}

		/**
		 * A millimetre inside each face of a solid, in the museum; or, where the solid is thinner than two millimetres behind
		 * the face (a wedge running out to its feather edge where two faces touch), halfway through it, for a sample outside
		 * its own solid would judge its neighbour, not the face.
		 */
		TArray<FVector> Samples(const FSolid& Solid)
		{
			TArray<FVector> Points;
			const UE::Geometry::FDynamicMesh3& Mesh = *Solid.Piece->Mesh;
			for (const int32 T : Mesh.TriangleIndicesItr())
			{
				const FVector Normal = Mesh.GetTriNormal(T);
				if (Normal.IsNearlyZero())
				{
					continue;
				}
				const FVector Centroid = Mesh.GetTriCentroid(T);
				UE::Geometry::IMeshSpatial::FQueryOptions Behind;
				Behind.TriangleFilterF = [T](int32 Other) { return Other != T; };
				double Through = 0.0;
				int32 Hit = IndexConstants::InvalidID;
				const bool bThin = Solid.Tree->FindNearestHitTriangle(FRay3d(Centroid, -Normal), Through, Hit, Behind) && Through < 2.0 * InsideEngine;
				Points.Add(Solid.ToMuseum.TransformPosition(Centroid - (bThin ? 0.5 * Through : InsideEngine) * Normal));
			}
			return Points;
		}

		/** A stage described, its pieces made, each placed in the museum where the plan puts the stage (a body's at rest). */
		struct FMadeStage
		{
			FSiteDescription Description;
			TArray<FFormMadePiece> Pieces;
		};

		bool MakeStagePieces(const FMuseeKey& Stage, FMadeStage& Out)
		{
			for (const USiteRoom* Room : USiteRoom::AllRooms())
			{
				FSiteDescription Description;
				Room->Describe(Description);
				if (Description.Stage == Stage)
				{
					Out.Description = MoveTemp(Description);
					TArray<FMuseeFinding> Findings;
					Musee::FormMaking::Make(Out.Description.Pieces, Out.Pieces, Findings);
					return true;
				}
			}
			return false;
		}

		FTransform StagePose(const FMadeStage& Stage, const FFormMadePiece& Piece, const FVector& Translation)
		{
			FMuseeFrame Frame = Piece.Frame;
			if (!Piece.Body.IsNone())
			{
				const FMuseeStageAnchor* Body = Stage.Description.Anchors.FindByPredicate([&Piece](const FMuseeStageAnchor& A)
				{
					return A.Key == Piece.Body && A.Kind == FName(TEXT("Body"));
				});
				Frame = Body ? Body->Frame.FrameToParent(Frame) : Frame;
			}
			return EngineTransform(Frame) * FTransform(EngineFromMeters(Translation));
		}

		TMap<FMuseeKey, TArray<FSitePort>> PortsOfRooms()
		{
			TMap<FMuseeKey, TArray<FSitePort>> Ports;
			for (const USiteRoom* Room : USiteRoom::AllRooms())
			{
				FSiteDescription Description;
				Room->Describe(Description);
				Ports.Add(Description.Stage, Description.Ports);
			}
			return Ports;
		}

		const FSitePort* FindMatedPort(const TMap<FMuseeKey, TArray<FSitePort>>& Ports, const FMuseeKey& Key)
		{
			const TArray<FSitePort>* OfStage = Ports.Find(Musee::Site::StageOfPort(Key));
			return OfStage ? OfStage->FindByPredicate([&Key](const FSitePort& Port) { return Port.Key == Key; }) : nullptr;
		}

		/** Every mating of the plan, one test each, named by its first port. */
		void MatingTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands)
		{
			const TValueOrError<FSitePlan, FString> Plan = Musee::Site::ReadPlan();
			if (Plan.HasError())
			{
				return;
			}
			for (const FSiteMating& Mating : Plan.GetValue().Matings)
			{
				OutBeautifiedNames.Add(Mating.A.ToString());
				OutTestCommands.Add(Mating.A.ToString());
			}
		}

		/** A joint drawn between its two stages where the plan places them, all made: the joint's solids, and each stage's near it. */
		struct FJointScene
		{
			FString A;
			FString B;
			FSitePortClass Class;
			TArray<FFormMadePiece> JointPieces;
			TArray<TUniquePtr<FSolid>> Joints;
			FMadeStage Stages[2];
			TArray<TUniquePtr<FSolid>> Walls[2];
			FBox Near = FBox(ForceInit);
		};

		enum class ESceneBuilt : uint8
		{
			Failed,
			/** Nothing to judge: a stage isn't built or placed, or the port is vertical, its faces touching. */
			Nothing,
			Built,
		};

		/** The joint of the mating a test is named by, drawn and made between its stages, and their pieces round it. */
		ESceneBuilt BuildScene(FAutomationTestBase& Test, const FString& Parameters, FJointScene& Scene)
		{
			const TValueOrError<FSitePlan, FString> Plan = Musee::Site::ReadPlan();
			const TValueOrError<FSiteJointRecord, FString> Record = Musee::Site::ReadJoints();
			if (!Test.TestTrue(TEXT("the site plan and the joints' record read"), Plan.HasValue() && Record.HasValue()))
			{
				return ESceneBuilt::Failed;
			}
			const FSiteMating* Mating = Plan.GetValue().Matings.FindByPredicate([&Parameters](const FSiteMating& M) { return M.A.ToString() == Parameters; });
			if (!Test.TestNotNull(TEXT("the mating is in the plan"), Mating))
			{
				return ESceneBuilt::Failed;
			}
			const TMap<FMuseeKey, TArray<FSitePort>> Ports = PortsOfRooms();
			TArray<FMuseeFinding> Solved;
			const FSiteLayout Layout = Musee::Site::Solve(Plan.GetValue(), Ports, Solved);
			const FMuseeKey StageA = Musee::Site::StageOfPort(Mating->A);
			const FMuseeKey StageB = Musee::Site::StageOfPort(Mating->B);
			const FSitePort* A = FindMatedPort(Ports, Mating->A);
			const FSitePort* B = FindMatedPort(Ports, Mating->B);
			const FVector* AtA = Layout.TranslationsMeters.Find(StageA);
			const FVector* AtB = Layout.TranslationsMeters.Find(StageB);
			if (!A || !B || !AtA || !AtB)
			{
				Test.AddInfo(FString::Printf(TEXT("%s and %s aren't both built and placed: no joint to judge"), *Mating->A.ToString(), *Mating->B.ToString()));
				return ESceneBuilt::Nothing;
			}
			const TValueOrError<FSitePortClass, FString> Class = Musee::Site::FindPortClass(A->Class);
			FSiteJoint Joint;
			FString Why;
			if (!Test.TestTrue(TEXT("its class reads"), Class.HasValue())
				|| !Test.TestTrue(TEXT("the joint is drawn"), Musee::Site::Joint(Musee::Site::Placed(*A, *AtA), Musee::Site::Placed(*B, *AtB),
					Mating->PassageMeters, Class.GetValue(), Record.GetValue(), Joint, Why)))
			{
				Test.AddError(Why);
				return ESceneBuilt::Failed;
			}
			if (Joint.Drawings.IsEmpty())
			{
				Test.AddInfo(TEXT("a vertical port: its faces touch and nothing lies between them"));
				return ESceneBuilt::Nothing;
			}
			Scene.A = Mating->A.ToString();
			Scene.B = Mating->B.ToString();
			Scene.Class = Class.GetValue();
			TArray<FMuseeFinding> Made;
			Musee::FormMaking::Make(Joint.Drawings, Scene.JointPieces, Made);
			for (const FMuseeFinding& Finding : Made)
			{
				Test.AddError(Finding.Summary);
			}
			for (const FFormMadePiece& Piece : Scene.JointPieces)
			{
				Scene.Joints.Add(PlaceSolid(Piece.Name.ToString(), Piece, EngineTransform(Piece.Frame)));
				IndexSolid(*Scene.Joints.Last());
				Scene.Near += Scene.Joints.Last()->Bounds;
			}
			Scene.Near = Scene.Near.ExpandBy(NearEngine);

			// Both stages' solids round the joint.
			const TPair<FMuseeKey, FVector> Sides[2] = {{StageA, *AtA}, {StageB, *AtB}};
			for (int32 Side = 0; Side < 2; ++Side)
			{
				FMadeStage& Stage = Scene.Stages[Side];
				if (!Test.TestTrue(*FString::Printf(TEXT("%s describes itself"), *Sides[Side].Key.ToString()), MakeStagePieces(Sides[Side].Key, Stage)))
				{
					return ESceneBuilt::Failed;
				}
				for (const FFormMadePiece& Piece : Stage.Pieces)
				{
					TUniquePtr<FSolid> Solid = PlaceSolid(FString::Printf(TEXT("%s.%s"), *Sides[Side].Key.ToString(), *Piece.Name.ToString()), Piece,
						StagePose(Stage, Piece, Sides[Side].Value));
					if (Solid->Bounds.Intersect(Scene.Near))
					{
						IndexSolid(*Solid);
						Scene.Walls[Side].Add(MoveTemp(Solid));
					}
				}
			}
			return ESceneBuilt::Built;
		}

		/**
		 * Whether a face of a joint's piece must have something right behind it, its normal and centroid in the joint's frame
		 * (x along the opening, y across it, z up from the threshold): a lining's back, away from the opening (across it
		 * beside the jambs, out from the arch's centre over the springing); and the ends of a strip or a threshold stone,
		 * across the opening.
		 */
		bool MustBeBacked(const FString& Name, const FVector& Normal, const FVector& Centroid, double HeadEngine)
		{
			if (Name.Contains(TEXT(".lining.")))
			{
				const FVector Away = Centroid.Z <= HeadEngine ? FVector(0.0, FMath::Sign(Centroid.Y), 0.0)
					: FVector(0.0, Centroid.Y, Centroid.Z - HeadEngine).GetSafeNormal();
				return FVector::DotProduct(Normal, Away) >= FacingCosine;
			}
			if (Name.Contains(TEXT(".strip.")) || Name.Contains(TEXT(".threshold.")))
			{
				return FMath::Abs(Normal.Y) >= FacingCosine;
			}
			return false;
		}
	}

	IMPLEMENT_COMPLEX_AUTOMATION_TEST(FStudioJointsClearTest, "Musee.Studio.JointsClear", JointsFlags)

	void FStudioJointsClearTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
	{
		MatingTests(OutBeautifiedNames, OutTestCommands);
	}

	bool FStudioJointsClearTest::RunTest(const FString& Parameters)
	{
		FJointScene Scene;
		const ESceneBuilt Built = BuildScene(*this, Parameters, Scene);
		if (Built != ESceneBuilt::Built)
		{
			return Built == ESceneBuilt::Nothing;
		}
		const FBox& Near = Scene.Near;

		// Every way round: a joint's face inside a stage's solid, a stage's inside a joint's, and one stage's inside the other's.
		// Each clash counts its faces and keeps the deepest: how far that face lies inside the other solid.
		struct FClash
		{
			int32 Faces = 0;
			double DeepestEngine = 0.0;
			FVector Deepest = FVector::ZeroVector;
		};
		TMap<FString, FClash> Clashes;
		auto Judge = [&Clashes, &Near](const TArray<TUniquePtr<FSolid>>& From, const TArray<TUniquePtr<FSolid>>& Into)
		{
			for (const TUniquePtr<FSolid>& Solid : From)
			{
				for (const FVector& Point : Samples(*Solid))
				{
					if (!Near.IsInside(Point))
					{
						continue;
					}
					for (const TUniquePtr<FSolid>& Other : Into)
					{
						if (Other->Contains(Point))
						{
							FClash& Clash = Clashes.FindOrAdd(FString::Printf(TEXT("%s lies inside %s"), *Solid->Name, *Other->Name));
							++Clash.Faces;
							const double Depth = Other->Depth(Point) + InsideEngine;
							if (Depth > Clash.DeepestEngine)
							{
								Clash.DeepestEngine = Depth;
								Clash.Deepest = Point;
							}
						}
					}
				}
			}
		};
		for (int32 Side = 0; Side < 2; ++Side)
		{
			Judge(Scene.Joints, Scene.Walls[Side]);
			Judge(Scene.Walls[Side], Scene.Joints);
			Judge(Scene.Walls[Side], Scene.Walls[1 - Side]);
		}
		for (const TPair<FString, FClash>& Clash : Clashes)
		{
			const FVector At = Clash.Value.Deepest / EngineUnitsPerMeter;
			AddError(FString::Printf(TEXT("%s (%d faces, up to %.1f mm, at museum (%.3f, %.3f, %.3f))"), *Clash.Key, Clash.Value.Faces,
				MetersFromEngine(Clash.Value.DeepestEngine) * 1000.0, At.X, At.Y, At.Z));
		}
		TestEqual(*FString::Printf(TEXT("the joint of %s and %s shares no space with them"), *Scene.A, *Scene.B), Clashes.Num(), 0);
		return true;
	}

	IMPLEMENT_COMPLEX_AUTOMATION_TEST(FStudioJointsClosedTest, "Musee.Studio.JointsClosed", JointsFlags)

	void FStudioJointsClosedTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
	{
		MatingTests(OutBeautifiedNames, OutTestCommands);
	}

	bool FStudioJointsClosedTest::RunTest(const FString& Parameters)
	{
		FJointScene Scene;
		const ESceneBuilt Built = BuildScene(*this, Parameters, Scene);
		if (Built != ESceneBuilt::Built)
		{
			return Built == ESceneBuilt::Nothing;
		}
		const double HeadEngine = Scene.Class.HeadMeters * EngineUnitsPerMeter;

		// Whether a point has a stage's solid or another of the joint's under it.
		auto Backed = [&Scene](const FVector& Point, const FSolid& Own)
		{
			for (const TArray<TUniquePtr<FSolid>>* Solids : {&Scene.Joints, &Scene.Walls[0], &Scene.Walls[1]})
			{
				for (const TUniquePtr<FSolid>& Other : *Solids)
				{
					if (Other.Get() != &Own && Other->Contains(Point))
					{
						return true;
					}
				}
			}
			return false;
		};

		// The solids nearest a point on each side (the joint's, then each stage's) within 10 cm, and how far off each lies.
		auto Nearest = [&Scene](const FVector& Point, const FSolid& Own)
		{
			TArray<FString> Found;
			for (const TArray<TUniquePtr<FSolid>>* Solids : {&Scene.Joints, &Scene.Walls[0], &Scene.Walls[1]})
			{
				double Closest = TNumericLimits<double>::Max();
				const FSolid* Nearest = nullptr;
				for (const TUniquePtr<FSolid>& Other : *Solids)
				{
					if (Other.Get() != &Own && Other->Bounds.ExpandBy(NearEngine).IsInside(Point))
					{
						const double Distance = Other->Depth(Point);
						if (Distance < Closest)
						{
							Closest = Distance;
							Nearest = Other.Get();
						}
					}
				}
				if (Nearest)
				{
					const FVector On = Nearest->ToMuseum.TransformPosition(Nearest->Tree->FindNearestPoint(Nearest->ToMuseum.InverseTransformPosition(Point)))
						/ EngineUnitsPerMeter;
					Found.Add(FString::Printf(TEXT("%s %.1f mm off at (%.4f, %.4f, %.4f)"), *Nearest->Name, MetersFromEngine(Closest) * 1000.0, On.X, On.Y, On.Z));
				}
			}
			return Found.IsEmpty() ? FString(TEXT("nothing within 10 cm")) : TEXT("nearest ") + FString::Join(Found, TEXT(", "));
		};

		// Each face that must be backed, sampled across its triangles 2 mm behind it. An open piece counts its open samples
		// and keeps the first, with the solid nearest it.
		struct FGap
		{
			int32 Samples = 0;
			FVector At = FVector::ZeroVector;
			FString Nearest;
			/** How far behind the face the first sample's gap runs before something stands there, engine units; zero past 20 cm. */
			double WidthEngine = 0.0;
		};
		TMap<FString, FGap> Gaps;
		int32 Judged = 0;
		for (const TUniquePtr<FSolid>& Solid : Scene.Joints)
		{
			// The joint's pieces are drawn in its frame (a threshold's and a strip's lowered, not turned), so their meshes lie in it.
			const UE::Geometry::FDynamicMesh3& Mesh = *Solid->Piece->Mesh;
			for (const int32 T : Mesh.TriangleIndicesItr())
			{
				const FVector Normal = Mesh.GetTriNormal(T);
				FVector P0, P1, P2;
				Mesh.GetTriVertices(T, P0, P1, P2);
				if (Normal.IsNearlyZero() || !MustBeBacked(Solid->Name, Normal, (P0 + P1 + P2) / 3.0, HeadEngine))
				{
					continue;
				}
				for (int32 U = 0; U < FaceDivisions; ++U)
				{
					for (int32 V = 0; U + V < FaceDivisions; ++V)
					{
						const double Along1 = (U + 1.0 / 3.0) / FaceDivisions;
						const double Along2 = (V + 1.0 / 3.0) / FaceDivisions;
						const FVector Point = Solid->ToMuseum.TransformPosition(P0 + Along1 * (P1 - P0) + Along2 * (P2 - P0) + BehindEngine * Normal);
						++Judged;
						if (!Backed(Point, *Solid))
						{
							FGap& Gap = Gaps.FindOrAdd(Solid->Name);
							if (Gap.Samples++ == 0)
							{
								Gap.At = Point;
								Gap.Nearest = Nearest(Point, *Solid);
								const FVector Out = Solid->ToMuseum.TransformVector(Normal).GetSafeNormal();
								for (double Behind = BehindEngine; Behind <= 2.0 * NearEngine && Gap.WidthEngine == 0.0; Behind += InsideEngine)
								{
									Gap.WidthEngine = Backed(Point + (Behind - BehindEngine) * Out, *Solid) ? Behind : 0.0;
								}
							}
						}
					}
				}
			}
		}
		TestTrue(TEXT("the joint has faces that must be backed"), Judged > 0);
		for (const TPair<FString, FGap>& Gap : Gaps)
		{
			const FVector At = Gap.Value.At / EngineUnitsPerMeter;
			const FString Width = Gap.Value.WidthEngine > 0.0 ? FString::Printf(TEXT("open %.1f mm deep"), MetersFromEngine(Gap.Value.WidthEngine) * 1000.0)
				: FString(TEXT("open past 20 cm"));
			AddError(FString::Printf(TEXT("nothing stands behind %s (%d samples, the first at museum (%.4f, %.4f, %.4f), %s; %s)"), *Gap.Key,
				Gap.Value.Samples, At.X, At.Y, At.Z, *Width, *Gap.Value.Nearest));
		}
		TestEqual(*FString::Printf(TEXT("the joint of %s and %s closes on them"), *Scene.A, *Scene.B), Gaps.Num(), 0);
		return true;
	}
}

#endif
