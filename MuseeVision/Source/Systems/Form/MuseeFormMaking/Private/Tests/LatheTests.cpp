#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Kernel/Lathe.h"
#include "Kernel/MeshBuilder.h"
#include "Kernel/Profile.h"

namespace Musee::Form::Tests
{
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormLatheUVMetersTest, "Musee.Form.LatheUVMeters",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

	bool FFormLatheUVMetersTest::RunTest(const FString& Parameters)
	{
		// A 5 m drum 3 m high under a hemispherical dome, turned once round: the profile runs up the drum, over the
		// springing (tangent, so smooth) and up the dome to the crown on the axis.
		const double Radius = 5.0;
		const FProfileStations Section = FProfile(FVector2D(Radius, -3.0))
			.LineTo(FVector2D(Radius, 0.0))
			.Arc(FVector2D(0.0, 0.0), Radius, 0.0, UE_DOUBLE_HALF_PI)
			.Resolve(0.001);
		const FLatheAxis Axis;
		const TArray<double> Angles = LatheStations(Section, 0.0, UE_DOUBLE_TWO_PI);
		TestEqual(TEXT("The drum is the widest parallel"), LargestRadius(Section), Radius);
		const TArray<FSweepFrame> Frames = LatheFrames(Axis, Angles, Radius);
		FMeshBuilder Mesh;
		Lathe(Mesh, Section, Axis, Angles);
		TestTrue(TEXT("The lathe made a surface"), Mesh.NumTriangles() > 0);

		// Round the drum, the reference parallel, U grows by the true arc length R dAngle, which the 1 mm chord
		// matches to within the tolerance. Down the profile V grows by the true distance.
		int32 WrongU = 0;
		int32 WrongV = 0;
		for (int32 Frame = 0; Frame + 1 < Frames.Num(); ++Frame)
		{
			const FVector2D Foot = Section.Vertices[0].Point;
			const double Arc = Radius * (Angles[Frame + 1] - Angles[Frame]);
			const double U = Frames[Frame + 1].U - Frames[Frame].U;
			const double Chord = FVector::Distance(Frames[Frame].At(Foot), Frames[Frame + 1].At(Foot));
			const bool bTrueArc = FMath::IsNearlyEqual(U, Arc, 1e-12);
			const bool bMatchesChord = U >= Chord - 1e-12 && U - Chord <= 0.001;
			WrongU += (bTrueArc && bMatchesChord) ? 0 : 1;
			for (int32 Station = 0; Station + 1 < Section.Vertices.Num(); ++Station)
			{
				const FVector2D Point = Section.Vertices[Station].Point;
				const FVector2D Next = Section.Vertices[Station + 1].Point;
				const double Down = FVector::Distance(Frames[Frame].At(Point), Frames[Frame].At(Next));
				const double V = Section.Vertices[Station + 1].LengthMeters - Section.Vertices[Station].LengthMeters;
				WrongV += FMath::IsNearlyEqual(V, Down, 1e-12) ? 0 : 1;
			}
		}
		TestEqual(TEXT("Steps round the drum whose U is not its length in metres"), WrongU, 0);
		TestEqual(TEXT("Steps down the profile whose V is not their length in metres"), WrongV, 0);

		// Every vertex carries the U of its meridian, R times its angle, so the texture never shears; the whole U range
		// round the drum is its circumference in metres, never a 0 to 1 stretched round it.
		int32 Sheared = 0;
		double DrumSpan = 0.0;
		for (int32 Vertex = 0; Vertex < Mesh.NumVertices(); ++Vertex)
		{
			const FVector P = Mesh.Position(Vertex);
			const double U = Mesh.UV(Vertex).X;
			DrumSpan = P.Z < -1.0 ? FMath::Max(DrumSpan, U) : DrumSpan;
			if (FVector2D(P.X, P.Y).Size() > 1e-9)
			{
				const double Angle = FMath::Atan2(P.Y, P.X);
				const double Turned = Angle < 0.0 ? Angle + UE_DOUBLE_TWO_PI : Angle;
				const bool bOnMeridian = FMath::IsNearlyEqual(U, Radius * Turned, 1e-9)
					|| FMath::IsNearlyEqual(U, Radius * (Turned + UE_DOUBLE_TWO_PI), 1e-9);
				Sheared += bOnMeridian ? 0 : 1;
			}
		}
		TestEqual(TEXT("Vertices whose U is not their meridian's"), Sheared, 0);
		TestEqual(TEXT("U round the drum spans its circumference"), DrumSpan, UE_DOUBLE_TWO_PI * Radius, 1e-9);

		// The dome's section is exact: its stations lie on the sphere and their normals point out from its centre.
		int32 OffSphere = 0;
		for (int32 Vertex = 0; Vertex < Mesh.NumVertices(); ++Vertex)
		{
			const FVector P = Mesh.Position(Vertex);
			if (P.Z > 1e-9)
			{
				const bool bOn = FMath::IsNearlyEqual(P.Size(), Radius, 1e-12);
				const bool bNormal = Mesh.Normal(Vertex).Equals(P.GetSafeNormal(), 1e-9);
				OffSphere += (bOn && bNormal) ? 0 : 1;
			}
		}
		TestEqual(TEXT("Dome vertices off the sphere or with a normal not radial"), OffSphere, 0);
		return true;
	}
}

#endif
