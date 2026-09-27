#include "Cast/Caster.h"
#include "Engine/StaticMesh.h"
#include "Misc/AutomationTest.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshResources.h"
#include "Tests/ProofPieces.h"
#include "Tests/ProofPlan.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace Musee::Studio
{
	namespace
	{
		UStaticMesh* LoadMesh(const FSoftObjectPath& Path)
		{
			return Cast<UStaticMesh>(Path.TryLoad());
		}

		/** Every vertex of the built mesh's LOD 0 (the Nanite fallback, which keeps every triangle), as the renderer has it. */
		TArray<FVector3f> BuiltNormals(const UStaticMesh& Mesh)
		{
			TArray<FVector3f> Normals;
			const FStaticMeshLODResources& Lod = Mesh.GetRenderData()->LODResources[0];
			for (uint32 Vertex = 0; Vertex < Lod.VertexBuffers.StaticMeshVertexBuffer.GetNumVertices(); ++Vertex)
			{
				Normals.Add(Lod.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(Vertex));
			}
			return Normals;
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioCastTest, "Musee.Studio.Cast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStudioCastTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Studio;

	TArray<FCastInput> Inputs;
	FCastInput& Square = Inputs.AddDefaulted_GetRef();
	Square.PackageName = ProofPaths().Root + TEXT("/Cases/SM_Case_LeaningSquare");
	Square.Description = DrawLeaningSquare();
	Square.Slots = {{TEXT("Stone"), FSoftObjectPath(TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial"))}};
	FCastInput& Wall = Inputs.AddDefaulted_GetRef();
	Wall.PackageName = ProofPaths().Root + TEXT("/Cases/SM_Case_GlazedWall");
	Wall.Description = BuildGlazedWall();
	Wall.Slots = {{TEXT("Stone"), FSoftObjectPath(TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial"))},
		{TEXT("Glass"), FSoftObjectPath(TEXT("/Engine/EngineDebugMaterials/M_SimpleTranslucent.M_SimpleTranslucent"))}};

	TArray<FMadePackage> Made;
	FCastStats Stats;
	const TArray<FCastResult> Results = CastMeshes(Inputs, Made, Stats);
	for (const FMadePackage& Package : Made)
	{
		TestTrue(FString::Printf(TEXT("%s is made (%s)"), *Package.PackageName, *Package.Why), Package.Outcome != EMadeOutcome::Failed);
	}
	if (!TestEqual(TEXT("the square casts whole and the wall in two"), Results[0].Meshes.Num() + Results[1].Meshes.Num(), 3))
	{
		return false;
	}

	UStaticMesh* Leaning = LoadMesh(Results[0].Meshes[0].Path);
	UStaticMesh* Stone = LoadMesh(Results[1].Meshes[0].Path);
	UStaticMesh* Glass = LoadMesh(Results[1].Meshes[1].Path);
	if (!TestTrue(TEXT("the cast meshes load"), Leaning && Stone && Glass))
	{
		return false;
	}
	TestTrue(TEXT("opaque parts are Nanite"), Leaning->IsNaniteEnabled() && Stone->IsNaniteEnabled());
	TestFalse(TEXT("translucent parts are not Nanite"), Glass->IsNaniteEnabled());
	TestEqual(TEXT("an opaque part has a distance field"), Stone->GetSourceModel(0).BuildSettings.DistanceFieldResolutionScale, 1.0f);
	TestEqual(TEXT("a translucent part has none"), Glass->GetSourceModel(0).BuildSettings.DistanceFieldResolutionScale, 0.0f);
	TestTrue(TEXT("collision is the mesh itself"), Stone->GetBodySetup()->CollisionTraceFlag == CTF_UseComplexAsSimple);
	TestTrue(TEXT("the opaque part carries a distance field"), Stone->GetRenderData()->LODResources[0].DistanceFieldData != nullptr);
	TestTrue(TEXT("the opaque part carries Lumen cards"), Stone->GetRenderData()->LODResources[0].CardRepresentationData != nullptr);

	// A recomputed normal of the flat square points straight up; the drawn ones lean outward by LeanDegrees.
	const float DrawnCosine = FMath::Cos(FMath::DegreesToRadians(LeanDegrees));
	const TArray<FVector3f> Normals = BuiltNormals(*Leaning);
	TestEqual(TEXT("the fallback keeps the square's four corners"), Normals.Num(), 4);
	for (const FVector3f& Normal : Normals)
	{
		TestEqual(TEXT("a built normal keeps its drawn lean"), Normal.Z, DrawnCosine, 2.0e-3f);
	}
	if (!Normals.IsEmpty())
	{
		AddInfo(FString::Printf(TEXT("Built normal of the leaning square: %s (drawn lean %.1f degrees, vertical component %.4f)."),
			*Normals[0].ToString(), LeanDegrees, DrawnCosine));
	}
	return true;
}

#endif
