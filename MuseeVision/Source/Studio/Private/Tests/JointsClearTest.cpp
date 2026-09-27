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
	}

	IMPLEMENT_COMPLEX_AUTOMATION_TEST(FStudioJointsClearTest, "Musee.Studio.JointsClear", JointsFlags)

	void FStudioJointsClearTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
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

	bool FStudioJointsClearTest::RunTest(const FString& Parameters)
	{
		const TValueOrError<FSitePlan, FString> Plan = Musee::Site::ReadPlan();
		const TValueOrError<FSiteJointRecord, FString> Record = Musee::Site::ReadJoints();
		if (!TestTrue(TEXT("the site plan and the joints' record read"), Plan.HasValue() && Record.HasValue()))
		{
			return false;
		}
		const FSiteMating* Mating = Plan.GetValue().Matings.FindByPredicate([&Parameters](const FSiteMating& M) { return M.A.ToString() == Parameters; });
		if (!TestNotNull(TEXT("the mating is in the plan"), Mating))
		{
			return false;
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
			AddInfo(FString::Printf(TEXT("%s and %s aren't both built and placed: no joint to judge"), *Mating->A.ToString(), *Mating->B.ToString()));
			return true;
		}
		const TValueOrError<FSitePortClass, FString> Class = Musee::Site::FindPortClass(A->Class);
		FSiteJoint Joint;
		FString Why;
		if (!TestTrue(TEXT("its class reads"), Class.HasValue())
			|| !TestTrue(TEXT("the joint is drawn"), Musee::Site::Joint(Musee::Site::Placed(*A, *AtA), Musee::Site::Placed(*B, *AtB), Mating->PassageMeters,
				Class.GetValue(), Record.GetValue(), Joint, Why)))
		{
			AddError(Why);
			return false;
		}
		if (Joint.Drawings.IsEmpty())
		{
			AddInfo(TEXT("a vertical port: its faces touch and nothing lies between them"));
			return true;
		}
		TArray<FFormMadePiece> JointPieces;
		TArray<FMuseeFinding> Made;
		Musee::FormMaking::Make(Joint.Drawings, JointPieces, Made);
		for (const FMuseeFinding& Finding : Made)
		{
			AddError(Finding.Summary);
		}
		TArray<TUniquePtr<FSolid>> Joints;
		FBox Near(ForceInit);
		for (const FFormMadePiece& Piece : JointPieces)
		{
			Joints.Add(PlaceSolid(Piece.Name.ToString(), Piece, EngineTransform(Piece.Frame)));
			IndexSolid(*Joints.Last());
			Near += Joints.Last()->Bounds;
		}
		Near = Near.ExpandBy(NearEngine);

		// Both stages' solids round the joint.
		FMadeStage Stages[2];
		TArray<TUniquePtr<FSolid>> Walls[2];
		const TPair<FMuseeKey, FVector> Sides[2] = {{StageA, *AtA}, {StageB, *AtB}};
		for (int32 Side = 0; Side < 2; ++Side)
		{
			if (!TestTrue(*FString::Printf(TEXT("%s describes itself"), *Sides[Side].Key.ToString()), MakeStagePieces(Sides[Side].Key, Stages[Side])))
			{
				return false;
			}
			for (const FFormMadePiece& Piece : Stages[Side].Pieces)
			{
				TUniquePtr<FSolid> Solid = PlaceSolid(FString::Printf(TEXT("%s.%s"), *Sides[Side].Key.ToString(), *Piece.Name.ToString()), Piece,
					StagePose(Stages[Side], Piece, Sides[Side].Value));
				if (Solid->Bounds.Intersect(Near))
				{
					IndexSolid(*Solid);
					Walls[Side].Add(MoveTemp(Solid));
				}
			}
		}

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
			Judge(Joints, Walls[Side]);
			Judge(Walls[Side], Joints);
			Judge(Walls[Side], Walls[1 - Side]);
		}
		for (const TPair<FString, FClash>& Clash : Clashes)
		{
			const FVector At = Clash.Value.Deepest / EngineUnitsPerMeter;
			AddError(FString::Printf(TEXT("%s (%d faces, up to %.1f mm, at museum (%.3f, %.3f, %.3f))"), *Clash.Key, Clash.Value.Faces,
				MetersFromEngine(Clash.Value.DeepestEngine) * 1000.0, At.X, At.Y, At.Z));
		}
		TestEqual(*FString::Printf(TEXT("the joint of %s and %s shares no space with them"), *Mating->A.ToString(), *Mating->B.ToString()), Clashes.Num(), 0);
		return true;
	}
}

#endif
