#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Kernel/Lathe.h"
#include "Kernel/MeshBuilder.h"
#include "Kernel/Tangents.h"
#include "Math/RandomStream.h"
#include "Tests/FormTestShapes.h"

namespace Musee::Form::Tests
{
	namespace
	{
		/** One triangle facing Normal with the given UVs; returns the tangent frame at its first corner. */
		FVertexTangent TriangleTangent(const FVector (&Points)[3], const FVector2D (&UVs)[3], const FVector& Normal)
		{
			FMeshBuilder Mesh;
			for (int32 Corner = 0; Corner < 3; ++Corner)
			{
				Mesh.AddVertex(Points[Corner], Normal, UVs[Corner]);
			}
			Mesh.AddTriangle(0, 1, 2);
			return DeriveTangents(Mesh)[0];
		}

		/**
		 * The handedness the UVs have seen from the side the normal faces: +1 where the UV triangle winds the same way
		 * as the triangle does seen from there, -1 where it is mirrored.
		 */
		float ExpectedSign(const FVector (&Points)[3], const FVector2D (&UVs)[3], const FVector& Normal)
		{
			const FVector Across = FVector::CrossProduct(Points[1] - Points[0], Points[2] - Points[0]);
			const double Seen = FVector::DotProduct(Across, Normal);
			const double Drawn = FVector2D::CrossProduct(UVs[1] - UVs[0], UVs[2] - UVs[0]);
			return (Seen > 0.0) == (Drawn > 0.0) ? 1.0f : -1.0f;
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormTangentHandednessTest, "Musee.Form.TangentHandedness",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

	bool FFormTangentHandednessTest::RunTest(const FString& Parameters)
	{
		const FVector Floor[3] = {FVector(0, 0, 0), FVector(1, 0, 0), FVector(0, 1, 0)};

		// A floor seen from above with U east and V south: TangentX east, and the binormal cross(up, east) is south.
		const FVector2D Plan[3] = {FVector2D(0, 0), FVector2D(1, 0), FVector2D(0, 1)};
		const FVertexTangent Plain = TriangleTangent(Floor, Plan, FVector::UpVector);
		TestTrue(TEXT("Plan UVs: tangent along +U"), Plain.TangentX.Equals(FVector::ForwardVector, 1e-12));
		TestEqual(TEXT("Plan UVs: right-handed"), Plain.BinormalSign, 1.0f);

		// The same floor with U mirrored: the tangent turns round and the handedness flips.
		const FVector2D Mirrored[3] = {FVector2D(0, 0), FVector2D(-1, 0), FVector2D(0, 1)};
		const FVertexTangent Flipped = TriangleTangent(Floor, Mirrored, FVector::UpVector);
		TestTrue(TEXT("Mirrored UVs: tangent along -x"), Flipped.TangentX.Equals(-FVector::ForwardVector, 1e-12));
		TestEqual(TEXT("Mirrored UVs: left-handed"), Flipped.BinormalSign, -1.0f);

		// The plan UVs seen from below (a soffit): the same UVs wind the other way.
		const FVertexTangent Soffit = TriangleTangent(Floor, Plan, -FVector::UpVector);
		TestEqual(TEXT("Plan UVs seen from below: left-handed"), Soffit.BinormalSign, -1.0f);

		// Random triangles, sides and affine UV maps: the sign follows the UV winding seen from the normal, the
		// tangent is unit and on the surface, and the binormal it implies points along increasing V.
		FRandomStream Random(20260926);
		int32 Wrong = 0;
		for (int32 Trial = 0; Trial < 500; ++Trial)
		{
			const FVector Points[3] = {Random.VRand() * 3.0, Random.VRand() * 3.0, Random.VRand() * 3.0};
			const FVector Across = FVector::CrossProduct(Points[1] - Points[0], Points[2] - Points[0]);
			if (Across.Size() < 0.05)
			{
				continue;
			}
			const FVector Normal = Across.GetSafeNormal() * (Random.FRand() < 0.5f ? -1.0 : 1.0);
			const FVector2D DU(Random.FRandRange(-2, 2), Random.FRandRange(-2, 2));
			const FVector2D DV(Random.FRandRange(-2, 2), Random.FRandRange(-2, 2));
			if (FMath::Abs(FVector2D::CrossProduct(DU, DV)) < 0.05)
			{
				continue;
			}
			const FVector2D UVs[3] = {FVector2D(0.5, 0.5), FVector2D(0.5, 0.5) + DU, FVector2D(0.5, 0.5) + DV};
			const FVertexTangent Frame = TriangleTangent(Points, UVs, Normal);
			const bool bUnit = FMath::IsNearlyEqual(Frame.TangentX.Size(), 1.0, 1e-9);
			const bool bOnSurface = FMath::Abs(FVector::DotProduct(Frame.TangentX, Normal)) < 1e-9;
			const bool bHanded = Frame.BinormalSign == ExpectedSign(Points, UVs, Normal);
			Wrong += (bUnit && bOnSurface && bHanded) ? 0 : 1;
		}
		TestEqual(TEXT("Random triangles whose tangent frame disagrees with their UVs"), Wrong, 0);

		// A full lathe with metre UVs along each parallel: no mirroring anywhere, tangents along the turn.
		FMeshBuilder Drum;
		const FProfileStations Wall = FProfile(FVector2D(5.0, 0.0)).LineTo(FVector2D(5.0, 4.0)).Resolve(0.001);
		const FLatheAxis Axis;
		const TArray<double> Angles = LatheStations(Wall, 0.0, UE_DOUBLE_TWO_PI);
		Lathe(Drum, Wall, Axis, Angles);
		const TArray<FVertexTangent> Tangents = DeriveTangents(Drum);
		int32 Astray = 0;
		for (int32 Vertex = 0; Vertex < Drum.NumVertices(); ++Vertex)
		{
			const FVector P = Drum.Position(Vertex);
			const FVector Along = FVector::CrossProduct(Axis.Up, FVector(P.X, P.Y, 0.0)).GetSafeNormal();
			const bool bAlong = FVector::DotProduct(Tangents[Vertex].TangentX, Along) > 0.999;
			const bool bSameHand = Tangents[Vertex].BinormalSign == Tangents[0].BinormalSign;
			Astray += (bAlong && bSameHand) ? 0 : 1;
		}
		TestEqual(TEXT("Lathe vertices whose tangent leaves the turn or whose handedness differs"), Astray, 0);
		return true;
	}
}

#endif
